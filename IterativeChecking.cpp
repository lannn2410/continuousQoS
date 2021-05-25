#include "IterativeChecking.h"
#include <numeric>
#include<iostream>
#include <time.h>

using namespace spdlog;

IterativeChecking::IterativeChecking(Network & g, PathBlockingAlgorithm & alg)
	: MainAlgorithm(g, alg)
{
}

IterativeChecking::~IterativeChecking()
{
}

double IterativeChecking::get_solution()
{
	no_queries = 0;
	no_paths.clear();
	vector<double> v;
	info("Start IC");
	while (!is_blocked()) {
		// if haven't blocked all txs, reset weight/budget of nodes
		g.reset_budget();
		no_paths.emplace_back(potential_paths.size());
		info("Start blocking {} paths in IC", potential_paths.size());
		v = path_blocking_alg.block(potential_paths);
		info("Finish blocking {} paths in IC", potential_paths.size());
		no_queries += path_blocking_alg.get_last_block_queries();
	}
	info("End IC");
	return accumulate(v.begin(), v.end(), 0.0);
}


bool IterativeChecking::is_blocked()
{
	auto const list_txs = g.get_list_txs();
	uint tmp = potential_paths.size();

	#pragma omp parallel for
	for (int i = 0; i < list_txs.size(); ++i) {
		uint s_id = list_txs[i].first;
		uint e_id = list_txs[i].second;
		vector<vector<uint>> shortest_paths = g.get_shortest_paths(s_id, e_id);
		info("Get {} new paths from {} to {}", shortest_paths.size(), s_id, e_id);
		#pragma omp critical
		{
			potential_paths.insert(
				potential_paths.end(), shortest_paths.begin(), shortest_paths.end());
		}
	}

	info("Number of paths: {}", potential_paths.size());
	return tmp == potential_paths.size();
}
