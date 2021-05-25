#pragma once

#include<vector>

#include "Constants.h"
#include "QoScommon.h"

using namespace std;
typedef unsigned int uint;
class Node
{
public:
	Node(const uint& id);
	~Node();
	void add_neighbor(uint const& node_id);
	vector<uint> get_neighbors();
	uint get_id();
	uint get_degree();

	// return the difference between new weight and current weight
	double increase_working_weight_by_budget(double const& inc_budget);
	const double get_working_budget();
	const double get_working_weight();
	const double get_weight();
	void reset_budget(); // reset during algorithm, working length = length
	void ultimate_reset(); // reset to start, budget = 0
	void save_budget();
	
private:
	uint id;
	vector<uint> neighbors;
	double weight; // current weight
	double budget; // current budget invested into the edge
	double working_weight; // working weight
	double working_budget; // working budget
};

