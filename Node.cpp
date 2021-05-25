#include "Node.h"

Node::Node(const uint& id) : 
	id(id), budget(0), working_budget(0)
{
	weight = QoScommon::get_weight(Constants::FUNCTION, 0.0);
	working_weight = weight;
}

Node::~Node()
{
}

void Node::add_neighbor(uint const & node_id)
{
	neighbors.emplace_back(node_id);
}

vector<uint> Node::get_neighbors()
{
	return neighbors;
}

uint Node::get_id()
{
	return id;
}

uint Node::get_degree()
{
	return neighbors.size();
}

double Node::increase_working_weight_by_budget(double const & inc_budget)
{
	working_budget += inc_budget;
	double old_working_weight = working_weight;
	working_weight = QoScommon::get_weight(Constants::FUNCTION, working_budget);
	return working_weight - old_working_weight;
}

const double Node::get_working_budget()
{
	return working_budget;
}

const double Node::get_working_weight()
{
	return working_weight;
}

const double Node::get_weight()
{
	return weight;
}

void Node::reset_budget()
{
	working_budget = budget;
	working_weight = weight;
}

void Node::ultimate_reset()
{
	budget = 0.0;
	working_budget = 0.0;
	weight = QoScommon::get_weight(Constants::FUNCTION, 0.0);
	working_weight = weight;
}

void Node::save_budget()
{
	budget = working_budget;
	weight = working_weight;
}
