#include "DiscreteGreedy.h"
#include <algorithm>
#include <math.h>

using namespace spdlog;

DiscreteGreedy::DiscreteGreedy(Network & g) : PathBlockingAlgorithm(g)
{
}

vector<double> DiscreteGreedy::block(vector<vector<uint>> const & list_paths)
{
	last_block_queries = 0;
	this->list_paths = list_paths;
	const int no_nodes = g.get_no_nodes();
	vector<double> x(no_nodes, 0); // returned result
	uint no_done_paths = initiate();
	info("No. input paths for DG", list_paths.size());
	info("No. done paths: {}", no_done_paths);

	while (no_done_paths < list_paths.size()) {
		uint best_node_id = no_nodes;
		double best_invest = 0.0;

		#pragma omp parallel for
		for (int n = 0; n < no_nodes; ++n) {
			auto vals_pair = get_invest(n);
			const auto& node_invest = vals_pair.first;
			const auto& is_queried = vals_pair.second;
			if (is_queried) {
				#pragma omp critical
				{
					++last_block_queries;
				}
			}
			if (node_invest > best_invest) {
				#pragma omp critical
				{
					if (node_invest > best_invest) {
						best_invest = node_invest;
						best_node_id = n;
					}
				}
			}
		}



		if (best_node_id == no_nodes) {
			info("Discrete Greedy cannot find solution");
			throw "Error here, cannot find solution";
		}

		info("Add {} budget to node {}", Constants::DISCRETE_STEP, best_node_id);
		x[best_node_id] += Constants::DISCRETE_STEP;
		// update edge and path working length and return no. new exceed paths
		uint no_new_done_paths = update_node_and_path_length(
			best_node_id, Constants::DISCRETE_STEP);
		no_done_paths += no_new_done_paths;
		//info("No. done paths: {}", no_done_paths);
	}

	return x;
}

pair<double, bool> DiscreteGreedy::get_invest(uint const & node_id)
{
	vector<double> sorted_path_lens;
	for (auto const& inv_path_id : involved_paths[node_id]) {
		if (path_lengths[inv_path_id] <
			Constants::T * (1 - Constants::VAREPSILON) - Constants::PRECISION)
			sorted_path_lens.push_back(path_lengths[inv_path_id]);
	}

	if (sorted_path_lens.size() == 0)
		return make_pair(0.0, false);

	sort(sorted_path_lens.begin(), sorted_path_lens.end());
	const double current_budget = g.get_node_current_budget(node_id);

	vector<double> comp_budgets, comp_lengths; // complement budget and length for each path to reach t_u
	for (auto const& path_len : sorted_path_lens) {
		double comp_len = max(0.0, Constants::T - path_len);
		double comp_budget =
			QoScommon::get_complement_budget(Constants::FUNCTION, current_budget, comp_len);
		comp_budgets.emplace_back(comp_budget);
		comp_lengths.emplace_back(comp_len);
	}

	double inc_D = 0;
	double inc_node_weight =
		QoScommon::get_increase_weight(
			Constants::FUNCTION, current_budget, Constants::DISCRETE_STEP);

	for (int i = 0; i < comp_budgets.size(); ++i) {
		if (Constants::DISCRETE_STEP < comp_budgets[i]) // not sufficient to make this path exceed t_u
			inc_D += inc_node_weight;
		else
			inc_D += comp_lengths[i];
	}
	double invest = inc_D / Constants::DISCRETE_STEP;
	return make_pair(invest, true);
}
