#include "MainAlgorithm.h"
#include <sstream> // stringstream


MainAlgorithm::MainAlgorithm(Network & g, PathBlockingAlgorithm & alg)
	: g(g), path_blocking_alg(alg)
{
}

uint MainAlgorithm::get_no_queries()
{
	return no_queries;
}

vector<uint> MainAlgorithm::get_no_paths()
{
	return no_paths;
}
