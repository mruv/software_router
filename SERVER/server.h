//server header file

#ifndef __SERVER_H__
#define __SERVER_H__

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

//registration structure
struct Register
{
	//client ip address
	unsigned int cIP;
	//port
	short cPort;
	//client name
	char cName[NAMESIZE];
};

typedef struct Register Register;


//password db
struct Database
{
	//client name
	char cName[NAMESIZE];
	//client password
	char cPwd[NAMESIZE];
};

typedef struct Database Database;

//////////////////////////////////////
////////////////functions/////////////


//initialize password db
Database* init_database(int*);

//send packet
void _send(Packet, struct sockaddr*, int);

//receive packet
Packet _recv(int);

//is this client registered?
int is_registered(Register*, int, const char*, short, unsigned int);

//authenticate client
int auth(Database*, int, char*, char*);

#endif