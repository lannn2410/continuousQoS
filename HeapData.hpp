#ifndef _HEAP_DATA_H_
#define _HEAP_DATA_H_

#include <vector>

using namespace std;

template<class T1>
struct DescendingOrder{
	vector<T1> *v1;
public:
	DescendingOrder(vector<T1> *u1):v1(u1){};
	
	bool operator() (int &i, int &j) const{
		return (*v1)[i] < (*v1)[j];
	}
};

template<class T1>
struct AscendingOrder {
	vector<T1> * v1;
public:
	AscendingOrder(vector<T1> * u1) :v1(u1) {};

	bool operator() (int &i, int &j) const {
		return (*v1)[i] > (*v1)[j];
	}
};

#endif
