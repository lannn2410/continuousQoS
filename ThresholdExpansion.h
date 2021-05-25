#pragma once
#include "PathBlockingAlgorithm.h"
class ThresholdExpansion : public PathBlockingAlgorithm
{
public:
	ThresholdExpansion(Network& g);
	vector<double> block(vector<vector<uint>> const& list_paths);
private:
	double get_increasing_budget(
		uint const& node_id, const double& M);
};

