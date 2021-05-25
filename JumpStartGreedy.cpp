#include "JumpStartGreedy.h"
#include <algorithm>
#include <math.h>
#include "HeapData.hpp"
#include "mappedheap.hpp"

using namespace spdlog;

JumpStartGreedy::JumpStartGreedy(Network& g) : PathBlockingAlgorithm(g)
{
}

vector<double> JumpStartGreedy::block(vector<vector<uint>> const & list_paths)
{
	last_block_queries = 0;
	this->list_paths = list_paths;
	const int no_nodes = g.get_no_nodes();
	vector<double> x(no_nodes, 0); // returned result
	uint no_done_paths = initiate();
	info("No. input paths for JSG", list_paths.size());
	info("No. done paths: {}", no_done_paths);
	double jump_step = Constants::ALPHA_JSG * estimate_opt() / no_nodes;
	info("Jump step {}", jump_step);

	while (no_done_paths < list_paths.size()) {
		uint best_node_id = no_nodes;
		double best_inc_budget = 0.0, best_invest = 0.0;

		#pragma omp parallel for
		for (int n = 0; n < no_nodes; ++n) {
			auto vals_tuple = get_best_increase_budget(n, jump_step);
			auto& node_invest = std::get<1>(vals_tuple);
			bool is_queried = std::get<2>(vals_tuple);
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
						best_inc_budget = std::get<0>(vals_tuple);
					}
				}
			}
		}

		

		if (best_inc_budget == 0.0) {
			info("JSG cannot find solution");
			throw "Error here, cannot find solution";
		}
			
		info("Add {} budget to node {}", best_inc_budget, best_node_id);
		x[best_node_id] += best_inc_budget;
		// update edge and path working length and return no. new exceed paths
		uint no_new_done_paths = update_node_and_path_length(
			best_node_id, best_inc_budget);
		no_done_paths += no_new_done_paths;
		//info("No. done paths: {}", no_done_paths);
	}

	return x;
}

tuple<double, double, bool> JumpStartGreedy::get_best_increase_budget(
	uint const & node_id, double const & jump_step)
{	
	vector<double> sorted_path_lens;
	for (auto const& inv_path_id : involved_paths[node_id]) {
		if (path_lengths[inv_path_id] <
			Constants::T * (1 - Constants::VAREPSILON) - Constants::PRECISION)
			sorted_path_lens.push_back(path_lengths[inv_path_id]);
	}

	if (sorted_path_lens.size() == 0)
		return make_tuple(0.0, 0.0, false);

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
	
	// trick for concave case
	if (Constants::FUNCTION == FUNC::concave) {
		no_discrete_steps = 1;
	}

	double best_budget = 0.0, best_invest = 0.0;
	double best_if_jump_budget = 0.0, best_if_jump_invest = 0.0;
	for (int x = no_discrete_steps; x > 0; --x) {
		double b_x = x * Constants::DISCRETE_STEP;
		
		// trick for concave case
		if (Constants::FUNCTION == FUNC::concave) {
			b_x = jump_step > Constants::DISCRETE_STEP 
				? jump_step : Constants::DISCRETE_STEP;
		}
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
		if (invest > best_invest) {
			best_invest = invest;
			best_budget = b_x;
		}
		if (invest > best_if_jump_invest && b_x >= jump_step) {
			best_if_jump_budget = b_x;
			best_if_jump_invest = invest;
		}
	}

	// in case of concave?
	if (best_invest == Constants::DISCRETE_STEP)
		return make_tuple(best_if_jump_budget, best_if_jump_invest, true);

	return make_tuple(best_budget, best_invest, true);
}

