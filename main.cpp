#include<iostream>
#include <omp.h>
#include <time.h>
#include <fstream>
#include <stdlib.h>
#include <string>
#include "Network.h"
#include "Constants.h"
//#include <time.h>
#include <chrono>

#include "IterativeChecking.h"
#include "ThresholdExpansion.h"
#include "JumpStartGreedy.h"
#include "OptimalBlocking.h"
#include "IncrementalInterdiction.h"
#include "DiscreteGreedy.h"

#include <iomanip> // setprecision
#include <sstream> // stringstream
#if defined(_WIN32)
	#include<direct.h>
#else 
	#include <sys/stat.h>
	#include <sys/types.h>
#endif

using namespace std;

#pragma warning(disable: 4996) 

enum CHANGE {T, varepsilon};
enum MAIN_ALG {IC, II, Main_Nah};
enum PATH_BLOCK_ALG {TE, JSG, OPT, DG};

static const char * MainAlgString[] = {"IC", "II"};
static const char * PathBlockString[] = {"TE", "JSG", "OPT", "DG"};
static const char * FuncString[] = {"CONCAVE", "CONVEX", "LINEAR", "STEP"};
static const char * ChangeString[] = {"T", "VAREPSILON"};

struct Result {
	double budget = 0.0;
	uint no_queries = 0;
	uint max_path = 0;
	vector<uint> no_paths{};
	Result(double const& budget, 
		uint const& no_queries, vector<uint> const& no_paths, uint const& max_path)
		: budget(budget), no_queries(no_queries), no_paths(no_paths), max_path(max_path) {};
};

Result getResultAlgorithm(
	Network& g, const MAIN_ALG& main_alg, const PATH_BLOCK_ALG& path_block_alg) {
	PathBlockingAlgorithm * pbAlg;
	switch (path_block_alg)
	{
	case PATH_BLOCK_ALG::JSG:
		pbAlg = new JumpStartGreedy(g);
		break;
	case PATH_BLOCK_ALG::OPT:
		pbAlg = new OptimalBlocking(g);
		break;
	case PATH_BLOCK_ALG::DG:
		pbAlg = new DiscreteGreedy(g);
		break;
	default:
		pbAlg = new ThresholdExpansion(g);
		break;
	}

	MainAlgorithm * mainAlg;
	switch (main_alg)
	{
	case MAIN_ALG::IC:
		mainAlg = new IterativeChecking(g, *pbAlg);
		break;
	default:
		mainAlg = new IncrementalInterdiction(g, *pbAlg);
		break;
	}

	spdlog::info("start running alg {}-{}", 
		MainAlgString[main_alg], PathBlockString[path_block_alg]);

	double budget = mainAlg->get_solution();
	uint no_queries = mainAlg->get_no_queries();
	auto no_paths = mainAlg->get_no_paths();
	auto max_path = *max_element(no_paths.begin(), no_paths.end());

	spdlog::info("Finish running alg {}-{}",
		MainAlgString[main_alg], PathBlockString[path_block_alg]);
	spdlog::info("{}-{}: result {}, query {}",
		MainAlgString[main_alg], PathBlockString[path_block_alg], budget, no_queries);

	delete pbAlg;
	delete mainAlg;

	return Result(budget, no_queries, no_paths, max_path);
}

vector<Result> runAlgorithms(Network& g) {
	spdlog::info("T {}, varepsilon {}", Constants::T, Constants::VAREPSILON);
	vector<Result> re;
	for (int i = MAIN_ALG::IC; i != MAIN_ALG::Main_Nah; ++i) {
		MAIN_ALG main_alg = static_cast<MAIN_ALG>(i);
		for (int j = PATH_BLOCK_ALG::TE; j != PATH_BLOCK_ALG::OPT; ++j) {
			PATH_BLOCK_ALG path_alg = static_cast<PATH_BLOCK_ALG>(j);
			g.reset_budget();
			re.emplace_back(getResultAlgorithm(g, main_alg, path_alg));
		}
	}

	// run cut, IEEE TSG
	auto old_discrete_step = Constants::DISCRETE_STEP;
	Constants::DISCRETE_STEP = QoScommon::get_budget(
		Constants::FUNCTION, Constants::T);
	g.reset_budget();
	re.emplace_back(getResultAlgorithm(g, MAIN_ALG::II, PATH_BLOCK_ALG::JSG));
	Constants::DISCRETE_STEP = old_discrete_step;

	// run discrete greedy
	old_discrete_step = Constants::DISCRETE_STEP;
	if (Constants::FUNCTION != FUNC::step)
		Constants::DISCRETE_STEP = QoScommon::get_budget(
			Constants::FUNCTION, Constants::T) / 3.0;
	else
		Constants::DISCRETE_STEP = 1.0;
	g.reset_budget();
	re.emplace_back(getResultAlgorithm(g, MAIN_ALG::IC, PATH_BLOCK_ALG::DG));
	Constants::DISCRETE_STEP = old_discrete_step;

	// run opt if linear or step
	if (Constants::FUNCTION == FUNC::linear || Constants::FUNCTION == FUNC::step) {
		g.reset_budget();
		re.emplace_back(getResultAlgorithm(g, MAIN_ALG::IC, PATH_BLOCK_ALG::OPT));
	}

	return re;
}

string get2PrecisionString(const double& d) {
	stringstream tmp;
	tmp << fixed << setprecision(2) << d;
	return tmp.str();
}

vector<vector<uint>> rearrangePathResults(const vector<Result>& results) {
	uint max_size = 0;
	for (auto const& re : results) {
		if (re.no_paths.size() > max_size)
			max_size = re.no_paths.size();
	}
	vector<vector<uint>> res(max_size, vector<uint>(results.size(), 0));
	for (int i = 0; i < results.size(); ++i) {
		for (int j = 0; j < results[i].no_paths.size(); ++j) {
			res[j][i] = results[i].no_paths[j];
		}
	}
	return res;
}

void runExperiment(
	int no_nodes, string file, CHANGE change = T, FUNC func = convex, 
	double min = 20, double max = 50, double step = 2) {
	Constants::FUNCTION = func;

	long folder_prefix = time(NULL);
	string re_folder = "result/" + to_string(folder_prefix) + "_" + file + "_"
		+ "_" + FuncString[func] + "_" + ChangeString[change];

	spdlog::info("Run experiments with {} nodes, {} file, {} change, {} func",
		no_nodes, file, ChangeString[change], FuncString[func]);

#if defined(_WIN32)
	mkdir(re_folder.c_str());
#else
	mkdir(re_folder.c_str(), 0777); // notice that 777 is different than 0777
#endif

	ofstream writefile_query(re_folder + "/query.csv");
	ofstream writefile_sol(re_folder + "/solution.csv");
	ofstream writefile_path(re_folder + "/path.csv");

	if (!writefile_query.is_open() || !writefile_sol.is_open())
		throw "files cannot be written";

	string header = "T,var,IC-TE,IC-JSG,II-TE,II-JSG,CUT,DISCRETE,";
	string header_path = "round,IC-TE,IC-JSG,II-TE,II-JSG,CUT,DISCRETE,";
	if (func == FUNC::linear || func == FUNC::step) {
		header += "OPT,";
		header_path += "OPT,";
	}
		
	writefile_query << header << endl;
	writefile_sol << header << endl;
	writefile_path << header << endl;

	// lambda function to write results to files
	auto writeResultToFile = [&writefile_query, &writefile_sol, &writefile_path, &re_folder,
		&header_path, &change](vector<Result> const& results) {
		writefile_query << get2PrecisionString(Constants::T) << ","
			<< get2PrecisionString(Constants::VAREPSILON) << ",";
		writefile_sol << get2PrecisionString(Constants::T) << ","
			<< get2PrecisionString(Constants::VAREPSILON) << ",";
		writefile_path << get2PrecisionString(Constants::T) << ","
			<< get2PrecisionString(Constants::VAREPSILON) << ",";
		for (auto const& result : results) {
			writefile_query << result.no_queries << ",";
			writefile_sol << result.budget << ",";
			writefile_path << result.max_path << ",";
		}
		writefile_query << endl;
		writefile_sol << endl;
		writefile_path << endl;

		// write path result to file
		ofstream writefile_path(
			re_folder + "/path_" + 
			(change == T ? get2PrecisionString(Constants::T) 
				: get2PrecisionString(Constants::VAREPSILON)) 
			+ ".csv");
		writefile_path << header_path << endl;
		auto rearrangeNoPaths = rearrangePathResults(results);
		for (int i = 0; i < rearrangeNoPaths.size(); ++i) {
			writefile_path << i << ",";
			for (auto const& no_paths : rearrangeNoPaths[i]) {
				writefile_path << no_paths << ",";
			}
			writefile_path << endl;
		}
		writefile_path.close();
	};

	Network g;
	g.read_network_from_file(no_nodes, "data/" + file);

	if (change == CHANGE::T) {
		for (double i = min; i <= max; i += step) {
			if (func == convex)
				Constants::T = i * i;
			else
				Constants::T = i;
			writeResultToFile(runAlgorithms(g));
		}
	}
	else {
		for (Constants::VAREPSILON = min; 
			Constants::VAREPSILON <= max; 
			Constants::VAREPSILON += step) {
			writeResultToFile(runAlgorithms(g));
		}
	}
	writefile_query.close();
	writefile_sol.close();
}

int main() {
	omp_set_num_threads(Constants::NUM_THREAD);
	spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
	spdlog::info("Welcome to spdlog!");
	//runExperiment(8, "test.txt", T, convex, 50, 101, 5);
	Constants::VAREPSILON = 0.1;
	runExperiment(6474, "as.txt", T, convex, 50, 101, 5);
	runExperiment(6474, "as.txt", T, concave, 1, 10.5, 1.0);
	runExperiment(6474, "as.txt", T, linear, 50, 101, 5);
	runExperiment(6474, "as.txt", T, step, 50, 101, 5);
	Constants::T = 10000;
	runExperiment(6474, "as.txt", varepsilon, convex, 0.05, 0.505, 0.05);
	Constants::T = 10;
	runExperiment(6474, "as.txt", varepsilon, concave, 0.05, 0.505, 0.05);
	Constants::T = 100;
	runExperiment(6474, "as.txt", varepsilon, linear, 0.05, 0.505, 0.05);
	runExperiment(6474, "as.txt", varepsilon, step, 0.05, 0.505, 0.05);
	return 0;
}