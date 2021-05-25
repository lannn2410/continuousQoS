#include "IncrementalInterdiction.h"
#include <numeric>

using namespace spdlog;

IncrementalInterdiction::IncrementalInterdiction(Network & g, PathBlockingAlgorithm & alg)
	:MainAlgorithm(g, alg)
{
}

IncrementalInterdiction::~IncrementalInterdiction()
{
}

double IncrementalInterdiction::get_solution()
{
	double re = 0.0;
	no_queries = 0;
	no_paths.clear();
	info("Start II");
	while (!is_blocked()) {
		info("Start blocking {} paths in II", potential_paths.size());
		auto v = path_blocking_alg.block(potential_paths);
		info("Finish blocking {} paths in II", potential_paths.size());
		re += accumulate(v.begin(), v.end(), 0.0);
		no_queries += path_blocking_alg.get_last_block_queries();
		no_paths.emplace_back(potential_paths.size());
	}
	info("End II");
	return re;
}

bool IncrementalInterdiction::is_blocked()
{
	auto const list_txs = g.get_list_txs();
	potential_paths.clear();

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
	return potential_paths.size() == 0;
}
