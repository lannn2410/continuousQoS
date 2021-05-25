#include "ThresholdExpansion.h"
#include <algorithm>
#include <math.h>

using namespace spdlog;

ThresholdExpansion::ThresholdExpansion(Network& g) : 
	PathBlockingAlgorithm(g)
{

}

vector<double> ThresholdExpansion::block(vector<vector<uint>> const& list_paths)
{
	last_block_queries = 0;
	this->list_paths = list_paths;
	double M = QoScommon::getM(Constants::FUNCTION, Constants::T);
	M *= list_paths.size();
	info("Initiate M = {}", M);
	const int no_nodes = g.get_no_nodes();
	uint n = 0; // start node
	vector<double> x(no_nodes, 0); // returned result
	uint no_done_paths = initiate();
	info("No. input paths for TE: {}", list_paths.size());
	info("No. done paths: {}", no_done_paths);
	while (no_done_paths < list_paths.size()) {
		// get increasing budget for e
		auto const b = get_increasing_budget(n, M);
		if (b > 0) {
			info("Add {} budget to node {}", b, n);
			x[n] += b;
			// update edge and path working length and return no. new exceed paths
			uint no_new_done_paths = update_node_and_path_length(n, b);
			no_done_paths += no_new_done_paths;
		}

		//info("No. done paths: {}", no_done_paths);

		// consider the next node
		++n;
		if (n == no_nodes) {
			n = 0;
			M *= 1 - Constants::EPS_TE;
			info("Reduce M to {}", M);
		}
	}

	return x;
}

double ThresholdExpansion::get_increasing_budget(
	uint const & node_id, const double& M)
{
	double re = 0;

	vector<double> sorted_path_lens;
	for (auto const& inv_path_id : involved_paths[node_id]) {
		if (path_lengths[inv_path_id] < 
			Constants::T * (1-Constants::VAREPSILON) - Constants::PRECISION)
			sorted_path_lens.push_back(path_lengths[inv_path_id]);
	}

	if (sorted_path_lens.size() == 0)
		return re;

	++last_block_queries;

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

	// discretize the continous function
	int no_discrete_steps = ceil(comp_budgets[0] / Constants::DISCRETE_STEP);
	bool stop = false;
	#pragma omp parallel for shared(stop)
	for (int x = no_discrete_steps; x > 0; --x) {
		if (stop) continue;
		double b_x = x * Constants::DISCRETE_STEP;
		double inc_D = 0;
		double inc_node_weight = 
			QoScommon::get_increase_weight(Constants::FUNCTION, current_budget, b_x);
		
		for (int i = 0; i < comp_budgets.size(); ++i) {
			if (b_x < comp_budgets[i]) // b_m is not sufficient to make this path exceed t_u
				inc_D += inc_node_weight;
			else
				inc_D += comp_lengths[i];
		}
		double invest = inc_D / b_x;
		if (invest >= M) {
			// end_x = no_discrete_steps + 1; // stop openmp
			#pragma omp critical
			{
				stop = true;
				if (b_x > re) // save the budget
					re = b_x;
			}
		}
	}

	return re;
}



