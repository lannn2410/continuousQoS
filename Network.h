#pragma once
#include <vector>
#include <map>
#include "Node.h"
#include "QoScommon.h"

using namespace std;

struct TreeNode {
	uint node_id = 0;
	uint parent_id;
	vector<uint> children_ids{};
	double distance = 0.0;
	TreeNode() {};
	TreeNode(uint const& node_id, uint const& parent_id, double const& distance) :
		node_id(node_id), parent_id(parent_id), distance(distance) {};
};

class Network
{
public:
	Network();
	~Network();
	const vector<pair<uint, uint>> get_list_txs();
	const uint get_no_nodes();
	const double get_node_current_budget(uint const& node_id);
	const double get_path_working_length(vector<uint> const& path);
	void generate_txs();
	void read_network_from_file(int no_nodes, string file);
	
	const vector<vector<uint>> get_shortest_paths(uint const& s_id, uint const& e_id);
	
	// return the increasing new node weight
	double increase_node_working_weight_by_budget(
		uint const& node_id, double const& inc_budget);
	
	void reset_budget(); // used during algorithm, make working length = length
	void save_budget();
	void clear();

private:
	const map<uint, TreeNode> get_shortest_path_tree(uint const& root);
	uint number_of_nodes;
	map<uint, uint> map_node_id; // map from true id -> ordered id (used for read graph from file)
	vector<Node> list_nodes; // id -> Node
	vector<int> node_idx; // used for heap
	vector<pair<uint, uint>> list_txs; // list of transactions
};

