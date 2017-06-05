
#pragma once
#ifndef skynetalgorithm_h__INCLUDED
#define skynetalgorithm_h__INCLUDED

#include <map>
#include <vector>

#include "commondefs.h"

using std::map;
using std::pair;
using std::make_pair;

enum Strategy {
	FX_SpotTriangle,
	Oil
};

class SkynetAlgorithm
{
public:
	SkynetAlgorithm();
	~SkynetAlgorithm() {}

	void updateTickPrice(long tickerId, int field, double price);

	bool isBarData(long tickerId);
	std::vector<SkynetBarData> getBarData(long tickerId);
	void updateBarData(long tickerId, std::vector<SkynetBarData> bar);
	int getBarLastMin(long tickerId);
	void updateBarLastMin(long tickerId, int min);

	std::vector<int> depolyStrategy(Strategy, std::vector<TickerId>);

private:
	std::vector<int> fxSpotTriangle(std::vector<TickerId>);

private:
	map<pair<long, int>, double> tickPrice_;
	map<long, std::vector<SkynetBarData>> barData_;
	map<long, int> barLastMin_;
};

struct SkynetBarData
{
	SkynetBarData() {}

	double open, high, low, close;

	int year, mon, mday, hour, min, sec;//2017-6-4 20:22:33
};


#endif
