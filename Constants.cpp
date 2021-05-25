#include "Constants.h"

Constants::Constants()
{
}


Constants::~Constants()
{
}


double Constants::T = 10.0;
int Constants::NUMBER_OF_TRANSACTIONS = 10;
double Constants::VAREPSILON = 0.1;
double Constants::PRECISION = 0.001;
FUNC Constants::FUNCTION = FUNC::convex;
double Constants::DISCRETE_STEP = 0.1;

//double Constants::EPS_TE = 0.5;
double Constants::EPS_TE = 0.2;
double Constants::ALPHA_JSG = 100.0;