#include "PathBlockingAlgorithm.h"
#include <algorithm>
#include <cfloat>


PathBlockingAlgorithm::PathBlockingAlgorithm(Network& g) : g(g)
{

}

PathBlockingAlgorithm::~PathBlockingAlgorithm()
{
}

uint PathBlockingAlgorithm::get_last_block_queries()
{
	return last_block_queries;
}

const double
PathBlockingAlgorithm::estimate_opt()
{
	double ep_l = 0;
	double ep_u = DBL_MAX;
	double ep_m = (ep_l + ep_u) / 2.0;
	const uint no_nodes = g.get_no_nodes();
	double current_D = 0;

	for (auto const& path : list_paths) {
		current_D += min(g.get_path_working_length(path), Constants::T);
	}

	while (ep_u - 0.0001 > ep_l) {
		// create a copy since multiple nodes can change a path length
		auto tmp_path_length = path_lengths; 
		double D = current_D;
		for (uint n = 0; n < no_nodes; ++n) {
			const vector<uint>& inv_path_ids = involved_paths[n];
			double inc_len = QoScommon::get_increase_weight(
				Constants::FUNCTION, g.get_node_current_budget(n), ep_m);
			for (auto const& path_id : inv_path_ids) {
				double current_len = tmp_path_length[path_id];
				tmp_path_length[path_id] += inc_len;
				if (current_len >= Constants::T)
					continue;
				double tmp = min(tmp_path_length[path_id], Constants::T);
				D += (tmp - current_len);
			}
		}

		if (D < (list_paths.size() * Constants::T - 0.0001)) {
			ep_l = ep_m;
		}
		else {
			ep_u = ep_m;
		}

		ep_m = (ep_u + ep_l) / 2.0;
	}
	return ep_m;
}

uint PathBlockingAlgorithm::initiate()
{
	uint no_done_path = 0;
	involved_paths.clear();
	path_lengths.clear();
	const uint no_nodes = g.get_no_nodes();
	involved_paths = vector<vector<uint>>(no_nodes, vector<uint>());
	for (int i = 0; i < list_paths.size(); ++i) {
		double path_len = g.get_path_working_length(list_paths[i]);
		if (path_len > Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION) {
			++no_done_path;
		}
		path_lengths.emplace_back(path_len);
		vector<uint> inv_nodes(
			list_paths[i].begin() + 1, list_paths[i].end() - 1); // ignore start and end node
		for (auto const& node_id : inv_nodes) {
			involved_paths[node_id].emplace_back(i);
		}
	}
	return no_done_path;
}

uint PathBlockingAlgorithm::update_node_and_path_length(uint const & node_id, double const & inc_budget)
{
	double inc_len = g.increase_node_working_weight_by_budget(node_id, inc_budget); // increase length of edge and return increase length
	uint no_new_done_paths = 0;
	auto const& inv_path_ids = involved_paths[node_id];
	for (auto const& inv_path_id : inv_path_ids) {
		double old_len = path_lengths[inv_path_id];
		path_lengths[inv_path_id] = old_len + inc_len;
		if (old_len > Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION)
			continue;
		if (path_lengths[inv_path_id] > 
			Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION)
			++no_new_done_paths;
	}
	return no_new_done_paths;
}
