#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>

typedef struct {
    in_port_t port;
    struct in_addr peerAddr;
} PortAssignment;

class PortBroker {
public:
    static int getPortAssignmentForHost(const char * host, PortAssignment &assignment);
    static void assignPorts(void *);

private:
    static int createTcpSocket(int timeoutSec = 1, int timeoutUSec = 0);
    static int validateAssignment(const PortAssignment &proposed, PortAssignment &existing);
    static int proposeAssignment(PortAssignment &assignment, uint16_t portOffset);
};