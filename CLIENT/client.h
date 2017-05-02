
#ifndef __CLIENT_H__
#define __CLIENT_H__

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

#include <netinet/in.h>

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
	char cN[NAMESIZE];
	//packet payload
	char pData[DATASIZE];//packet data
};

typedef struct Packet Packet;

struct Args
{
	//client port ==> 'this' client
	short cP;
	//client name ==> 'this' client
	char cN[NAMESIZE];
	//client password
	char cPwd[NAMESIZE];
	//server ip
	char sIP[INET_ADDRSTRLEN];
	//server port
	short sP;
	//router ip
	char rIP[INET_ADDRSTRLEN];
	//router port
	short rP;
	//name of client to send packets to
	char ocN[NAMESIZE];
	//port of that client
	short ocP;
};

typedef struct Args Args;

//send packet
void _send(Packet, struct sockaddr*, int);

//receive packet
Packet _recv(int);

//parse command line args
Args* parse_args(int,char**);

//help page
void _usage(const char*);


#endif