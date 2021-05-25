#pragma once
#include "PathBlockingAlgorithm.h"
#include <map>
class DiscreteGreedy : public PathBlockingAlgorithm
{
public:
	DiscreteGreedy(Network& g);
	vector<double> block(vector<vector<uint>> const& list_paths);

private:
	// return pair: 
	// (2) invest = increase in path length / DISCRETE STEP
	// (3) does this node need to queried. No if no involving paths
	pair<double, bool> get_invest(uint const& node_id);
};

