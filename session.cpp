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

	struct sockaddr_in addr;
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(42000);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(m_rxSock, (struct sockaddr*)&addr, sizeof(addr) ) == -1)
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

	Bela_scheduleAuxiliaryTask(m_manageSessionTask);
	Bela_scheduleAuxiliaryTask(m_rxUdpTask);
}

void Session::stop() {
    m_terminate = true;
}

void Session::processFrame(BelaContext *context, Mixer &mixer) {
    std::lock_guard<std::mutex> guard(m_connMutex);
	for (auto &pair : m_connections) {
		pair.second->processFrame(context, mixer);
	}
}

void Session::writeToGuiBuffer() {
    std::lock_guard<std::mutex> guard(m_connMutex);
    m_wootBase->gui().sendBuffer(9, m_connections.size());
    int connectionBufferOffset = 10;
    for (auto &pair : m_connections) {
        int increment = pair.second->writeToGuiBuffer(m_wootBase->gui(), connectionBufferOffset);
        pair.second->readFromGuiBuffer(m_wootBase->gui());
        connectionBufferOffset += increment;
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
        for (auto &pair : s->m_connections) {
            delete pair.second;
        }
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

void Session::manageSessionImpl() {
    struct in_addr localIp;
    if (getLocalIp("eth0", localIp) != 0) {
        memset(&localIp, 0, sizeof(struct in_addr));
    }
    std::cout << "Local network IP: " << inet_ntoa(localIp) << "\n";

    struct sockaddr_in peerAddr;
	socklen_t peerAddrLen = sizeof(peerAddr);

    struct addrinfo *addrInfo;
    int result = getaddrinfo("wooten.smr.llc", NULL, NULL, &addrInfo);
    if (result != 0) {
		printf("ERROR: Failed to resolve hostname into address, error: %d\n", result);
		fflush(stdout);
		this->stop();
        return;
    }
    memcpy(&peerAddr, addrInfo->ai_addr, addrInfo->ai_addrlen);
	peerAddr.sin_port = htons(28314);
    freeaddrinfo(addrInfo);

	int sock;
    int nBytes;
    ConnPkt pkt;
    ConnPkt sendPkt;
    
    if (m_sessId.size() == 0) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "FATAL: failed to create TCP socket! errno: " << errno << "\n";
            this->stop();
            return;
        }


        if (connect(sock, (struct sockaddr*)&peerAddr, peerAddrLen) != 0) {
            std::cerr << "FATAL: failed to connect TCP socket! errno: " << errno << "\n";
            this->stop();
            return;
        }

        std::cout << "Creating session...\n";
        sendPkt.magic = SESSION_PKT_MAGIC;
        sendPkt.type = PTYPE_CREATE;
        if (send(sock, (char*)&sendPkt, sizeof(ConnPkt), 0) == -1) {
            std::cerr << "FATAL: failed to send on TCP socket! errno: " << errno << "\n";
            close(sock);
            this->stop();
            return;
        }

        nBytes = read(sock, &pkt, sizeof(ConnPkt));
        if (nBytes != sizeof(ConnPkt) || pkt.magic != SESSION_PKT_MAGIC || pkt.type != PTYPE_CREATED) {
            std::cerr << "FATAL: bad response " << pkt.type << ", " << nBytes << "\n";
            close(sock);
            this->stop();
            return;
        }
        close(sock);
        m_sessId = std::string(pkt.sid, 4);
    }
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "FATAL: failed to create TCP socket! errno: " << errno << "\n";
        this->stop();
        return;
    }
    if (connect(sock, (struct sockaddr*)&peerAddr, peerAddrLen) != 0) {
        std::cerr << "FATAL: failed to connect TCP socket! errno: " << errno << "\n";
		close(sock);
        this->stop();
        return;
	}

    std::cout << "Joining Session: " << m_sessId << "\n";
    sendPkt.magic = SESSION_PKT_MAGIC;
    sendPkt.type = PTYPE_JOIN;
    JoinData data;
    data.port = htons(42000);
    data.privateAddr = localIp;
    memcpy(sendPkt.sid, m_sessId.c_str(), 4);
    memcpy(sendPkt.data, &data, sizeof(JoinData));
    if (send(sock, (char*)&sendPkt, sizeof(ConnPkt), 0) == -1) {
        std::cerr << "FATAL: failed to send on TCP socket! errno: " << errno << "\n";
		close(sock);
        this->stop();
        return;
    }

    nBytes = read(sock, &pkt, sizeof(ConnPkt));
    if (nBytes != sizeof(ConnPkt) || pkt.magic != SESSION_PKT_MAGIC) {
        std::cerr << "FATAL: bad response " << nBytes << "\n";
		close(sock);
        this->stop();
        return;
    }
    if (pkt.type != PTYPE_JOINED) {
        std::cerr << "FATAL: bad response, type " << pkt.type << "\n";
		close(sock);
        this->stop();
        return;
    }
    std::cout << "Got joined response\n";

    JoinedData joined;
    memcpy(&joined, pkt.data, sizeof(JoinedData));
    memcpy(&m_myJoinedData, pkt.data, sizeof(JoinedData));

    std::cout << "Port: " << ntohs(joined.port) << "\n";
    std::cout << "Private IP: " << inet_ntoa(joined.privateAddr) << "\n";
    std::cout << "Public IP: " << inet_ntoa(joined.publicAddr) << "\n";

    sigset_t sigMask;
    sigemptyset(&sigMask);
    sigaddset(&sigMask, SIGINT);
    sigaddset(&sigMask, SIGQUIT);
    sigaddset(&sigMask, SIGHUP);

    ConnPkt heartbeat;
    heartbeat.magic = SESSION_PKT_MAGIC;
    heartbeat.version = CONN_PKT_VERSION;
    heartbeat.type = PTYPE_HEARTBEAT;
    memcpy(heartbeat.sid, m_sessId.c_str(), 4);
    memcpy(heartbeat.connId, pkt.connId, 6);

    struct timespec timeOut;
	timeOut.tv_sec = 9;
	timeOut.tv_nsec = 0;
    while (!Bela_stopRequested() && !m_terminate) {
        struct pollfd pSock;
        pSock.fd = sock;
        pSock.events = POLLIN;

        int pollResult = ppoll(&pSock, 1, &timeOut, &sigMask);
        if (pollResult == -1) {
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
            nBytes = read(sock, &pkt, sizeof(ConnPkt));
            if (nBytes != sizeof(ConnPkt) || pkt.magic != SESSION_PKT_MAGIC) {
                std::cerr << "FATAL: bad response " << nBytes << "\n";
                break;
            }
            std::cout << "Response type: " << int(pkt.type) << "\n";

            if (pkt.type == PTYPE_JOINED) {
                memcpy(&joined, pkt.data, sizeof(JoinedData));
                std::string joinedId(joined.connId, 6);
                std::map<std::string,Connection*>::iterator it;
                bool makeNew = false;
                {
                    std::lock_guard<std::mutex> guard(m_connMutex);
                    it = m_connections.find(joinedId);
                    makeNew = (it == m_connections.end());
                }
                if (makeNew) {
                    Connection *c = new Connection();
                    if (c->connect(joined, m_myJoinedData) != 0) {
                        delete c;
                    }
                    else {
                        c->initializeReadBuffer(m_wootBase->gui());
                        {
                            std::lock_guard<std::mutex> guard(m_connMutex);
                            m_connections[joinedId] = c;
                        }
                    }
                }
            }
            else if (pkt.type == PTYPE_LEFT) {
                Connection *toDelete = nullptr;
                {
                    std::lock_guard<std::mutex> guard(m_connMutex);
                    memcpy(&joined, pkt.data, sizeof(JoinedData));
                    std::string leftId(joined.connId, 6);
                    auto it = m_connections.find(leftId);
                    if (it != m_connections.end()) {
                        {
                            m_connections.erase(it);
                        }
                        toDelete = it->second;
                    }
                }
                if (toDelete) {
                    delete toDelete;
                }
            }
        }
        else {
            std::cout << "Sending heartbeat...\n";
            if (send(sock, (char*)&heartbeat, sizeof(ConnPkt), 0) == -1) {
                std::cerr << "FATAL: failed to send on TCP socket! errno: " << errno << "\n";
                break;
            }
        }
    }


    close(sock);
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
				printf("Bad size %d\n", nBytes);
				continue;
			}
			if (pkt.header.magic != WOOT_PKT_MAGIC) {
				printf("Bad magic %d\n", int(pkt.header.magic));
				continue;
			}

			std::string connId(pkt.header.connId, 6);
            printf("got Pkt from %s\n", connId.c_str());
            {
                std::lock_guard<std::mutex> guard(m_connMutex);
                for (auto pair : m_connections) {
                    printf("conn key %s\n", pair.first.c_str());
                }
                auto it = m_connections.find(connId);
                if (it != m_connections.end()) {
                    printf("processing Pkt from %s\n", connId.c_str());
                    it->second->handleFrame(pkt);
                }
            }
		}

		fflush(stdout);
		usleep(250);
	}

	printf("Ending rxUdp\n");
	fflush(stdout);
}