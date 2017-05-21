// Skynet.cpp : Defines the entry point for the console application.
//

#include <stdafx.h>

#include "IBCppClient.h"

#include <boost/shared_ptr.hpp>


int main(int argc, char** argv)
{
	const char* host = argc > 1 ? argv[1] : "";
	unsigned int port = argc > 2 ? atoi(argv[2]) : 0;
	if (port <= 0)
		port = 7496;
	int clientId = 0;

	printf("Starting Skynet\n");

	boost::shared_ptr<InteractiveBrokersCppClient> ibClient(new InteractiveBrokersCppClient());

	//! [connect]
	ibClient->connect(host, port, clientId);
	//! [connect]

	//! [ereader]
	//Unlike the C# and Java clients, there is no need to explicitely create an EReader object nor a thread
	while (ibClient->isConnected()) {
		ibClient->processMessages();
	}
	//! [ereader]

	printf("End of Skynet Engine\n");

    return 0;
}

