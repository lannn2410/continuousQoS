#pragma once
#include <omp.h>
#include <vector>
#include <string>
#include "Constants.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

using namespace std;
typedef unsigned int uint;

class QoScommon
{
public:
	QoScommon();
	~QoScommon();

	static QoScommon * getInstance();
	unsigned randomInThread(int thread_id);

	static double get_budget(const FUNC& func, const double& len);

	static double get_weight(const FUNC& func, const double& budget);

	// get budget needed to make node weight increase by comp_len
	static double get_complement_budget(
		const FUNC& func, const double& budget, const double& comp_len);

	// get increase of length with inc_budget consider the current budget
	static double get_increase_weight(FUNC func, double budget, double inc_budget);
	
	static double getM(const FUNC& func, const double& up); // used for threshold expansion

	static string getPathString(const vector<uint>& path);
private:
	static QoScommon * instance;
	int * seed;
};

struct SamplePath {
	int length;
	double prob;
	vector<int> path;
};

