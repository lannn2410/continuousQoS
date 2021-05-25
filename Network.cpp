#include "Network.h"
#include <iostream>
#include <fstream>
#include "Constants.h"
#include "QoScommon.h"
#include "HeapData.hpp"
#include "mappedheap.hpp"
#include <queue>
#include <set>

using namespace spdlog;

Network::Network()
{
}


Network::~Network()
{

}


const vector<pair<uint, uint>> Network::get_list_txs()
{
	return list_txs;
}

const uint Network::get_no_nodes()
{
	return number_of_nodes;
}

const double Network::get_node_current_budget(uint const & node_id)
{
	return list_nodes[node_id].get_working_budget();
}

const double Network::get_path_working_length(vector<uint> const & path)
{
	double length = 0.0;
	for (auto const& node_id : path) {
		length += list_nodes[node_id].get_working_weight();
	}
	return length;
}

void Network::generate_txs()
{
	list_txs.clear();
	vector<set<uint>> txs(number_of_nodes);
	vector<uint> leaf_nodes; // store node with degree 1
	for (auto& node : list_nodes) {
		if (node.get_degree() == 1) {
			leaf_nodes.emplace_back(node.get_id());
		}
	}
	if (leaf_nodes.size() < 2)
		return;
	for (int i = 0; i < Constants::NUMBER_OF_TRANSACTIONS; ++i) {
		uint s_idx = rand() % leaf_nodes.size();
		uint s_id = leaf_nodes[s_idx];
		uint rest_span = leaf_nodes.size() - s_idx - 1;
		if (rest_span < 1) continue;
		uint e_idx = rand() % rest_span + s_idx + 1;
		uint e_id = leaf_nodes[e_idx];
		if (txs[s_id].find(e_id) == txs[s_id].end()) {
			txs[s_id].emplace(e_id);
			list_txs.emplace_back(make_pair(s_id, e_id));
		}
	}
}

void Network::read_network_from_file(int no_nodes, string file)
{
	clear();
	number_of_nodes = no_nodes;
	for (uint i = 0; i < number_of_nodes; ++i) {
		node_idx.push_back(i);
	}
	ifstream is(file);
	is.seekg(0, is.end);
	long bufSize = is.tellg();
	is.seekg(0, is.beg);
	int item = 0;

	char * buffer = new char[bufSize];

	is.read(buffer, bufSize);
	is.close();

	std::string::size_type sz = 0;
	long sp = 0;
	uint start_id, end_id;
	bool is_start = true;
	uint id = 0;
	uint s_id, e_id; // used to stored ordered id of startId and endId
	uint edge_id = 0;

	while (sp < bufSize) {
		char c = buffer[sp];
		item = item * 10 + c - 48;
		sp++;
		if (sp == bufSize || (sp < bufSize && (buffer[sp] < 48 || buffer[sp] > 57))) {
			while (sp < bufSize && (buffer[sp] < 48 || buffer[sp] > 57))
				sp++;

			if (is_start) {
				start_id = item;
				is_start = false;
			}
			else {
				end_id = item;
				is_start = true;

				if (start_id != end_id) {
					auto const s_id_it = map_node_id.find(start_id);
					if (s_id_it == map_node_id.end()) {
						map_node_id[start_id] = id;
						Node s_node = Node(id);
						list_nodes.push_back(s_node);
						s_id = id;
						id++;
					}
					else {
						s_id = s_id_it->second;
					}

					auto const e_id_it = map_node_id.find(end_id);
					if (e_id_it == map_node_id.end()) {
						map_node_id[end_id] = id;
						Node e_node = Node(id);
						list_nodes.push_back(e_node);
						e_id = id;
						id++;
					}
					else {
						e_id = e_id_it->second;
					}

					list_nodes[s_id].add_neighbor(e_id);
					list_nodes[e_id].add_neighbor(s_id);
				}
			}
			item = 0;
		}
	}

	generate_txs();
	spdlog::info("Finish reading graph of {} file, {} nodes", file, no_nodes);
	spdlog::info("Generate {} txs", list_txs.size());
	delete[] buffer;
}

const vector<vector<uint>>
Network::get_shortest_paths(
	uint const& s_id, uint const& e_id)
{
	info("Start calculating shortest path tree rooted at {}", e_id);
	// first construct shortest path rooted at e_id
	auto shortest_path_tree = get_shortest_path_tree(e_id);
	info("Finish calculating shortest path tree rooted at {}", e_id);

	auto const s_id_it = shortest_path_tree.find(s_id);
	if (s_id_it == shortest_path_tree.end()) {
		info("No path from {} to {}", s_id, e_id);
		return vector<vector<uint>>{};
	}
	
	// store the result
	vector<vector<uint>> ksp;

	// add shortest path to ksp
	vector<uint> shortest_path{ s_id };
	auto tmp = s_id;
	while (tmp != e_id) {
		auto const tmp_it = shortest_path_tree.find(tmp);
		if (tmp_it == shortest_path_tree.end()) {
			info("Error in constructing shortest path tree");
			throw "Error here";
		}
		tmp = tmp_it->second.parent_id;
		shortest_path.emplace_back(tmp);
	}

	auto sp_len = get_path_working_length(shortest_path);
	if (sp_len > Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION) {
		info("No path from {} to {}", s_id, e_id);
		return vector<vector<uint>>{};
	}

	ksp.emplace_back(shortest_path);

	// store length of paths
	vector<double> path_lengths;

	// store potential paths
	vector<vector<uint>> potential_paths;
	set<vector<uint>> potential_path_set; // the same as potential_paths but in set

	// potential path idx, used for heap
	vector<int> potential_path_idx;

	AscendingOrder<double> potential_path_hd(&path_lengths);
	MappedHeap<AscendingOrder<double>> potential_path_heap(
		potential_path_idx, potential_path_hd);

	while (ksp.size() < Constants::KSP) {
		auto const& newest_path = ksp.back();
		#pragma omp parallel for
		for (int i = 0; i < newest_path.size() - 1; ++i) {
			auto const& spur_node = newest_path[i];
			vector<bool> exclude_neighbor(number_of_nodes, false);
			vector<uint> root_path(newest_path.begin(), newest_path.begin() + i + 1);
			// Remove the links that are part of the previous shortest paths 
			// which share the same root path.
			for (auto const& p : ksp) {
				if (p.size() < i + 1) continue;
				if (root_path == vector<uint>(p.begin(), p.begin() + i + 1)) {
					exclude_neighbor[p[i + 1]] = true;
				}
			}
			vector<bool> exclude_node(number_of_nodes, false);
			queue<uint> q; // queue for removing exclude node from tree
			for (auto const& node : root_path) {
				exclude_node[node] = true;
				q.push(node);
			}

			// find shortest path from spurnode to e_id 
			// with exclude_neighbors and exclude nodes
			
			// remove exclude nodes from trees
			auto new_tree = shortest_path_tree;		
			while (!q.empty()) {
				uint id = q.front();
				q.pop();
				auto const id_it = new_tree.find(id);
				if (id_it == new_tree.end())
					continue;
				auto& children_ids = id_it->second.children_ids;
				for (auto const& children_id : children_ids) {
					q.push(children_id);
				}
				new_tree.erase(id);
			}

			// start from spur_node to find a new shortest path to e_id
			vector<double> distance(number_of_nodes, Constants::T);
			distance[spur_node] = 0.0;
			AscendingOrder<double> hd(&distance);
			MappedHeap<AscendingOrder<double>> heap(node_idx, hd);
			vector<uint> parent(number_of_nodes, number_of_nodes);
			vector<bool> out_of_heap(number_of_nodes, false);
			uint touch = number_of_nodes;
			while (!heap.empty()) {
				uint id = heap.pop();
				out_of_heap[id] = true;
				if (distance[id] > 
					Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION || id == e_id)
					break;
			
				auto const id_it = new_tree.find(id);
				if (id_it != new_tree.end()) {
					// if this node exists in shortest path tree to e_id, 
					// update distance to e_id
					double tmp = distance[id] + id_it->second.distance;
					if (tmp < distance[e_id]) {
						distance[e_id] = tmp;
						touch = id;
						heap.heapify(e_id);
					}
					continue;
				}
			
				for (auto const& neighbor_id : list_nodes[id].get_neighbors()) {
					if (id == spur_node && exclude_neighbor[neighbor_id])
						continue;
					if (exclude_node[neighbor_id])
						continue;
					if (out_of_heap[neighbor_id])
						continue;
			
					double tmp = distance[id] 
						+ list_nodes[neighbor_id].get_working_weight();
					if (tmp >= distance[neighbor_id])
						continue;
					distance[neighbor_id] = tmp;
					parent[neighbor_id] = id;
					heap.heapify(neighbor_id);
				}
			}

			if (distance[e_id] > 
				Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION) {
				//info("No new spur path is found between {} and {}", spur_node, e_id);
				continue;
			}

			// construct new full path
			auto total_path = root_path;
			auto tmp = touch;
			vector<uint> touch_to_spur;
			while (tmp != spur_node) {
				touch_to_spur.push_back(tmp);
				tmp = parent[tmp];
			}
			std::reverse(touch_to_spur.begin(), touch_to_spur.end());
			total_path.insert(
				total_path.end(), touch_to_spur.begin(), touch_to_spur.end());
			tmp = touch;
			while (tmp != e_id) {
				auto const tmp_it = new_tree.find(tmp);
				if (tmp_it == new_tree.end()) {
					throw "error here";
				}
				tmp = tmp_it->second.parent_id;
				total_path.push_back(tmp);
			}

			auto total_path_length = get_path_working_length(total_path);
			if (total_path_length > 
				Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION) {
				continue;
			}


			#pragma omp critical
			{
				if (potential_path_set.find(total_path) == potential_path_set.end()) {
					potential_paths.push_back(total_path);
					potential_path_set.insert(total_path);
					path_lengths.push_back(total_path_length);
					auto new_key = potential_path_idx.size();
					potential_path_idx.push_back(new_key);
					potential_path_heap.push(new_key);
				}
			}
			
		}

		if (potential_path_heap.empty())
			break;

		auto new_p_id = potential_path_heap.pop();
		info("New path from {} to {}: {}", 
			s_id, e_id, QoScommon::getPathString(potential_paths[new_p_id]));
		ksp.push_back(potential_paths[new_p_id]);
	}

	return ksp;
}

//const vector<vector<uint>>
//Network::get_shortest_paths(
//	uint const& s_id, uint const& e_id)
//{
//	info("Start calculating shortest path tree rooted at {}", e_id);
//	// first construct shortest path rooted at e_id
//	auto shortest_path_tree = get_shortest_path_tree(e_id);
//	info("Finish calculating shortest path tree rooted at {}", e_id);
//
//	auto const s_id_it = shortest_path_tree.find(s_id);
//	if (s_id_it == shortest_path_tree.end()) {
//		info("No path from {} to {}", s_id, e_id);
//		return vector<vector<uint>>{};
//	}
//
//	// store expected length of potential paths
//	// expected length = path length + length from tail node to end_id
//	vector<double> potential_lengths;
//	
//	// current length of potential path
//	vector<double> current_lengths;
//
//	// store potential paths
//	vector<vector<uint>> potential_paths;
//	
//	// potential path idx, used for heap
//	vector<int> potential_path_idx;
//
//	// initiate first partial path
//	potential_lengths.emplace_back(s_id_it->second.distance);
//	current_lengths.emplace_back(0.0);
//	potential_paths.emplace_back(vector<uint>{s_id});
//	potential_path_idx.emplace_back(0);
//
//	AscendingOrder<double> hd(&potential_lengths);
//	MappedHeap<AscendingOrder<double>> heap(potential_path_idx, hd);
//
//	// set of ksps
//	set<vector<uint>> ksp;
//
//	while (ksp.size() < Constants::KSP && !heap.empty()) {
//		auto potential_path_id = heap.pop();
//		auto const path = potential_paths[potential_path_id];
//		// keep track which nodes are in path
//		vector<bool> is_in_path(number_of_nodes, false);
//		for (auto const& node_id : path) is_in_path[node_id] = true;
//
//		auto const current_length = current_lengths[potential_path_id];
//
//		auto const tail_node_id = path.back();
//		auto const tail_it = shortest_path_tree.find(tail_node_id);
//		// error here, the alg should guarantee this not happen
//		if (tail_it == shortest_path_tree.end()) {
//			info("Error in finding ksp (1)");
//			throw "Error here";
//		}
//			
//		auto const distance_tail_to_e_id = tail_it->second.distance;
//		// also error here, the alg should guarantee this not happen
//		if (current_length + distance_tail_to_e_id
//			>= Constants::T * (1 - Constants::VAREPSILON)) {
//			info("Error in finding ksp (2)");
//			throw "Error here";
//		}
//
//		// get full path
//		auto tmp = tail_node_id;
//		bool is_simple_path = true; // check if new-formed path is simple
//		auto full_path = path;
//		while (tmp != e_id) {
//			auto const tmp_it = shortest_path_tree.find(tmp);
//			// also error here, the alg should guarantee this not happen
//			if (tmp_it == shortest_path_tree.end()) {
//				info("Error in finding ksp (3)");
//				throw "Error here";
//			}
//			tmp = tmp_it->second.parent_id;
//			// if cannot form a simple path, break
//			if (is_in_path[tmp]) {
//				is_simple_path = false;
//				break;
//			}
//			full_path.emplace_back(tmp);
//		}
//
//		// add to ksp if this is a new path
//		if (is_simple_path && ksp.find(full_path) == ksp.end()) {
//			info("Add new full path {}", QoScommon::getPathString(full_path));
//			ksp.insert(full_path);
//		}
//
//		// extend this path
//		for (auto const& neighbor_id : list_nodes[tail_node_id].get_neighbors()) {
//			if (is_in_path[neighbor_id])
//				continue;
//			auto const nei_it = shortest_path_tree.find(neighbor_id);
//			if (nei_it == shortest_path_tree.end())
//				continue;
//			auto expected_length = nei_it->second.distance 
//				+ current_length + list_nodes[neighbor_id].get_working_weight();
//			if (expected_length >= Constants::T * (1 - Constants::VAREPSILON))
//				continue;
//			auto new_path = path;
//			new_path.push_back(neighbor_id);
//			auto new_path_length = 
//				current_length + list_nodes[neighbor_id].get_working_weight();
//
//			/*info("Add new partial path {}, length {}, expected length {}",
//				QoScommon::getPathString(new_path), new_path_length, expected_length);*/
//
//			// add new path to heap
//			potential_paths.push_back(new_path);
//			potential_lengths.push_back(expected_length);
//			current_lengths.push_back(new_path_length);
//			auto new_key = potential_path_idx.size();
//			potential_path_idx.push_back(new_key);
//			heap.push(new_key);
//		}
//
//	}
//	// return vector instead of set
//	vector<vector<uint>> re(ksp.size());
//	std::copy(ksp.begin(), ksp.end(), re.begin());
//	info("Add new {} paths for {} to {}", ksp.size(), s_id, e_id);
//	return re;
//}

//const vector<vector<uint>> 
//Network::get_shortest_paths(
//	uint const& s_id, uint const& e_id)
//{
//	vector<vector<uint>> re;
//
//	info("Start calculating shortest path tree rooted at {}", e_id);
//	// first construct shortest path rooted at e_id
//	auto shortest_path_tree = get_shortest_path_tree(e_id);
//	info("Finish calculating shortest path tree rooted at {}", e_id);
//
//	auto const s_id_it = shortest_path_tree.find(s_id);
//	if (s_id_it == shortest_path_tree.end()) {
//		info("No path from {} to {}", s_id, e_id);
//		return re;
//	}
//
//	// add shortest path into returned solution
//	vector<uint> shortest_path{};
//	shortest_path.emplace_back(s_id);
//	uint tmp = s_id;
//	while (tmp != e_id) {
//		auto const tmp_it = shortest_path_tree.find(tmp);
//		if (tmp_it == shortest_path_tree.end()) {
//			throw "Error!";
//		}
//		tmp = tmp_it->second.parent_id;
//		shortest_path.emplace_back(tmp);
//	}
//
//	// store node to delete to find other shortest paths
//	vector<bool> excluded_node(number_of_nodes, false);
//
//	while (true) {
//		re.emplace_back(shortest_path);
//		info("Found new path {}", QoScommon::getPathString(shortest_path));
//		if (shortest_path.size() <= 2) {
//			info("Error: New path {} has only 2 hops, throw exception",
//				QoScommon::getPathString(shortest_path));
//			throw "shortest path has fewer than 2 hops";
//		}
//		// random select a node from the shortest path
//		// random remove an edge out of graph to recalculate shortest paths
//		int idx =
//			QoScommon::getInstance()->randomInThread(omp_get_thread_num())
//			% (shortest_path.size() - 2); // -2 is to not consider s_id and e_id
//		uint rmv_id = shortest_path[idx + 1];
//		info("Exclude node {} when find path from {} to {}", rmv_id, s_id, e_id);
//		excluded_node[rmv_id] = true;
//
//		// delete the nodes in tree related to rmv_id
//		queue<uint> q;
//		q.push(rmv_id);
//		while (!q.empty()) {
//			uint id = q.front();
//			q.pop();
//			auto const id_it = shortest_path_tree.find(id);
//			if (id_it == shortest_path_tree.end())
//				continue;
//			auto& children_ids = id_it->second.children_ids;
//			for (auto const& children_id : children_ids) {
//				q.push(children_id);
//			}
//			shortest_path_tree.erase(id);
//		}
//
//		// start from s_id to find a new shortest path to e_id
//		vector<double> distance(number_of_nodes, Constants::T);
//		distance[s_id] = 0.0;
//		AscendingOrder<double> hd(&distance);
//		MappedHeap<AscendingOrder<double>> heap(node_idx, hd);
//		vector<uint> parent(number_of_nodes, number_of_nodes);
//		vector<bool> out_of_heap(number_of_nodes, false);
//		uint touch = number_of_nodes; // node id that the process reach the tree
//		while (!heap.empty()) {
//			uint id = heap.pop();
//			out_of_heap[id] = true;
//			if (distance[id] >= Constants::T * (1 - Constants::VAREPSILON) || id == e_id)
//				break;
//
//			auto const id_it = shortest_path_tree.find(id);
//			if (id_it != shortest_path_tree.end()) {
//				// if this node exists in shortest path tree to e_id, 
//				// update distance to e_id
//				double tmp = distance[id] + id_it->second.distance;
//				if (tmp < distance[e_id]) {
//					distance[e_id] = tmp;
//					touch = id;
//					heap.heapify(e_id);
//				}
//				continue;
//			}
//
//			for (auto const& neighbor_id : list_nodes[id].get_neighbors()) {
//				if (excluded_node[neighbor_id])
//					continue;
//				if (out_of_heap[neighbor_id])
//					continue;
//
//				double tmp = distance[id] + list_nodes[neighbor_id].get_working_weight();
//				if (tmp >= distance[neighbor_id])
//					continue;
//				distance[neighbor_id] = tmp;
//				parent[neighbor_id] = id;
//				heap.heapify(neighbor_id);
//			}
//		}
//		if (distance[e_id] >= Constants::T * (1 - Constants::VAREPSILON)) {
//			info("No new path is found between {} and {}", s_id, e_id);
//			return re;
//		}
//		// re-construct shortest path
//		shortest_path.clear();
//		uint tmp = touch;
//		while (tmp != s_id) {
//			shortest_path.emplace_back(tmp);
//			tmp = parent[tmp];
//		}
//		shortest_path.emplace_back(s_id);
//		std::reverse(shortest_path.begin(), shortest_path.end());
//		tmp = touch;
//		while (tmp != e_id) {
//			auto const tmp_it = shortest_path_tree.find(tmp);
//			if (tmp_it == shortest_path_tree.end()) {
//				info("Error while finding path from {} to {}", s_id, e_id);
//				throw "Error here";
//			}
//			tmp = tmp_it->second.parent_id;
//			shortest_path.emplace_back(tmp);
//		}
//	}
//	return re;
//}

double Network::increase_node_working_weight_by_budget(
	uint const & node_id, double const & inc_budget)
{
	return list_nodes[node_id].increase_working_weight_by_budget(inc_budget);
}

void Network::reset_budget()
{
	for (auto& node : list_nodes) {
		node.reset_budget();
	}
}

void Network::save_budget()
{
	for (auto& node : list_nodes) {
		node.save_budget();
	}
}

void Network::clear()
{
	list_nodes.clear();
	map_node_id.clear();
	node_idx.clear();
	list_txs.clear();
	number_of_nodes = 0;
}

const map<uint, TreeNode> Network::get_shortest_path_tree(uint const & root)
{
	map<uint, TreeNode> result;
	result.emplace(root, TreeNode(root, number_of_nodes, 0.0));
	vector<double> distance(number_of_nodes, Constants::T);
	distance[root] = 0.0;
	AscendingOrder<double> hd(&distance);
	MappedHeap<AscendingOrder<double>> heap(node_idx, hd);
	vector<bool> out_of_heap(number_of_nodes, false);

	while (!heap.empty()) {
		uint id = heap.pop();
		out_of_heap[id] = true;
		if (distance[id] > 
			Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION)
			break;
		for (auto const& neighbor_id : list_nodes[id].get_neighbors()) {
			if (out_of_heap[neighbor_id])
				continue;
			double tmp = distance[id] + list_nodes[id].get_working_weight();
			if (tmp >= distance[neighbor_id] || 
				tmp > Constants::T * (1.0 - Constants::VAREPSILON) - Constants::PRECISION)
				continue;
			distance[neighbor_id] = tmp;
			result[neighbor_id] = 
				TreeNode(neighbor_id, id, distance[neighbor_id]);
			heap.heapify(neighbor_id);
		}
	}

	// update children
	for (auto tree_node_it : result) {
		auto& tree_node = tree_node_it.second;
		if (tree_node.node_id == root)
			continue;
		result[tree_node.parent_id].children_ids.push_back(tree_node.node_id);
	}
	return result;
}

