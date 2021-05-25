#include "OptimalBlocking.h"
#include <ilcplex/ilocplex.h>
#include <ilcplex/cplex.h>
#include <float.h>

OptimalBlocking::OptimalBlocking(Network& g) : PathBlockingAlgorithm(g)
{
}

// used only for linear case
vector<double> OptimalBlocking::block(vector<vector<uint>> const& list_paths)
{
	if (Constants::FUNCTION != FUNC::linear && Constants::FUNCTION != FUNC::step) {
		throw "opt should not run here";
	}

	last_block_queries = 0;
	initiate();
	const uint no_nodes = g.get_no_nodes();
	vector<double> v(no_nodes, 0); // returned result

	// setup cplex environment
	IloEnv env;
	IloCplex cplex(env);
	IloModel model(env);

	cplex.extract(model);
	cplex.setOut(env.getNullStream());

	IloNumVarArray var(env, no_nodes, 0.0, DBL_MAX, 
		Constants::FUNCTION == FUNC::linear ? ILOFLOAT : ILOINT);
	IloNumArray result(env, no_nodes);

	// create objective function
	IloExpr obj(env);
	for (int i = 0; i < no_nodes; ++i) {
		obj += var[i];
	}

	model.add(IloMinimize(env, obj));

	// create constraints
	for (auto const& path : list_paths) {
		IloExpr expr(env);
		vector<uint> sub_path(
			path.begin() + 1, path.end() - 1); // ignore start and end node
		double thres = Constants::T * (1.0 - Constants::VAREPSILON);
		for (auto const& node_id : sub_path) {
			//expr += var[node_id] * var[node_id];
			expr += var[node_id];
			thres -= 1.0;
		}
		if (thres > 0)
			model.add(IloRange(env, thres, expr));
	}

	cplex.solve();
	if (cplex.getStatus() == IloAlgorithm::Optimal) {
		cplex.getValues(var, result);
		for (int i = 0; i < no_nodes; ++i) {
			v[i] = result[i];
			update_node_and_path_length(i, v[i]);
		}
	}

	env.end();

	return v;
}
