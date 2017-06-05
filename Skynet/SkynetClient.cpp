/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
* and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"

#ifdef _WIN32
# include <Windows.h>
# define sleep( seconds) Sleep( seconds * 1000);
#else
# include <unistd.h>
#endif

#include "SkynetClient.h"

#include "EClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "CommissionReport.h"
#include "ContractSamples.h"
#include "OrderSamples.h"
#include "ScannerSubscription.h"
#include "ScannerSubscriptionSamples.h"
#include "executioncondition.h"
#include "PriceCondition.h"
#include "MarginCondition.h"
#include "PercentChangeCondition.h"
#include "TimeCondition.h"
#include "VolumeCondition.h"
#include "AvailableAlgoParams.h"
#include "FAMethodSamples.h"
#include "CommonDefs.h"
#include "AccountSummaryTags.h"

#include <stdio.h>
#include <iostream>
#include <thread>
#include <ctime>

const int PING_DEADLINE = 2; // seconds
const int SLEEP_BETWEEN_PINGS = 30; // seconds

///////////////////////////////////////////////////////////
// member funcs
//! [socket_init]
SkynetEWrapper::SkynetEWrapper() :
	m_osSignal(2000)//2-seconds timeout
	, m_pClient(new EClientSocket(this, &m_osSignal))
	, m_state(ST_TICKDATAOPERATION)
	, m_sleepDeadline(0)
	, m_orderId(0)
	, m_pReader(0)
	, m_extraAuth(false)
	, skynetAlgo_(new SkynetAlgorithm())
	, t(time(NULL))
	, last_min(100)
{
	//pFile = fopen("D:\\Trading\\Skynet\\myfile.txt", "a");

	//ore::data::Log::instance().registerLogger(boost::make_shared<ore::data::FileLogger>("D:\\Trading\\Skynet\\myfile.txt"));
	//ore::data::Log::instance().setMask(15);
	//ore::data::Log::instance().switchOn();
}
//! [socket_init]
SkynetEWrapper::~SkynetEWrapper()
{
	if (m_pReader)
		delete m_pReader;

	delete m_pClient;
}

bool SkynetEWrapper::connect(const char *host, unsigned int port, int clientId)
{
	// trying to connect
	printf("Connecting to %s:%d clientId:%d\n", !(host && *host) ? "127.0.0.1" : host, port, clientId);

	bool bRes = m_pClient->eConnect(host, port, clientId, m_extraAuth);

	if (bRes) {
		printf("Connected to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);

		m_pReader = new EReader(m_pClient, &m_osSignal);

		m_pReader->start();
	}
	else
		printf("Cannot connect to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);

	return bRes;
}

void SkynetEWrapper::disconnect() const
{
	m_pClient->eDisconnect();

	printf("Disconnected\n");
}

bool SkynetEWrapper::isConnected() const
{
	return m_pClient->isConnected();
}

void SkynetEWrapper::setConnectOptions(const std::string& connectOptions)
{
	m_pClient->setConnectOptions(connectOptions);
}

void SkynetEWrapper::processMessages() {

	switch (m_state) {
	case ST_TICKDATAOPERATION:
		tickDataOperation();
	    break;
	case ST_TICKDATAOPERATION_ACK:
		break;
	case ST_CONTRACTOPERATION:
		contractOperations();
		break;
	case ST_CONTRACTOPERATION_ACK:
		break;
	case ST_HISTORICALDATAREQUESTS:
		historicalDataRequests();
		break;
	case ST_HISTORICALDATAREQUESTS_ACK:
		break;
	}


	m_pReader->checkClient();
	m_osSignal.waitForSignal();
	m_pReader->processMsgs();

}

//////////////////////////////////////////////////////////////////
// methods
//! [connectack]
void SkynetEWrapper::connectAck() {
	if (!m_extraAuth && m_pClient->asyncEConnect())
		m_pClient->startApi();
}
//! [connectack]

void SkynetEWrapper::reqCurrentTime()
{
	printf("Requesting Current Time\n");

	// set ping deadline to "now + n seconds"
	m_sleepDeadline = time(NULL) + PING_DEADLINE;

	m_state = ST_PING_ACK;

	m_pClient->reqCurrentTime();
}

void SkynetEWrapper::tickDataOperation()
{
	m_pClient->reqMarketDataType(3);
	/*** Requesting real time market data ***/
	//sleep(1);
	//! [reqmktdata]
	m_pClient->reqMktData(1001, ContractSamples::FutureWithLocalSymbol_CL(), "", false, TagValueListSPtr());
	m_pClient->reqMktData(1002, ContractSamples::FutureWithLocalSymbol_BZ(), "", false, TagValueListSPtr());
	//m_pClient->reqMktData(1003, ContractSamples::USDJPYFx(), "", false, TagValueListSPtr());
	//! [reqmktdata]

	//sleep(1);
	/*** Canceling the market data subscription ***/
	//! [cancelmktdata]
	//m_pClient->cancelMktData(1001);
	//m_pClient->cancelMktData(1002);
	//m_pClient->cancelMktData(1003);
	//! [cancelmktdata]

	m_state = ST_TICKDATAOPERATION_ACK;
}

void SkynetEWrapper::tickDataOperation(std::vector<TickerId> t, std::vector<Contract> c) {

	for (int i=0; i<t.size(); ++i)
		m_pClient->reqMktData(t[i], c[i], "", false, TagValueListSPtr());

	//for (int i = 0; i<t.size(); ++i)
		//m_pClient->cancelMktData(t[i]);

	m_state = ST_TICKDATAOPERATION_ACK;
}

void SkynetEWrapper::marketDepthOperations()
{
	/*** Requesting the Deep Book ***/
	//! [reqmarketdepth]
	m_pClient->reqMktDepth(2001, ContractSamples::EurGbpFx(), 5, TagValueListSPtr());
	//! [reqmarketdepth]
	sleep(2);
	/*** Canceling the Deep Book request ***/
	//! [cancelmktdepth]
	m_pClient->cancelMktDepth(2001);
	//! [cancelmktdepth]

	m_state = ST_MARKETDEPTHOPERATION_ACK;
}

void SkynetEWrapper::realTimeBars()
{
	/*** Requesting real time bars ***/
	//! [reqrealtimebars]
	m_pClient->reqRealTimeBars(3001, ContractSamples::EurGbpFx(), 5, "MIDPOINT", true, TagValueListSPtr());
	//! [reqrealtimebars]
	sleep(2);
	/*** Canceling real time bars ***/
	//! [cancelrealtimebars]
	m_pClient->cancelRealTimeBars(3001);
	//! [cancelrealtimebars]

	m_state = ST_REALTIMEBARS_ACK;
}

void SkynetEWrapper::marketDataType()
{
	//! [reqmarketdatatype]
	/*** By default only real-time (1) market data is enabled
	Sending frozen (2) enables frozen market data
	Sending delayed (3) enables delayed market data and disables delayed-frozen market data
	Sending delayed-frozen (4) enables delayed and delayed-frozen market data
	Sending real-time (1) disables frozen, delayed and delayed-frozen market data ***/
	/*** Switch to live (1) frozen (2) delayed (3) or delayed frozen (4)***/
	m_pClient->reqMarketDataType(2);
	//! [reqmarketdatatype]

	m_state = ST_MARKETDATATYPE_ACK;
}

void SkynetEWrapper::historicalDataRequests()
{
	/*** Requesting historical data ***/
	//! [reqhistoricaldata]
	std::time_t rawtime;
	std::tm* timeinfo;
	char queryTime[80];

	std::time(&rawtime);
	timeinfo = std::localtime(&rawtime);
	std::strftime(queryTime, 80, "%Y%m%d %H:%M:%S", timeinfo);

	m_pClient->reqHistoricalData(4001, ContractSamples::FutureWithLocalSymbol_CL(), queryTime, "1 Y", "1 day", "BID_ASK", 1, 1, TagValueListSPtr());
	m_pClient->reqHistoricalData(4002, ContractSamples::FutureWithLocalSymbol_BZ(), queryTime, "1 Y", "1 day", "BID_ASK", 1, 1, TagValueListSPtr());
	//! [reqhistoricaldata]
	sleep(2);
	/*** Canceling historical data requests ***/
	m_pClient->cancelHistoricalData(4001);
	m_pClient->cancelHistoricalData(4002);

	m_state = ST_HISTORICALDATAREQUESTS_ACK;
}

void SkynetEWrapper::optionsOperations()
{
	//! [reqsecdefoptparams]
	m_pClient->reqSecDefOptParams(0, "IBM", "", "STK", 8314);
	//! [reqsecdefoptparams]

	//! [calculateimpliedvolatility]
	m_pClient->calculateImpliedVolatility(5001, ContractSamples::NormalOption(), 5, 85);
	//! [calculateimpliedvolatility]

	//** Canceling implied volatility ***
	m_pClient->cancelCalculateImpliedVolatility(5001);

	//! [calculateoptionprice]
	m_pClient->calculateOptionPrice(5002, ContractSamples::NormalOption(), 0.22, 85);
	//! [calculateoptionprice]

	//** Canceling option's price calculation ***
	m_pClient->cancelCalculateOptionPrice(5002);

	//! [exercise_options]
	//** Exercising options ***
	m_pClient->exerciseOptions(5003, ContractSamples::OptionWithTradingClass(), 1, 1, "", 1);
	//! [exercise_options]

	m_state = ST_OPTIONSOPERATIONS_ACK;
}

void SkynetEWrapper::contractOperations()
{
	//! [reqcontractdetails]
	m_pClient->reqContractDetails(210, ContractSamples::FutureWithLocalSymbol_CL());
	m_pClient->reqContractDetails(210, ContractSamples::FutureWithLocalSymbol_BZ());
	//! [reqcontractdetails]

	//! [reqcontractdetailsnews]
	m_pClient->reqContractDetails(211, ContractSamples::NewsFeedForQuery());
	//! [reqcontractdetailsnews]

	m_state = ST_CONTRACTOPERATION_ACK;
}

void SkynetEWrapper::marketScanners()
{
	/*** Requesting all available parameters which can be used to build a scanner request ***/
	//! [reqscannerparameters]
	m_pClient->reqScannerParameters();
	//! [reqscannerparameters]
	sleep(2);

	/*** Triggering a scanner subscription ***/
	//! [reqscannersubscription]
	m_pClient->reqScannerSubscription(7001, ScannerSubscriptionSamples::HotUSStkByVolume(), TagValueListSPtr());
	//! [reqscannersubscription]

	sleep(2);
	/*** Canceling the scanner subscription ***/
	//! [cancelscannersubscription]
	m_pClient->cancelScannerSubscription(7001);
	//! [cancelscannersubscription]

	m_state = ST_MARKETSCANNERS_ACK;
}

void SkynetEWrapper::reutersFundamentals()
{
	/*** Requesting Fundamentals ***/
	//! [reqfundamentaldata]
	m_pClient->reqFundamentalData(8001, ContractSamples::USStock(), "ReportsFinSummary");
	//! [reqfundamentaldata]
	sleep(2);

	/*** Canceling fundamentals request ***/
	//! [cancelfundamentaldata]
	m_pClient->cancelFundamentalData(8001);
	//! [cancelfundamentaldata]

	m_state = ST_REUTERSFUNDAMENTALS_ACK;
}

void SkynetEWrapper::bulletins()
{
	/*** Requesting Interactive Broker's news bulletins */
	//! [reqnewsbulletins]
	m_pClient->reqNewsBulletins(true);
	//! [reqnewsbulletins]
	sleep(2);
	/*** Canceling IB's news bulletins ***/
	//! [cancelnewsbulletins]
	m_pClient->cancelNewsBulletins();
	//! [cancelnewsbulletins]

	m_state = ST_BULLETINS_ACK;
}

void SkynetEWrapper::accountOperations()
{
	/*** Requesting managed accounts***/
	//! [reqmanagedaccts]
	//m_pClient->reqManagedAccts();
	//! [reqmanagedaccts]
	//sleep(2);
	/*** Requesting accounts' summary ***/
	//! [reqaaccountsummary]
	//m_pClient->reqAccountSummary(9001, "All", AccountSummaryTags::getAllTags());
	//! [reqaaccountsummary]
	//sleep(2);
	//! [reqaaccountsummaryledger]
	//m_pClient->reqAccountSummary(9002, "All", "$LEDGER");
	//! [reqaaccountsummaryledger]
	//sleep(2);
	//! [reqaaccountsummaryledgercurrency]
	//m_pClient->reqAccountSummary(9003, "All", "$LEDGER:EUR");
	//! [reqaaccountsummaryledgercurrency]
	//sleep(2);
	//! [reqaaccountsummaryledgerall]
	//m_pClient->reqAccountSummary(9004, "All", "$LEDGER:ALL");
	//! [reqaaccountsummaryledgerall]
	//sleep(2);
	//! [cancelaaccountsummary]
	//m_pClient->cancelAccountSummary(9001);
	//m_pClient->cancelAccountSummary(9002);
	//m_pClient->cancelAccountSummary(9003);
	//m_pClient->cancelAccountSummary(9004);
	//! [cancelaaccountsummary]
	//sleep(2);
	/*** Subscribing to an account's information. Only one at a time! ***/
	//! [reqaaccountupdates]
	//m_pClient->reqAccountUpdates(true, "U150462");
	//! [reqaaccountupdates]
	//sleep(2);
	//! [cancelaaccountupdates]
	//m_pClient->reqAccountUpdates(false, "U150462");
	//! [cancelaaccountupdates]
	//sleep(2);

	//! [reqaaccountupdatesmulti]
	//m_pClient->reqAccountUpdatessMulti(9002, "U150462", "EUstocks", true);
	//! [reqaaccountupdatesmulti]
	//sleep(2);

	/*** Requesting all accounts' positions. ***/
	//! [reqpositions]
	m_pClient->reqPositions();
	//! [reqpositions]
	//sleep(2);
	//! [cancelpositions]
	m_pClient->cancelPositions();
	//! [cancelpositions]

	//! [reqpositionsmulti]
	//m_pClient->reqPositionsMulti(9003, "U150462", "EUstocks");
	//! [reqpositionsmulti]

	m_state = ST_ACCOUNTOPERATIONS_ACK;
}

void SkynetEWrapper::orderOperations()
{
	/*** Requesting the next valid id ***/
	//! [reqids]
	//The parameter is always ignored.
	m_pClient->reqIds(-1);
	//! [reqids]
	//! [reqallopenorders]
	m_pClient->reqAllOpenOrders();
	//! [reqallopenorders]
	//! [reqautoopenorders]
	m_pClient->reqAutoOpenOrders(true);
	//! [reqautoopenorders]
	//! [reqopenorders]
	m_pClient->reqOpenOrders();
	//! [reqopenorders]

	/*** Placing/modifying an order - remember to ALWAYS increment the nextValidId after placing an order so it can be used for the next one! ***/
	//! [order_submission]
	m_pClient->placeOrder(m_orderId++, ContractSamples::EURJPYFx(), OrderSamples::MarketOrder("BUY", 100));
	//! [order_submission]

	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::Block("BUY", 50, 20));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::BoxTop("SELL", 10));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::FutureComboContract(), OrderSamples::ComboLimitOrder("SELL", 1, 1, false));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::StockComboContract(), OrderSamples::ComboMarketOrder("BUY", 1, false));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionComboContract(), OrderSamples::ComboMarketOrder("BUY", 1, true));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::StockComboContract(), OrderSamples::LimitOrderForComboWithLegPrices("BUY", 1, std::vector<double>(10, 5), true));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::Discretionary("SELL", 1, 45, 0.5));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::LimitIfTouched("BUY", 1, 30, 34));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOnClose("SELL", 1, 34));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOnOpen("BUY", 1, 35));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketIfTouched("BUY", 1, 35));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOnClose("SELL", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOnOpen("BUY", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOrder("SELL", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketToLimit("BUY", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtIse(), OrderSamples::MidpointMatch("BUY", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::Stop("SELL", 1, 34.4));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::StopLimit("BUY", 1, 35, 33));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::StopWithProtection("SELL", 1, 45));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::SweepToFill("BUY", 1, 35));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::TrailingStop("SELL", 1, 0.5, 30));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::TrailingStopLimit("BUY", 1, 50, 5, 30));

	/*** Cancel all orders for all accounts ***/
	//m_pClient->reqGlobalCancel();

	/*** Request the day's executions ***/
	//! [reqexecutions]
	m_pClient->reqExecutions(10001, ExecutionFilter());
	//! [reqexecutions]

	//m_state = ST_ORDEROPERATIONS_ACK;
}

void SkynetEWrapper::ocaSamples()
{
	//OCA ORDER
	//! [ocasubmit]
	std::vector<Order> ocaOrders;
	ocaOrders.push_back(OrderSamples::LimitOrder("BUY", 1, 10));
	ocaOrders.push_back(OrderSamples::LimitOrder("BUY", 1, 11));
	ocaOrders.push_back(OrderSamples::LimitOrder("BUY", 1, 12));
	for (unsigned int i = 0; i < ocaOrders.size(); i++) {
		OrderSamples::OneCancelsAll("TestOca", ocaOrders[i], 2);
		m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), ocaOrders[i]);
	}
	//! [ocasubmit]

	m_state = ST_OCASAMPLES_ACK;
}

void SkynetEWrapper::conditionSamples()
{
	//! [order_conditioning_activate]
	Order lmt = OrderSamples::LimitOrder("BUY", 100, 10);
	//Order will become active if conditioning criteria is met
	PriceCondition* priceCondition = dynamic_cast<PriceCondition *>(OrderSamples::Price_Condition(208813720, "SMART", 600, false, false));
	ExecutionCondition* execCondition = dynamic_cast<ExecutionCondition *>(OrderSamples::Execution_Condition("EUR.USD", "CASH", "IDEALPRO", true));
	MarginCondition* marginCondition = dynamic_cast<MarginCondition *>(OrderSamples::Margin_Condition(30, true, false));
	PercentChangeCondition* pctChangeCondition = dynamic_cast<PercentChangeCondition *>(OrderSamples::Percent_Change_Condition(15.0, 208813720, "SMART", true, true));
	TimeCondition* timeCondition = dynamic_cast<TimeCondition *>(OrderSamples::Time_Condition("20160118 23:59:59", true, false));
	VolumeCondition* volumeCondition = dynamic_cast<VolumeCondition *>(OrderSamples::Volume_Condition(208813720, "SMART", false, 100, true));

	lmt.conditions.push_back(ibapi::shared_ptr<PriceCondition>(priceCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<ExecutionCondition>(execCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<MarginCondition>(marginCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<PercentChangeCondition>(pctChangeCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<TimeCondition>(timeCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<VolumeCondition>(volumeCondition));
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), lmt);
	//! [order_conditioning_activate]

	//Conditions can make the order active or cancel it. Only LMT orders can be conditionally canceled.
	//! [order_conditioning_cancel]
	Order lmt2 = OrderSamples::LimitOrder("BUY", 100, 20);
	//The active order will be cancelled if conditioning criteria is met
	lmt2.conditionsCancelOrder = true;
	PriceCondition* priceCondition2 = dynamic_cast<PriceCondition *>(OrderSamples::Price_Condition(208813720, "SMART", 600, false, false));
	lmt2.conditions.push_back(ibapi::shared_ptr<PriceCondition>(priceCondition2));
	m_pClient->placeOrder(m_orderId++, ContractSamples::EuropeanStock(), lmt2);
	//! [order_conditioning_cancel]

	m_state = ST_CONDITIONSAMPLES_ACK;
}

void SkynetEWrapper::bracketSample() {
	Order parent;
	Order takeProfit;
	Order stopLoss;
	//! [bracketsubmit]
	OrderSamples::BracketOrder(m_orderId++, parent, takeProfit, stopLoss, "BUY", 100, 30, 40, 20);
	m_pClient->placeOrder(parent.orderId, ContractSamples::EuropeanStock(), parent);
	m_pClient->placeOrder(takeProfit.orderId, ContractSamples::EuropeanStock(), takeProfit);
	m_pClient->placeOrder(stopLoss.orderId, ContractSamples::EuropeanStock(), stopLoss);
	//! [bracketsubmit]

	m_state = ST_BRACKETSAMPLES_ACK;
}

void SkynetEWrapper::hedgeSample() {
	//F Hedge order
	//! [hedgesubmit]
	//Parent order on a contract which currency differs from your base currency
	Order parent = OrderSamples::LimitOrder("BUY", 100, 10);
	parent.orderId = m_orderId++;
	//Hedge on the currency conversion
	Order hedge = OrderSamples::MarketFHedge(parent.orderId, "BUY");
	//Place the parent first...
	m_pClient->placeOrder(parent.orderId, ContractSamples::EuropeanStock(), parent);
	//Then the hedge order
	m_pClient->placeOrder(m_orderId++, ContractSamples::EurGbpFx(), hedge);
	//! [hedgesubmit]

	m_state = ST_HEDGESAMPLES_ACK;
}

void SkynetEWrapper::testAlgoSamples() {
	//! [algo_base_order]
	Order baseOrder = OrderSamples::LimitOrder("BUY", 1000, 1);
	//! [algo_base_order]

	//! [arrivalpx]
	AvailableAlgoParams::FillArrivalPriceParams(baseOrder, 0.1, "Aggressive", "09:00:00 CET", "16:00:00 CET", true, true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [arrivalpx]

	//! [darkice]
	AvailableAlgoParams::FillDarkIceParams(baseOrder, 10, "09:00:00 CET", "16:00:00 CET", true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [darkice]

	//! [ad]
	AvailableAlgoParams::FillAccumulateDistributeParams(baseOrder, 10, 60, true, true, 1, true, true, "09:00:00 CET", "16:00:00 CET");
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [ad]

	//! [twap]
	AvailableAlgoParams::FillTwapParams(baseOrder, "Marketable", "09:00:00 CET", "16:00:00 CET", true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [twap]

	//! [vwap]
	AvailableAlgoParams::FillBalanceImpactRiskParams(baseOrder, 0.1, "Aggressive", true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [vwap]

	//! [balanceimpactrisk]
	AvailableAlgoParams::FillBalanceImpactRiskParams(baseOrder, 0.1, "Aggressive", true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [balanceimpactrisk]

	//! [minimpact]
	AvailableAlgoParams::FillMinImpactParams(baseOrder, 0.3);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [minimpact]

	//! [adaptive]
	AvailableAlgoParams::FillAdaptiveParams(baseOrder, "Normal");
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [adaptive]

	m_state = ST_TESTALGOSAMPLES_ACK;
}

void SkynetEWrapper::financialAdvisorOrderSamples()
{
	//! [faorderoneaccount]
	Order faOrderOneAccount = OrderSamples::MarketOrder("BUY", 100);
	// Specify the Account Number directly
	faOrderOneAccount.account = "DU119915";
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), faOrderOneAccount);
	//! [faorderoneaccount]
	sleep(1);

	//! [faordergroupequalquantity]
	Order faOrderGroupEQ = OrderSamples::LimitOrder("SELL", 200, 2000);
	faOrderGroupEQ.faGroup = "Group_Equal_Quantity";
	faOrderGroupEQ.faMethod = "EqualQuantity";
	m_pClient->placeOrder(m_orderId++, ContractSamples::SimpleFuture(), faOrderGroupEQ);
	//! [faordergroupequalquantity]
	sleep(1);

	//! [faordergrouppctchange]
	Order faOrderGroupPC;
	faOrderGroupPC.action = "BUY";
	faOrderGroupPC.orderType = "MKT";
	// You should not specify any order quantity for PctChange allocation method
	faOrderGroupPC.faGroup = "Pct_Change";
	faOrderGroupPC.faMethod = "PctChange";
	faOrderGroupPC.faPercentage = "100";
	m_pClient->placeOrder(m_orderId++, ContractSamples::EurGbpFx(), faOrderGroupPC);
	//! [faordergrouppctchange]
	sleep(1);

	//! [faorderprofile]
	Order faOrderProfile = OrderSamples::LimitOrder("BUY", 200, 100);
	faOrderProfile.faProfile = "Percent_60_40";
	m_pClient->placeOrder(m_orderId++, ContractSamples::EuropeanStock(), faOrderProfile);
	//! [faorderprofile]

	m_state = ST_FAORDERSAMPLES_ACK;
}

void SkynetEWrapper::financialAdvisorOperations()
{
	/*** Requesting FA information ***/
	//! [requestfaaliases]
	m_pClient->requestFA(faDataType::ALIASES);
	//! [requestfaaliases]

	//! [requestfagroups]
	m_pClient->requestFA(faDataType::GROUPS);
	//! [requestfagroups]

	//! [requestfaprofiles]
	m_pClient->requestFA(faDataType::PROFILES);
	//! [requestfaprofiles]

	/*** Replacing FA information - Fill in with the appropriate XML string. ***/
	//! [replacefaonegroup]
	m_pClient->replaceFA(faDataType::GROUPS, FAMethodSamples::FAOneGroup());
	//! [replacefaonegroup]

	//! [replacefatwogroups]
	m_pClient->replaceFA(faDataType::GROUPS, FAMethodSamples::FATwoGroups());
	//! [replacefatwogroups]

	//! [replacefaoneprofile]
	m_pClient->replaceFA(faDataType::PROFILES, FAMethodSamples::FAOneProfile());
	//! [replacefaoneprofile]

	//! [replacefatwoprofiles]
	m_pClient->replaceFA(faDataType::PROFILES, FAMethodSamples::FATwoProfiles());
	//! [replacefatwoprofiles]

	m_state = ST_FAOPERATIONS_ACK;
}

void SkynetEWrapper::testDisplayGroups() {
	//! [querydisplaygroups]
	m_pClient->queryDisplayGroups(9001);
	//! [querydisplaygroups]

	sleep(1);

	//! [subscribetogroupevents]
	m_pClient->subscribeToGroupEvents(9002, 1);
	//! [subscribetogroupevents]

	sleep(1);

	//! [updatedisplaygroup]
	m_pClient->updateDisplayGroup(9002, "8314@SMART");
	//! [updatedisplaygroup]

	sleep(1);

	//! [subscribefromgroupevents]
	m_pClient->unsubscribeFromGroupEvents(9002);
	//! [subscribefromgroupevents]

	m_state = ST_TICKDATAOPERATION_ACK;
}

void SkynetEWrapper::miscelaneous()
{
	/*** Request TWS' current time ***/
	m_pClient->reqCurrentTime();
	/*** Setting TWS logging level  ***/
	m_pClient->setServerLogLevel(5);

	m_state = ST_MISCELANEOUS_ACK;
}

//! [nextvalidid]
void SkynetEWrapper::nextValidId(OrderId orderId)
{
	printf("Next Valid Id: %ld\n", orderId);
	m_orderId = orderId;
	//! [nextvalidid]

	//m_state = ST_TICKDATAOPERATION;
	//m_state = ST_MARKETDEPTHOPERATION;
	//m_state = ST_REALTIMEBARS;
	//m_state = ST_MARKETDATATYPE;
	//m_state = ST_HISTORICALDATAREQUESTS;
	//m_state = ST_CONTRACTOPERATION;
	//m_state = ST_MARKETSCANNERS;
	//m_state = ST_REUTERSFUNDAMENTALS;
	//m_state = ST_BULLETINS;
	//m_state = ST_ACCOUNTOPERATIONS;
	//m_state = ST_ORDEROPERATIONS;
	//m_state = ST_OCASAMPLES;
	//m_state = ST_CONDITIONSAMPLES;
	//m_state = ST_BRACKETSAMPLES;
	//m_state = ST_HEDGESAMPLES;
	//m_state = ST_TESTALGOSAMPLES;
	//m_state = ST_FAORDERSAMPLES;
	//m_state = ST_FAOPERATIONS;
	//m_state = ST_DISPLAYGROUPS;
	//m_state = ST_MISCELANEOUS;
	//m_state = ST_PING;
}


void SkynetEWrapper::currentTime(long time)
{
	if (m_state == ST_PING_ACK) {
		time_t t = (time_t)time;
		struct tm * timeinfo = localtime(&t);
		printf("The current date/time is: %s", asctime(timeinfo));

		time_t now = ::time(NULL);
		m_sleepDeadline = now + SLEEP_BETWEEN_PINGS;

		m_state = ST_PING_ACK;
	}
}

//! [error]
void SkynetEWrapper::error(const int id, const int errorCode, const std::string errorString)
{
	printf("Error. Id: %d, Code: %d, Msg: %s\n", id, errorCode, errorString.c_str());
}
//! [error]

//! [tickprice]
void SkynetEWrapper::tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute) {
	printf("Tick Price. Ticker Id: %ld, Field: %d, Price: %g, CanAutoExecute: %d\n", tickerId, (int)field, price, canAutoExecute);
	current_time = localtime(&t);
	current_min = current_time->tm_min;
	
	if (field == 68) {
		
		if (!skynetAlgo_->isBarData(tickerId)) {
			std::vector<SkynetBarData> bars;
			SkynetBarData bar = SkynetBarData();
			
			bar.open = price;
            bar.high = price;
            bar.low = price;
            bar.close = price;
            
            bar.min = current_min;
			
			bars.push_back(bar);
			
			skynetAlgo_->updateBarData(tickerId, bars);
			skynetAlgo_->updateBarLastMin(tickerId, current_min);

		} else {
			std::vector<SkynetBarData> bars = skynetAlgo_->getBarData(tickerId);
			last_min = skynetAlgo_->getBarLastMin(tickerId);
			
			if (current_min != last_min) {
				SkynetBarData bar = SkynetBarData();
			
				bar.open = price;
				bar.high = price;
				bar.low = price;
				bar.close = price;

				bar.min = current_min;

				bars.push_back(bar);

				skynetAlgo_->updateBarData(tickerId, bars);
				skynetAlgo_->updateBarLastMin(tickerId, current_min);
				
			} else {
				SkynetBarData bar = bars.back();
				
				bar.high = std::max(bar.high, price);
				bar.low = std::min(bar.low, price);
				bar.close = price;
				
				bars.pop_back();
				bars.push_back(bar);
				
				skynetAlgo_->updateBarData(tickerId, bars);
			}
		}
	}

	//fprintf(pFile, "Tick Price. Ticker Id: %ld, Field: %d, Price: %g, CanAutoExecute: %d\n", tickerId, (int)field, price, canAutoExecute);
	//ore::data::Log::instance().logStream() << "Ticker Id: " + std::to_string(tickerId) + " Field: " + std::to_string((int)field) + " Price: " + std::to_string(price);
	skynetAlgo_->updateTickPrice(tickerId, (int)field, price);
}
//! [tickprice]

//! [ticksize]
void SkynetEWrapper::tickSize(TickerId tickerId, TickType field, int size) {
	//printf("Tick Size. Ticker Id: %ld, Field: %d, Size: %d\n", tickerId, (int)field, size);
}
//! [ticksize]

//! [tickoptioncomputation]
void SkynetEWrapper::tickOptionComputation(TickerId tickerId, TickType tickType, double impliedVol, double delta,
	double optPrice, double pvDividend,
	double gamma, double vega, double theta, double undPrice) {
	printf("TickOptionComputation. Ticker Id: %ld, Type: %d, ImpliedVolatility: %g, Delta: %g, OptionPrice: %g, pvDividend: %g, Gamma: %g, Vega: %g, Theta: %g, Underlying Price: %g\n", tickerId, (int)tickType, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice);
}
//! [tickoptioncomputation]

//! [tickgeneric]
void SkynetEWrapper::tickGeneric(TickerId tickerId, TickType tickType, double value) {
	printf("Tick Generic. Ticker Id: %ld, Type: %d, Value: %g\n", tickerId, (int)tickType, value);
}
//! [tickgeneric]

//! [tickstring]
void SkynetEWrapper::tickString(TickerId tickerId, TickType tickType, const std::string& value) {
	printf("Tick String. Ticker Id: %ld, Type: %d, Value: %s\n", tickerId, (int)tickType, value.c_str());
}
//! [tickstring]

void SkynetEWrapper::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
	double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) {
	printf("TickEFP. %ld, Type: %d, BasisPoints: %g, FormattedBasisPoints: %s, Total Dividends: %g, HoldDays: %d, Future Last Trade Date: %s, Dividend Impact: %g, Dividends To Last Trade Date: %g\n", tickerId, (int)tickType, basisPoints, formattedBasisPoints.c_str(), totalDividends, holdDays, futureLastTradeDate.c_str(), dividendImpact, dividendsToLastTradeDate);
}

//! [orderstatus]
void SkynetEWrapper::orderStatus(OrderId orderId, const std::string& status, double filled,
	double remaining, double avgFillPrice, int permId, int parentId,
	double lastFillPrice, int clientId, const std::string& whyHeld) {
	printf("OrderStatus. Id: %ld, Status: %s, Filled: %g, Remaining: %g, AvgFillPrice: %g, PermId: %d, LastFillPrice: %g, ClientId: %d, WhyHeld: %s\n", orderId, status.c_str(), filled, remaining, avgFillPrice, permId, lastFillPrice, clientId, whyHeld.c_str());
}
//! [orderstatus]

//! [openorder]
void SkynetEWrapper::openOrder(OrderId orderId, const Contract& contract, const Order& order, const OrderState& ostate) {
	printf("OpenOrder. ID: %ld, %s, %s @ %s: %s, %s, %g, %s\n", orderId, contract.symbol.c_str(), contract.secType.c_str(), contract.exchange.c_str(), order.action.c_str(), order.orderType.c_str(), order.totalQuantity, ostate.status.c_str());
}
//! [openorder]

//! [openorderend]
void SkynetEWrapper::openOrderEnd() {
	printf("OpenOrderEnd\n");
}
//! [openorderend]

void SkynetEWrapper::winError(const std::string& str, int lastError) {}
void SkynetEWrapper::connectionClosed() {
	printf("Connection Closed\n");
}

//! [updateaccountvalue]
void SkynetEWrapper::updateAccountValue(const std::string& key, const std::string& val,
	const std::string& currency, const std::string& accountName) {
	printf("UpdateAccountValue. Key: %s, Value: %s, Currency: %s, Account Name: %s\n", key.c_str(), val.c_str(), currency.c_str(), accountName.c_str());
}
//! [updateaccountvalue]

//! [updateportfolio]
void SkynetEWrapper::updatePortfolio(const Contract& contract, double position,
	double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const std::string& accountName) {
	printf("UpdatePortfolio. %s, %s @ %s: Position: %g, MarketPrice: %g, MarketValue: %g, AverageCost: %g, UnrealisedPNL: %g, RealisedPNL: %g, AccountName: %s\n", (contract.symbol).c_str(), (contract.secType).c_str(), (contract.primaryExchange).c_str(), position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName.c_str());
}
//! [updateportfolio]

//! [updateaccounttime]
void SkynetEWrapper::updateAccountTime(const std::string& timeStamp) {
	printf("UpdateAccountTime. Time: %s\n", timeStamp.c_str());
}
//! [updateaccounttime]

//! [accountdownloadend]
void SkynetEWrapper::accountDownloadEnd(const std::string& accountName) {
	printf("Account download finished: %s\n", accountName.c_str());
}
//! [accountdownloadend]

//! [contractdetails]
void SkynetEWrapper::contractDetails(int reqId, const ContractDetails& contractDetails) {
	printf("ContractDetails. ReqId: %d - %s, %s, ConId: %ld @ %s, Trading Hours: %s, Liquidation Hours: %s\n", reqId, contractDetails.summary.symbol.c_str(), contractDetails.summary.secType.c_str(), contractDetails.summary.conId, contractDetails.summary.exchange.c_str(), contractDetails.tradingHours.c_str(), contractDetails.liquidHours.c_str());
}
//! [contractdetails]

void SkynetEWrapper::bondContractDetails(int reqId, const ContractDetails& contractDetails) {
	printf("Bond. ReqId: %d, Symbol: %s, Security Type: %s, Currency: %s, Trading Hours: %s, Liquidation Hours: %s\n", reqId, contractDetails.summary.symbol.c_str(), contractDetails.summary.secType.c_str(), contractDetails.summary.currency.c_str(), contractDetails.tradingHours.c_str(), contractDetails.liquidHours.c_str());
}

//! [contractdetailsend]
void SkynetEWrapper::contractDetailsEnd(int reqId) {
	printf("ContractDetailsEnd. %d\n", reqId);
}
//! [contractdetailsend]

//! [execdetails]
void SkynetEWrapper::execDetails(int reqId, const Contract& contract, const Execution& execution) {
	printf("ExecDetails. ReqId: %d - %s, %s, %s - %s, %ld, %g\n", reqId, contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), execution.execId.c_str(), execution.orderId, execution.shares);
}
//! [execdetails]

//! [execdetailsend]
void SkynetEWrapper::execDetailsEnd(int reqId) {
	printf("ExecDetailsEnd. %d\n", reqId);
}
//! [execdetailsend]

//! [updatemktdepth]
void SkynetEWrapper::updateMktDepth(TickerId id, int position, int operation, int side,
	double price, int size) {
	printf("UpdateMarketDepth. %ld - Position: %d, Operation: %d, Side: %d, Price: %g, Size: %d\n", id, position, operation, side, price, size);
}
//! [updatemktdepth]

void SkynetEWrapper::updateMktDepthL2(TickerId id, int position, std::string marketMaker, int operation,
	int side, double price, int size) {
	printf("UpdateMarketDepthL2. %ld - Position: %d, Operation: %d, Side: %d, Price: %g, Size: %d\n", id, position, operation, side, price, size);
}

//! [updatenewsbulletin]
void SkynetEWrapper::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch) {
	printf("News Bulletins. %d - Type: %d, Message: %s, Exchange of Origin: %s\n", msgId, msgType, newsMessage.c_str(), originExch.c_str());
}
//! [updatenewsbulletin]

//! [managedaccounts]
void SkynetEWrapper::managedAccounts(const std::string& accountsList) {
	printf("Account List: %s\n", accountsList.c_str());
}
//! [managedaccounts]

//! [receivefa]
void SkynetEWrapper::receiveFA(faDataType pFaDataType, const std::string& cxml) {
	std::cout << "Receiving FA: " << (int)pFaDataType << std::endl << cxml << std::endl;
}
//! [receivefa]

//! [historicaldata]
void SkynetEWrapper::historicalData(TickerId reqId, const std::string& date, double open, double high,
	double low, double close, int volume, int barCount, double WAP, int hasGaps) {
	printf("HistoricalData. ReqId: %ld - Date: %s, Open: %g, High: %g, Low: %g, Close: %g, Volume: %d, Count: %d, WAP: %g, HasGaps: %d\n", reqId, date.c_str(), open, high, low, close, volume, barCount, WAP, hasGaps);
}
//! [historicaldata]

//! [scannerparameters]
void SkynetEWrapper::scannerParameters(const std::string& xml) {
	printf("ScannerParameters. %s\n", xml.c_str());
}
//! [scannerparameters]

//! [scannerdata]
void SkynetEWrapper::scannerData(int reqId, int rank, const ContractDetails& contractDetails,
	const std::string& distance, const std::string& benchmark, const std::string& projection,
	const std::string& legsStr) {
	printf("ScannerData. %d - Rank: %d, Symbol: %s, SecType: %s, Currency: %s, Distance: %s, Benchmark: %s, Projection: %s, Legs String: %s\n", reqId, rank, contractDetails.summary.symbol.c_str(), contractDetails.summary.secType.c_str(), contractDetails.summary.currency.c_str(), distance.c_str(), benchmark.c_str(), projection.c_str(), legsStr.c_str());
}
//! [scannerdata]

//! [scannerdataend]
void SkynetEWrapper::scannerDataEnd(int reqId) {
	printf("ScannerDataEnd. %d\n", reqId);
}
//! [scannerdataend]

//! [realtimebar]
void SkynetEWrapper::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
	long volume, double wap, int count) {
	printf("RealTimeBars. %ld - Time: %ld, Open: %g, High: %g, Low: %g, Close: %g, Volume: %ld, Count: %d, WAP: %g\n", reqId, time, open, high, low, close, volume, count, wap);
}
//! [realtimebar]

//! [fundamentaldata]
void SkynetEWrapper::fundamentalData(TickerId reqId, const std::string& data) {
	printf("FundamentalData. ReqId: %ld, %s\n", reqId, data.c_str());
}
//! [fundamentaldata]

void SkynetEWrapper::deltaNeutralValidation(int reqId, const UnderComp& underComp) {
	printf("DeltaNeutralValidation. %d, ConId: %ld, Delta: %g, Price: %g\n", reqId, underComp.conId, underComp.delta, underComp.price);
}

//! [ticksnapshotend]
void SkynetEWrapper::tickSnapshotEnd(int reqId) {
	printf("TickSnapshotEnd: %d\n", reqId);
}
//! [ticksnapshotend]

//! [marketdatatype]
void SkynetEWrapper::marketDataType(TickerId reqId, int marketDataType) {
	printf("MarketDataType. ReqId: %ld, Type: %d\n", reqId, marketDataType);
}
//! [marketdatatype]

//! [commissionreport]
void SkynetEWrapper::commissionReport(const CommissionReport& commissionReport) {
	printf("CommissionReport. %s - %g %s RPNL %g\n", commissionReport.execId.c_str(), commissionReport.commission, commissionReport.currency.c_str(), commissionReport.realizedPNL);
}
//! [commissionreport]

//! [position]
void SkynetEWrapper::position(const std::string& account, const Contract& contract, double position, double avgCost) {
	printf("Position. %s - Symbol: %s, SecType: %s, Currency: %s, Position: %g, Avg Cost: %g\n", account.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), position, avgCost);
}
//! [position]

//! [positionend]
void SkynetEWrapper::positionEnd() {
	printf("PositionEnd\n");
}
//! [positionend]

//! [accountsummary]
void SkynetEWrapper::accountSummary(int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& currency) {
	printf("Acct Summary. ReqId: %d, Account: %s, Tag: %s, Value: %s, Currency: %s\n", reqId, account.c_str(), tag.c_str(), value.c_str(), currency.c_str());
}
//! [accountsummary]

//! [accountsummaryend]
void SkynetEWrapper::accountSummaryEnd(int reqId) {
	printf("AccountSummaryEnd. Req Id: %d\n", reqId);
}
//! [accountsummaryend]

void SkynetEWrapper::verifyMessageAPI(const std::string& apiData) {
	printf("verifyMessageAPI: %s\b", apiData.c_str());
}

void SkynetEWrapper::verifyCompleted(bool isSuccessful, const std::string& errorText) {
	printf("verifyCompleted. IsSuccessfule: %d - Error: %s\n", isSuccessful, errorText.c_str());
}

void SkynetEWrapper::verifyAndAuthMessageAPI(const std::string& apiDatai, const std::string& xyzChallenge) {
	printf("verifyAndAuthMessageAPI: %s %s\n", apiDatai.c_str(), xyzChallenge.c_str());
}

void SkynetEWrapper::verifyAndAuthCompleted(bool isSuccessful, const std::string& errorText) {
	printf("verifyAndAuthCompleted. IsSuccessful: %d - Error: %s\n", isSuccessful, errorText.c_str());
	if (isSuccessful)
		m_pClient->startApi();
}

//! [displaygrouplist]
void SkynetEWrapper::displayGroupList(int reqId, const std::string& groups) {
	printf("Display Group List. ReqId: %d, Groups: %s\n", reqId, groups.c_str());
}
//! [displaygrouplist]

//! [displaygroupupdated]
void SkynetEWrapper::displayGroupUpdated(int reqId, const std::string& contractInfo) {
	std::cout << "Display Group Updated. ReqId: " << reqId << ", Contract Info: " << contractInfo << std::endl;
}
//! [displaygroupupdated]

//! [positionmulti]
void SkynetEWrapper::positionMulti(int reqId, const std::string& account, const std::string& modelCode, const Contract& contract, double pos, double avgCost) {
	printf("Position Multi. Request: %d, Account: %s, ModelCode: %s, Symbol: %s, SecType: %s, Currency: %s, Position: %g, Avg Cost: %g\n", reqId, account.c_str(), modelCode.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), pos, avgCost);
}
//! [positionmulti]

//! [positionmultiend]
void SkynetEWrapper::positionMultiEnd(int reqId) {
	printf("Position Multi End. Request: %d\n", reqId);
}
//! [positionmultiend]

//! [accountupdatemulti]
void SkynetEWrapper::accountUpdateMulti(int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency) {
	printf("AccountUpdate Multi. Request: %d, Account: %s, ModelCode: %s, Key, %s, Value: %s, Currency: %s\n", reqId, account.c_str(), modelCode.c_str(), key.c_str(), value.c_str(), currency.c_str());
}
//! [accountupdatemulti]

//! [accountupdatemultiend]
void SkynetEWrapper::accountUpdateMultiEnd(int reqId) {
	printf("Account Update Multi End. Request: %d\n", reqId);
}
//! [accountupdatemultiend]

//! [securityDefinitionOptionParameter]
void SkynetEWrapper::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, std::set<std::string> expirations, std::set<double> strikes) {
	printf("Security Definition Optional Parameter. Request: %d, Trading Class: %s, Multiplier: %s\n", reqId, tradingClass.c_str(), multiplier.c_str());
}
//! [securityDefinitionOptionParameter]

//! [securityDefinitionOptionParameterEnd]
void SkynetEWrapper::securityDefinitionOptionalParameterEnd(int reqId) {
	printf("Security Definition Optional Parameter End. Request: %d\n", reqId);
}
//! [securityDefinitionOptionParameterEnd]

//! [softDollarTiers]
void SkynetEWrapper::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) {
	printf("Soft dollar tiers (%d):", tiers.size());

	for (int i = 0; i < tiers.size(); i++) {
		printf("%s\n", tiers[0].displayName());
	}
}
//! [softDollarTiers]
