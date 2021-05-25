#pragma once
#include "MainAlgorithm.h"
class IncrementalInterdiction : public MainAlgorithm
{
public:
	IncrementalInterdiction(Network& g, PathBlockingAlgorithm& alg);
	~IncrementalInterdiction();
	double get_solution();
private:
	bool is_blocked(); // check whether all transactions are blocked, if not, adding paths whose length < T into potential paths set
	vector<vector<uint>> potential_paths;
};

