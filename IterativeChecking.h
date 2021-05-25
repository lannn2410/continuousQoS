#pragma once
#include "MainAlgorithm.h"
class IterativeChecking : public MainAlgorithm
{
public:
	IterativeChecking(Network& g, PathBlockingAlgorithm& alg);
	~IterativeChecking();
	double get_solution();
private:
	bool is_blocked(); // check whether all transactions are blocked, if not, adding paths whose length < T into potential paths set
	vector<vector<uint>> potential_paths;
};

