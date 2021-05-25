#pragma once
#include "PathBlockingAlgorithm.h"
#include <chrono>

using namespace chrono;

class MainAlgorithm
{
public:
	MainAlgorithm(Network& g, PathBlockingAlgorithm& alg);
	virtual double get_solution() = 0;
	uint get_no_queries();
	vector<uint> get_no_paths();
protected:
	PathBlockingAlgorithm& path_blocking_alg;
	Network& g;
	uint no_queries;
	vector<uint> no_paths;
};

