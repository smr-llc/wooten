
#pragma once

#define SESSION_PKT_MAGIC 0x77
#define PTYPE_HEARTBEAT 0x01
#define PTYPE_HOLEPUNCH 0x02

// client -> server
#define PTYPE_CREATE 0x10
#define PTYPE_JOIN 0x11
#define PTYPE_LEAVE 0x12

// server -> client
#define PTYPE_CREATED 0x20
#define PTYPE_JOINED 0x21
#define PTYPE_LEFT 0x22

#define PTYPE_ERR_NOT_FOUND 0x34

#define CONN_PKT_VERSION 0x01
#define CONN_PKT_DATA_LEN 51

#include <stdint.h>
#include <arpa/inet.h>

typedef struct {
    uint8_t magic;
    uint8_t version;
    uint8_t type;
    char sid[4];
    char connId[6];
    char data[CONN_PKT_DATA_LEN];
} ConnPkt;

typedef struct {
    in_port_t port;
    struct in_addr privateAddr;
} JoinData;

typedef struct {
    in_port_t port;
    struct in_addr privateAddr;
    struct in_addr publicAddr;
    char connId[6];
} JoinedData;