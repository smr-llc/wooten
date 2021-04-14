#include "session.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <random>
#include <chrono>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <iostream>

#include "wootbase.h"

Session::Session() :
    m_udpActive(false),
    m_managerActive(false),
    m_terminate(false)    
{
    memset(&m_myJoinedData, 0, sizeof(JoinedData));
}

int Session::setup(WootBase *wootBase) {
    m_wootBase = wootBase;
    if ((m_rxSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		printf("FATAL: Failed to create udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		return -1;
	}

	memset((char *) &m_udpAddr, 0, sizeof(m_udpAddr));
	m_udpAddr.sin_family = AF_INET;
	m_udpAddr.sin_port = htons(APP_PORT_NUMBER);
	m_udpAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    if (setsockopt(m_rxSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "FATAL: Failed to set timeout for UDP receive socket for NAT mapping\n";
        return -1;
    }

    int enableReuse = 1;
	if (setsockopt(m_rxSock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        std::cerr << "FATAL: Failed to set port re-use on UDP receive socket! errno: " << errno << "\n";
        return -1;
	}

	if( bind(m_rxSock, (struct sockaddr*)&m_udpAddr, sizeof(m_udpAddr) ) == -1)
	{
		printf("FATAL: Failed to bind udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		return -1;
	}

    m_manageSessionTask = Bela_createAuxiliaryTask(Session::manageSession, 50, "Manage Session", this);
    m_rxUdpTask = Bela_createAuxiliaryTask(Session::rxUdp, 50, "Receive UDP", this);

    return 0;
}

void Session::start(std::string sessId) {
    if (isActive()) {
        return;
    }
    m_sessId = sessId;
    m_terminate = false;

    if (sessId.size() == 0) {
        sessId = createSession();
        if (sessId.size() == 0) {
            return;
        }
    }

    if (joinSession(sessId) != 0) {
        return;
    }

	Bela_scheduleAuxiliaryTask(m_manageSessionTask);
	Bela_scheduleAuxiliaryTask(m_rxUdpTask);
}

void Session::stop() {
    m_terminate = true;
}

void Session::processFrame(BelaContext *context, Mixer &mixer) {
    std::lock_guard<std::mutex> guard(m_connMutex);
	for (auto &c : m_connections) {
		c->processFrame(context, mixer);
	}
}

void Session::writeToGuiBuffer() {
    std::lock_guard<std::mutex> guard(m_connMutex);
    m_wootBase->gui().sendBuffer(9, m_connections.size());
    int connectionBufferOffset = 10;
    int readBuffOffset = 0;
    for (auto &c : m_connections) {
        connectionBufferOffset += c->writeToGuiBuffer(m_wootBase->gui(), connectionBufferOffset);
        c->readFromGuiBuffer(m_wootBase->gui(), m_guiBuffIds.at(readBuffOffset++));
    }
}

bool Session::isActive() const {
    return m_udpActive || m_managerActive;
}


std::string Session::sessionId() const {
    return m_sessId;
}

void Session::manageSession(void* session) {
    Session *s = (Session*) session;
    s->m_managerActive = true;
    s->manageSessionImpl();
    {
        std::lock_guard<std::mutex> guard(s->m_connMutex);
        s->m_connections.clear();
    }
    s->m_managerActive = false;
}

void Session::rxUdp(void* session) {
    Session *s = (Session*) session;
    s->m_udpActive = true;
    s->rxUdpImpl();
    s->m_udpActive = false;
}

int getLocalIp(std::string interface, struct in_addr &addr) {
    memset(&addr, 0, sizeof(struct in_addr));

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    memset(&ifr.ifr_name, 0, IFNAMSIZ);
    strncpy(ifr.ifr_name, interface.c_str(), interface.size());

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to open IPv4 socket for local address detection. errno: " << errno << "\n";
        return -1;
    }
    int status = ioctl(sock, SIOCGIFADDR, &ifr);
    close(sock);
    if (status < 0) {
        std::cerr << "Failed to get address on device '" << interface << "'. errno: " << errno << "\n";
        return -1;
    }

    addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;

    return 0;
}

std::string Session::createSession() {
    int sock = serverConnect();
    if (sock == -1) {
        return "";
    }

    std::cout << "Creating session...\n";
    ConnPkt sendPkt;
    sendPkt.magic = SESSION_PKT_MAGIC;
    sendPkt.type = PTYPE_CREATE;
    if (send(sock, (char*)&sendPkt, sizeof(ConnPkt), 0) == -1) {
        std::cerr << "FATAL: failed to send on TCP socket! errno: " << errno << "\n";
        close(sock);
        return "";
    }

    ConnPkt pkt;
    ssize_t nBytes = read(sock, &pkt, sizeof(ConnPkt));
    if (nBytes != sizeof(ConnPkt) || pkt.magic != SESSION_PKT_MAGIC || pkt.type != PTYPE_CREATED) {
        std::cerr << "FATAL: bad response " << pkt.type << ", " << nBytes << "\n";
        close(sock);
        return "";
    }

    close(sock);
    return std::string(pkt.sid, 4);
}

int Session::joinSession(std::string sessId) {

    struct in_addr localIp;
    if (getLocalIp("eth0", localIp) != 0) {
        memset(&localIp, 0, sizeof(struct in_addr));
    }
    std::cout << "Local network IP: " << inet_ntoa(localIp) << "\n";

    int sock = serverConnect();
    if (sock == -1) {
        return -1;
    }

    std::cout << "Joining Session: " << sessId << "\n";
    ConnPkt sendPkt;
    sendPkt.magic = SESSION_PKT_MAGIC;
    sendPkt.type = PTYPE_JOIN;
    JoinData data;
    data.privatePort = htons(APP_PORT_NUMBER);
    data.privateAddr = localIp;
    memcpy(sendPkt.sid, sessId.c_str(), 4);
    memcpy(sendPkt.data, &data, sizeof(JoinData));
    if (send(sock, (char*)&sendPkt, sizeof(ConnPkt), 0) == -1) {
        std::cerr << "FATAL: failed to send on TCP socket! errno: " << errno << "\n";
		close(sock);
        return -1;
    }

    ConnPkt pkt;
    ssize_t nBytes = read(sock, &pkt, sizeof(ConnPkt));
    if (nBytes != sizeof(ConnPkt) || pkt.magic != SESSION_PKT_MAGIC) {
        std::cerr << "FATAL: bad response " << nBytes << "\n";
		close(sock);
        return -1;
    }
    if (pkt.type != PTYPE_JOINED) {
        std::cerr << "FATAL: bad response, type " << pkt.type << "\n";
		close(sock);
        return -1;
    }
    std::cout << "Got joined response\n";

    m_sessId = sessId;

    memcpy(&m_myJoinedData, pkt.data, sizeof(JoinedData));

    std::cout << "Port: " << ntohs(m_myJoinedData.privatePort) << "\n";
    std::cout << "Private IP: " << inet_ntoa(m_myJoinedData.privateAddr) << "\n";
    std::cout << "Public Port: " << ntohs(m_myJoinedData.publicPort) << "\n";
    std::cout << "Public IP: " << inet_ntoa(m_myJoinedData.publicAddr) << "\n";

    return 0;
}

int Session::serverConnect() {
    struct addrinfo *addrInfo;
    int result = getaddrinfo("wooten.smr.llc", NULL, NULL, &addrInfo);
    if (result != 0) {
		printf("ERROR: Failed to resolve hostname into address, error: %d\n", result);
		fflush(stdout);
		return -1;
    }
	struct sockaddr_in serverAddr;
    memcpy(&serverAddr, addrInfo->ai_addr, addrInfo->ai_addrlen);
	serverAddr.sin_port = htons(28314);
    freeaddrinfo(addrInfo);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "FATAL: failed to create TCP socket! errno: " << errno << "\n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != 0) {
        std::cerr << "FATAL: failed to connect TCP socket! errno: " << errno << "\n";
        return -1;
    }

    ConnPkt recvPkt;
	struct sockaddr_in peerAddr;
    socklen_t peerAddrLen = sizeof(struct sockaddr_in);
    ConnPkt sendPkt;
    sendPkt.magic = SESSION_PKT_MAGIC;
    sendPkt.version = CONN_PKT_VERSION;
    sendPkt.type = PTYPE_HOLEPUNCH;
    int tries = 0;
    int allTries = 0;
    while (true) {
        tries++;
        allTries++;
        if (tries > 5 || allTries > 500) {
            break;
        }
        ssize_t nBytes = sendto(m_rxSock,
            (char*)&sendPkt,
            sizeof(ConnPkt),
            0,
            (struct sockaddr *) &serverAddr,
            sizeof(serverAddr));
        if (nBytes < 0) {
            std::cerr << "Failed to send NAT holepunch, got errno " << errno << "\n";
            break;
        }

        nBytes = recvfrom(m_rxSock, &recvPkt, sizeof(ConnPkt), 0, (struct sockaddr *) &peerAddr, &peerAddrLen);
        
        if (nBytes < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            std::cerr << "ERROR: Failed to read server UDP message, got errno " << errno << "\n";
            break;
        }

        if (nBytes != sizeof(ConnPkt)) {
            std::cerr << "WARN: Invalid server UDP message size " << nBytes << "\n";
            tries--;
            continue;
        }

        if (peerAddr.sin_addr.s_addr != serverAddr.sin_addr.s_addr) {
            std::cerr << "WARN: Server UDP message from unexpected IP " << inet_ntoa(peerAddr.sin_addr) << " (waiting for " << inet_ntoa(serverAddr.sin_addr) << ")\n";
            tries--;
            continue;
        }

        m_sessSock = sock;
        return sock;
    }

    close(sock);
    return -1;
}

void Session::manageSessionImpl() {
    int nBytes;
    ConnPkt pkt;

    sigset_t sigMask;
    sigemptyset(&sigMask);
    sigaddset(&sigMask, SIGTERM);
    sigaddset(&sigMask, SIGINT);
    sigaddset(&sigMask, SIGQUIT);
    sigaddset(&sigMask, SIGHUP);

    ConnPkt heartbeat;
    heartbeat.magic = SESSION_PKT_MAGIC;
    heartbeat.version = CONN_PKT_VERSION;
    heartbeat.type = PTYPE_HEARTBEAT;
    memcpy(heartbeat.sid, m_sessId.c_str(), 4);
    memcpy(heartbeat.connId, pkt.connId, 6);


    JoinedData joined;

    struct timespec timeOut;
	timeOut.tv_sec = 9;
	timeOut.tv_nsec = 0;
    while (!Bela_stopRequested() && !m_terminate) {
        struct pollfd pSock;
        pSock.fd = m_sessSock;
        pSock.events = POLLIN;

        int pollResult = ppoll(&pSock, 1, &timeOut, &sigMask);
        if (pollResult == -1) {
            if (errno == EINTR) {
                std::cerr << "Session manager interrupted by signal, closing...";
                break;
            }
            std::cerr << "poll errno " << errno;
            break;
        }
        
        if (pSock.revents & POLLHUP || pSock.revents & POLLRDHUP) {
            std::cerr << "FATAL: Server closed connection...\n";
            break;
        }
        else if (pSock.revents & POLLERR) {
            std::cerr << "FATAL: Unexpected POLLERR on socket, closing...\n";
            break;
        }
        else if (pSock.revents & POLLNVAL) {
            std::cerr << "FATAL: Unexpected POLLNVAL on socket, closing...\n";
            break;
        }
        
        if (pollResult > 0) {
            nBytes = read(m_sessSock, &pkt, sizeof(ConnPkt));
            if (nBytes != sizeof(ConnPkt) || pkt.magic != SESSION_PKT_MAGIC) {
                std::cerr << "FATAL: bad response " << nBytes << "\n";
                break;
            }
            std::cout << "Response type: " << int(pkt.type) << "\n";

            if (pkt.type == PTYPE_JOINED) {
                memcpy(&joined, pkt.data, sizeof(JoinedData));
                std::string joinedId(joined.connId, 6);
                bool makeNew = true;
                {
                    std::lock_guard<std::mutex> guard(m_connMutex);
                    for (auto &c : m_connections) {
                        if (c->connId() == joinedId) {
                            makeNew = false;
                            break;
                        }
                    }
                }
                if (makeNew) {
                    Connection *c = new Connection();
                    if (c->connect(joined, m_myJoinedData, m_rxSock) != 0) {
                        delete c;
                    }
                    else {
                        std::cout << "Adding connection to " << joinedId << "\n";
                        {
                            std::lock_guard<std::mutex> guard(m_connMutex);
                            m_connections.push_back(std::unique_ptr<Connection>(c));
                            while (m_connections.size() > m_guiBuffIds.size()) {
                                m_guiBuffIds.push_back(m_wootBase->gui().setBuffer('d', 5));
                            }
                        }
                    }
                }
            }
            else if (pkt.type == PTYPE_LEFT) {
                {
                    std::lock_guard<std::mutex> guard(m_connMutex);
                    memcpy(&joined, pkt.data, sizeof(JoinedData));
                    std::string leftId(joined.connId, 6);
                    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
                        if ((*it)->connId() == leftId) {
                            m_connections.erase(it);
                            break;
                        }
                    }
                }
            }
        }
        else {
            std::cout << "Sending heartbeat...\n";
            if (send(m_sessSock, (char*)&heartbeat, sizeof(ConnPkt), 0) == -1) {
                std::cerr << "FATAL: failed to send on TCP socket! errno: " << errno << "\n";
                break;
            }
        }
    }


    close(m_sessSock);
}

void Session::rxUdpImpl() {
    printf("Starting rxUdp...\n");
	fflush(stdout);

	printf("Payload size: %d\n", sizeof(WootPkt));

    WootPkt pkt;
	struct sockaddr_in peerAddr;
	socklen_t peerSockLen = sizeof(peerAddr);

	while (!Bela_stopRequested() && !m_terminate) {

		while (true) {
			int nBytes = recvfrom(m_rxSock,
									(char*)&pkt,
									sizeof(WootPkt),
									MSG_DONTWAIT,
									(struct sockaddr *) &peerAddr,
									&peerSockLen);
			if (nBytes == 0) {
				break;
			}
			if (nBytes < 0) {
				if (errno == EAGAIN) {
					break;
				}
				printf("ERROR: Unexpected receive error, got errno %d\n", errno);
				sleep(3);
				break;
			}
			if (nBytes != sizeof(WootPkt)) {
                if (nBytes == sizeof(ConnPkt)) {
                    // maybe track that this is a holepunch packet?
                    printf("Got holepunch\n");
                    continue;
                }
				printf("Bad size %d\n", nBytes);
				continue;
			}
			if (pkt.header.magic != WOOT_PKT_MAGIC) {
				printf("Bad magic %d\n", int(pkt.header.magic));
				continue;
			}

			std::string connId(pkt.header.connId, 6);
            {
                std::lock_guard<std::mutex> guard(m_connMutex);
                for (auto &c : m_connections) {
                    if (c->connId() == connId) {
                        c->handleFrame(pkt);
                        break;
                    }
                }
            }
		}

		fflush(stdout);
		usleep(250);
	}

	printf("Ending rxUdp\n");
	fflush(stdout);
}