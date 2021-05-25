#pragma once
#include "PathBlockingAlgorithm.h"

class OptimalBlocking : public PathBlockingAlgorithm
{
public:
	OptimalBlocking(Network& g);
	vector<double> block(vector<vector<uint>> const& list_paths);
};

