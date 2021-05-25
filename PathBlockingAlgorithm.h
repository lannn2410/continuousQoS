#pragma once
#include "Network.h"

class PathBlockingAlgorithm
{
public:
	PathBlockingAlgorithm(Network& g);
	~PathBlockingAlgorithm();

	virtual vector<double> block(vector<vector<uint>> const& list_paths) = 0;
	uint get_last_block_queries();
protected:
	const double estimate_opt();
	uint initiate(); // initiate for algorithms, filter exceed path and return number of exceed paths
	uint update_node_and_path_length(uint const& node_id, double const& inc_budget);
	Network& g;
	vector<vector<uint>> list_paths;
	vector<vector<uint>> involved_paths; // edge_id -> path index that contains the edge
	vector<double> path_lengths;
	uint last_block_queries; // count # function queries the last time the block function() is called
};

