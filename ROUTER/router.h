
#ifndef __ROUTER_H__
#define __ROUTER_H__

//define packet type constants
#define REG   0
#define REQ   1
#define CONF  2
#define REPLY 3
#define ERROR 4
#define DATA  5
#define ACK   6

#define NAMESIZE 16
#define DATASIZE 64

#include <netinet/in.h>//sockaddr

//data packet
struct Packet 
{
	//source ip
	unsigned int sIP;
	//source port
	short sP;
	//destination ip
	unsigned int dIP;
	//destination port
	short dP;
	//packet type
	short pType;
	//sender client name
	char cName[NAMESIZE];
	//packet payload
	char pData[DATASIZE];
};

typedef struct Packet Packet;

//command line arguments
struct Args
{
	//this router's port
	short rP;
	//neighbouring router ip
	unsigned int rIP;
	//neighbouring router port
	short orP;
	//neighbours file
	char n[NAMESIZE];
};

typedef struct Args Args;

//neighbour
struct Neighbour
{
	//ip
	unsigned int nIP;
	//port
	short nP;
};

typedef struct Neighbour Neighbour;

/********************************************/
/**************functions*********************/

//send packet
void _send(Packet, struct sockaddr*, int);

//receive packet
Packet _recv(int);

//parse command line args
void parse_args(int,char* [],Args*);

//print help
void _usage(const char*);

int neighbour(unsigned int ,short ,Neighbour* );
#endif