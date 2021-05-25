#pragma once

enum FUNC { concave, convex, linear, step};
// convex: f(x) = x^2 + 1
// concave: f(x) = log(x+1)
// linear: f(x) = x + 1
// step: f(x) = floor(x) + 1

class Constants
{
public:
	Constants();
	~Constants();

	static int NUMBER_OF_TRANSACTIONS;
	static double T;
	static const int NUM_THREAD = 70;
	static const int KSP = 20;
	static double VAREPSILON;
	static double PRECISION; // precision on double calculation
	static double DISCRETE_STEP; // used to discretize the continuous function
	static FUNC FUNCTION;
	static double EPS_TE;
	static double ALPHA_JSG;
};

