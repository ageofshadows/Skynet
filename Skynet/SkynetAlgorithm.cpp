
#include "skynetalgorithm.h"

SkynetAlgorithm::SkynetAlgorithm() {

}

void SkynetAlgorithm::updateTickPrice(long tickerId, int field, double price) {
	tickPrice_[make_pair(tickerId, field)] = price;
}

bool SkynetAlgorithm::isBarData(long tickerId) {
	auto it = barData_.find(tickerId);

	return it != barData_.end() ? true : false;
}

std::vector<SkynetBarData> SkynetAlgorithm::getBarData(long tickerId) {
	auto it = barData_.find(tickerId);

	return it->second;
}

void SkynetAlgorithm::updateBarData(long tickerId, std::vector<SkynetBarData> bar) {
	barData_[tickerId] = bar;
}

int SkynetAlgorithm::getBarLastMin(long tickerId) {
	auto it = barLastMin_.find(tickerId);

	return it->second;
}

void SkynetAlgorithm::updateBarLastMin(long tickerId, int min) {
	barLastMin_[tickerId] = min;
}

std::vector<int> SkynetAlgorithm::depolyStrategy(Strategy, std::vector<TickerId> t) {
	return fxSpotTriangle(t);
}

std::vector<int> SkynetAlgorithm::fxSpotTriangle(std::vector<TickerId> t) {
	std::vector<int> s(3, 0);
	map<pair<long, int>, double>::iterator it;
	double bid1, ask1, bid2, ask2, bid3, ask3;
	double mid1, mid2, mid3;
	// bid
	it = tickPrice_.find(make_pair(t[0], 1));
	if (it != tickPrice_.end())
		bid1 = it->second;
	else
		return s;
	// ask
	it = tickPrice_.find(make_pair(t[0], 2));
	if (it != tickPrice_.end())
		ask1 = it->second;
	else
		return s;

	mid1 = (bid1 + ask1) / 2.0;

	// bid
	it = tickPrice_.find(make_pair(t[1], 1));
	if (it != tickPrice_.end())
		bid2 = it->second;
	else
		return s;
	// ask
	it = tickPrice_.find(make_pair(t[1], 2));
	if (it != tickPrice_.end())
		ask2 = it->second;
	else
		return s;

	mid2 = (bid2 + ask2) / 2.0;

	// bid
	it = tickPrice_.find(make_pair(t[2], 1));
	if (it != tickPrice_.end())
		bid3 = it->second;
	else
		return s;
	// ask
	it = tickPrice_.find(make_pair(t[2], 2));
	if (it != tickPrice_.end())
		ask3 = it->second;
	else
		return s;

	mid3 = (bid3 + ask3) / 2.0;

	if (bid1*bid3 > ask2) {
		s[0] = s[2] = -1;
		s[1] = 1;
	}
	else if (ask1*ask3 < bid2) {
		s[0] = s[2] = 1;
		s[1] = -1;
	}

	return s;

}
