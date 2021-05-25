#include "QoScommon.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <cstring>
#include <math.h>
#include <float.h>

using namespace std;

QoScommon * QoScommon::instance = nullptr;

QoScommon::QoScommon()
{
	seed = new int[10000];
	for (int i = 0; i < 10000; i++) {
		seed[i] = rand();
	}
}

QoScommon::~QoScommon()
{
}

QoScommon * QoScommon::getInstance()
{
	if (!instance)
		instance = new QoScommon();
	return instance;
}

unsigned QoScommon::randomInThread(int thread_id)
{
	unsigned tmp = seed[thread_id % 10000];
	tmp = tmp * 17931 + 7391;
	seed[thread_id % 10000] = tmp;
	return tmp;
}

// convex: y = x^2 + 1
// concave: y = log (x+1)
// linear: y = x + 1
// step: y = floor(x) + 1
double QoScommon::get_weight(const FUNC & func, const double & budget)
{
	switch (func) {
	case FUNC::concave:
		return log(budget + 1.0);
	case FUNC::linear:
		return budget + 1.0;
	case FUNC::step:
		return floor(budget) + 1.0;
	default:
		return budget * budget + 1.0;
	}
	
	return 0.0;
}

double QoScommon::get_budget(const FUNC & func, const double & len)
{
	switch (func)
	{
	case FUNC::concave:
		return exp(len) - 1.0;
	case FUNC::linear:
		return len - 1.0;
	case FUNC::step:
		return ceil(len - 1.0);
	default:
		return sqrt(len - 1.0);
	}
	return 0.0;
}

double QoScommon::get_complement_budget(
	const FUNC & func, const double & budget, const double & comp_len)
{
	double weight = get_weight(func, budget);
	double expected_weight = weight + comp_len;
	double expected_budget = get_budget(func, expected_weight);
	return expected_budget - budget;
}

double QoScommon::get_increase_weight(FUNC func, double budget, double inc_budget)
{
	return get_weight(func, budget + inc_budget) - get_weight(func, budget);
}

double QoScommon::getM(const FUNC & func, const double & up)
{
	switch (func)
	{
	case FUNC::concave:
	case FUNC::linear:
		return 1.0;
	case FUNC::step:
		return 1000000.0;
	default:
		auto max_x = get_budget(func, up);
		return 2.0 * max_x;
	}
	return 0.0;
}

string QoScommon::getPathString(const vector<uint>& path)
{
	if (path.size() == 0)
		return "";
	string s = to_string(path[0]);
	for (int i = 1; i < path.size(); ++i){
		s += " - " + to_string(path[i]);
	}
	return s;
}
