#pragma once
#include "PathBlockingAlgorithm.h"
#include <map>
class JumpStartGreedy : public PathBlockingAlgorithm
{
public:
	JumpStartGreedy(Network& g);
	vector<double> block(vector<vector<uint>> const& list_paths);

private:
	// return tuple: 
	// (1) best budget
	// (2) best invest = best budget / increase in path length
	// (3) does this node need to queried to find budget 
	tuple<double, double, bool> get_best_increase_budget(
		uint const& node_id, double const& jump_step);
};

