#include "portbroker.h"

#include <Bela.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>


#define BROKER_PORT 8314
#define STARTING_PORT 42000
#define MAX_PORTS 50

#define MAGIC_1 0xbe
#define MAGIC_2 0x1a


PortAssignment localReservations[MAX_PORTS];
PortAssignment remoteReservations[MAX_PORTS];


typedef struct {
	uint8_t magic1;
	uint8_t magic2;
    in_port_t port;
	int status;
} BrokerPkt;


int PortBroker::getPortAssignmentForHost(const char * host, PortAssignment &assignment) {

	printf("Negotiating port assignment with %s...\n", host);
    fflush(stdout);

	struct sockaddr_in peerAddr;
	socklen_t peerAddrLen = sizeof(peerAddr);
	memset((char *) &peerAddr, 0, peerAddrLen);
	peerAddr.sin_family = AF_INET;
	peerAddr.sin_port = htons(BROKER_PORT);
	if (inet_aton(host, &peerAddr.sin_addr) == 0) 
	{
		printf("ERROR: Failed to parse peer address: '%s'\n", host);
		fflush(stdout);
		return -1;
	}

	int sock = createTcpSocket(2);
	if (sock < 0) {
		printf("ERROR: Failed to create TCP socket for port assignment, got errno %d\n", errno);
		fflush(stdout);
		return -1;
	}

	if (connect(sock, (struct sockaddr*)&peerAddr, peerAddrLen) != 0) {
		printf("ERROR: Failed to connect TCP socket to host, got errno %d\n", errno);
		fflush(stdout);
		close(sock);
		return -1;
	}

	BrokerPkt pkt;
	BrokerPkt replyPkt;
	pkt.magic1 = MAGIC_1;
	pkt.magic2 = MAGIC_2;


	printf("Starting assignment exchange...\n");
    fflush(stdout);

	assignment.peerAddr = peerAddr.sin_addr;
	for (uint16_t offset = 0; offset < MAX_PORTS; offset++) {
		int result = proposeAssignment(assignment, offset);
		if (result == -1) {
			continue;
		}
		else if (result == 1) {
			offset = MAX_PORTS;
		}

		pkt.status = 0;
		pkt.port = assignment.port;

		printf("Sending proposal...\n");
		fflush(stdout);

		if (send(sock, (char*)&pkt, sizeof(BrokerPkt), 0) == -1) {
			printf("ERROR: Unexpected PortBroker send error, got errno %d\n", errno);
			break;
		}

		printf("Awaiting proposal response...\n");
		fflush(stdout);

		if (read(sock, (char*)&replyPkt, sizeof(BrokerPkt)) == -1) {
			if (errno == EAGAIN) {
				printf("ERROR: Timed out waiting for peer PortBroker respose\n");
			}
			else {
				printf("ERROR: Unexpected PortBroker read error, got errno %d\n", errno);
			}
			break;
		}

		if (replyPkt.status == -1) {
			continue;
		}
		else if (replyPkt.status == 1) {
			assignment.port = replyPkt.port;
		}

		printf("Assignment successfully negotiated: %d\n", ntohs(assignment.port));
		fflush(stdout);

		uint16_t portOffset = ntohs(assignment.port) - STARTING_PORT;
		localReservations[portOffset] = assignment;
		close(sock);
		return 0;
	}

	close(sock);
	printf("ERROR: Failed to negotiate a transmission port with peer!\n");
	fflush(stdout);
	return -1;
}


void PortBroker::assignPorts(void *) {
	printf("Starting PortBroker...\n");
	fflush(stdout);

	memset((char *) &localReservations, 0, sizeof(PortAssignment) * MAX_PORTS);
	memset((char *) &remoteReservations, 0, sizeof(PortAssignment) * MAX_PORTS);

	int sock = createTcpSocket(1, 0);
	if (sock < 0) {
		return;
	}

	struct sockaddr_in addr;
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(BROKER_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(sock, (struct sockaddr*)&addr, sizeof(addr) ) == -1)
	{
		printf("FATAL: Failed to bind TCP receive socket, got errno %d\n", errno);
		fflush(stdout);
		return;
	}

	if (listen(sock, 20) == -1) {
		printf("FATAL: Failed to listen with TCP receive socket, got errno %d\n", errno);
		fflush(stdout);
		return;
	}

	struct sockaddr_in peerAddr;
	socklen_t peerAddrLen = sizeof(peerAddr);
	BrokerPkt pkt;
	BrokerPkt replyPkt;

	PortAssignment proposedAssignment;
	PortAssignment existingAssignment;

	while (!Bela_stopRequested()) {
		int cSock = accept(sock, (struct sockaddr*)&peerAddr, &peerAddrLen);

		if (cSock < 0) {
			if (errno == EAGAIN) {
				continue;
			}
			printf("ERROR: Unexpected PortBroker accept error, got errno %d\n", errno);
			continue;
		}

		while (true) {
			int nBytes = read(cSock, (char*)&pkt, sizeof(BrokerPkt));

			if (nBytes < 0) {
				printf("ERROR: Unexpected PortBroker read error, got errno %d\n", errno);
				break;
			}
			if (nBytes == 0) {
				break;
			}
			if (nBytes != sizeof(BrokerPkt)) {
				break;
			}
			if (pkt.magic1 != MAGIC_1 || pkt.magic2 != MAGIC_2) {
				break;
			}

			printf("Processing port assignment request...\n");
			fflush(stdout);

			proposedAssignment.peerAddr = peerAddr.sin_addr;
			proposedAssignment.port = pkt.port;

			printf("Checking for conflict on proposed port %d...\n", ntohs(pkt.port));
			fflush(stdout);

			int status = validateAssignment(proposedAssignment, existingAssignment);
			memcpy(&replyPkt, &pkt, sizeof(BrokerPkt));
			replyPkt.status = status;
			if (status == 1) {
				replyPkt.port = existingAssignment.port;
				proposedAssignment.port = existingAssignment.port;
			}

			printf("Replying with determination (%d)\n", status);
			fflush(stdout);

			if (send(cSock, (char*)&replyPkt, sizeof(BrokerPkt), 0) == -1) {
				printf("ERROR: Unexpected PortBroker send error, got errno %d\n", errno);
				break;
			}

			if (status != -1) {
				uint16_t portOffset = ntohs(replyPkt.port) - STARTING_PORT;
				remoteReservations[portOffset] = proposedAssignment;
				break;
			}
		}

		fflush(stdout);
		usleep(500);
	}

	printf("PortBroker Finished\n");
	fflush(stdout);
}


int PortBroker::createTcpSocket(int timeoutSec, int timeoutUSec) {
	int sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("FATAL: Failed to create TCP socket, got errno %d\n", errno);
		fflush(stdout);
		return -1;
	}

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			printf("Failed to set receive timeout on TCP socket, got errno %d\n", errno);
			fflush(stdout);
			return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
			printf("Failed to set send timeout on TCP socket, got errno %d\n", errno);
			fflush(stdout);
			return -1;
	}

	return sock;
}


int PortBroker::validateAssignment(const PortAssignment &proposed, PortAssignment &existing) {
	uint16_t portOffset = ntohs(proposed.port) - STARTING_PORT;
	memset((char*)&existing, 0, sizeof(PortAssignment));

	// check for existing local assignment of peer and port
	if (localReservations[portOffset].peerAddr.s_addr == proposed.peerAddr.s_addr) {
		return 0;
	}

	// check for existing local assignment of same peer, different port
	for (int i = 0; i < MAX_PORTS; i++) {
		if (localReservations[i].peerAddr.s_addr == proposed.peerAddr.s_addr) {
			memcpy(&existing, &localReservations[i], sizeof(PortAssignment));
			return 1;
		}
	}

	// check for conflict with a port reserved by a different remote peer
	if (remoteReservations[portOffset].port != 0) {
		return -1;
	}

	return 0;
}


int PortBroker::proposeAssignment(PortAssignment &assignment, uint16_t portOffset) {
	// check for existing remote assignment of peer and port
	if (remoteReservations[portOffset].peerAddr.s_addr == assignment.peerAddr.s_addr) {
		assignment.port = remoteReservations[portOffset].port;
		return 0;
	}

	// check for existing remote assignment of peer on different port
	for (int i = 0; i < MAX_PORTS; i++) {
		if (remoteReservations[i].peerAddr.s_addr == assignment.peerAddr.s_addr) {
			assignment.port = remoteReservations[i].port;
			return 1;
		}
	}

	// check for conflict with a locally reserved port
	if (localReservations[portOffset].port != 0) {
		return -1;
	}

	in_port_t netPort = htons(STARTING_PORT + portOffset);
	assignment.port = netPort;
	return 0;
}