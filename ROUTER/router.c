#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <getopt.h>

#include "router.h"

int main(int argc, char *argv[])
{
	//parse command  line args
	printf(">> parsing command line args...\n");
	if(argc != 5)
	{
		_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	//connected to this router
	char rtrIp[INET_ADDRSTRLEN];
	strncpy(rtrIp,argv[2],strlen(argv[2]));

	short rPort = (short)atoi(argv[3]);

	Neighbour *nghbr;
	printf(">> initializing neighbours list...\n");
	//initialize neighbours list
	FILE *stream = fopen(argv[4],"r");
	if(stream == NULL)
	{
		perror(">> fopen");//error
		printf(">> exiting...\n");
		exit(EXIT_FAILURE);
	}

	//allocate memory
	nghbr = (Neighbour*)malloc(sizeof(Neighbour)*2);
	char line[DATASIZE];
	char tmp[NAMESIZE];
	int len, from = 0,i = 0;

	while(!feof(stream))
	{
		//read a line
		memset(line,'\0',sizeof(line));
		fgets(line,sizeof(line),stream);
		if(strlen(line) < 5) break;
		//get ip
		memset(tmp,'\0',sizeof(tmp));

		len = strcspn(line + from," ");
		strncpy(tmp,line + from,len);
		nghbr[i].nIP = inet_addr(tmp);

		from += len + 1;
		//get port
		len = strcspn(line + from,"\n");
		strncpy(tmp,line + from,len);
		nghbr[i].nP = atoi(tmp);
		++i;
	}

	printf(">> initialized\n");
	fclose(stream);

	//socket fd
	int sock, opt = 1;
	socklen_t size = sizeof(opt);
	struct sockaddr_in my_addr;//ip address of this PC

	//create socket
	printf(">> creating socket...\n");
	if((sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
	{
		//error
		perror(">> socket");
		close(sock);
		exit(EXIT_FAILURE);
	}

	//set socket options
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,size) != 0)
	{
		//error
		perror(">> setsockopt");
		close(sock);
		exit(EXIT_FAILURE);
	}
	//initialize client socket address parameters
	memset(&my_addr,'\0',sizeof(struct sockaddr_in));
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	my_addr.sin_port = htons((short)atoi(argv[1]));
	my_addr.sin_family = AF_INET;

	if(bind(sock,(struct sockaddr*)&my_addr,sizeof(struct sockaddr)) != 0)
	{
		//error
		perror(">> bind");
		exit(EXIT_FAILURE);
	}

	printf(">> socket created\n");
	Packet packet;
	int loc;

	//peer address
	struct sockaddr_in peer;
	memset(&peer,'\0',sizeof(struct sockaddr_in));
	peer.sin_family = AF_INET;

	//handle packets
	while(1)
	{
		//get a packet from a router or client
		packet = _recv(sock);

		if((loc = neighbour(packet.dIP,packet.dP,nghbr)) == -1)
		{
			//forward packet to the next connected router
			peer.sin_addr.s_addr = inet_addr(rtrIp);
			peer.sin_port = htons(rPort);
		}
		else
		{
			//forward to client
			peer.sin_addr.s_addr = nghbr[loc].nIP;
			peer.sin_port = nghbr[loc].nP;
		}
		//send packet
		_send(packet,(struct sockaddr*)&peer,sock);
	}


	free(nghbr);
	close(sock);
	return 0;
}

//print usage
void _usage(const char* n)
{
	printf("usage : %s port other-router-ip other-router-port neighbours-list-file\n",n);
}

//is this packet meant for a neighbouring host?
int neighbour(unsigned int h,short p,Neighbour* n)
{
	//search
	for(int i = 0;i < 2;++i)
	{
		if(n[i].nIP == h && n[i].nP == p)
		{
			return i;
		}
	}
	return -1;//not a neighbour, forward packet to the neighbouring router
}

//send a packet
void _send(Packet p, struct sockaddr* peer, int s)
{
	//declare a character buffer
	char buffer[sizeof(Packet)];
	//temporary char buffer
	char tmp[sizeof(unsigned int)];

	memset(buffer,'\0',sizeof(buffer));//clear memory

	//build a buffer from the Packet object
	//separate the packet fields using ':'
	//append the source ip
	memset(tmp,'\0',sizeof(tmp));
	sprintf(tmp,"%d:",p.sIP);
	strcat(buffer,tmp);

	//append source port
	memset(tmp,'\0',sizeof(tmp));
	sprintf(tmp,"%d:",p.sP);
	strcat(buffer,tmp);

	//append destination ip
	memset(tmp,'\0',sizeof(tmp));
	sprintf(tmp,"%d:",p.dIP);
	strcat(buffer,tmp);

	//append destination port
	memset(tmp,'\0',sizeof(tmp));
	sprintf(tmp,"%d:",p.dP);
	strcat(buffer,tmp);

	//append packet type
	memset(tmp,'\0',sizeof(tmp));
	sprintf(tmp,"%d:",p.pType);
	strcat(buffer,tmp);

	//append client name
	strcat(buffer,p.cName);
	//append ':'
	strcat(buffer,":");
	//append packet payload / actual data
	strcat(buffer,p.pData);

	strcat(buffer,"\n\r");
	//send packet
	sendto(s,buffer,strlen(buffer),0,peer,sizeof(Packet));
}


//receive a packet
Packet _recv(int s)
{
	//declare Packet object
	Packet p;
	struct sockaddr peerAddr;
	socklen_t size = sizeof(struct sockaddr);
	//receive buffer
	char buffer[sizeof(Packet)];
	//temp buffer
	char tmp[sizeof(unsigned int)];
	int len, from = 0;

	//receive
	recvfrom(s,buffer,sizeof(buffer),0,&peerAddr,&size);

	//break down buffer into packet
	//fields delimited by ":"
	memset(tmp,'\0',sizeof(tmp));
	len = strcspn(buffer + from,":");

	//get source ip
	strncpy(tmp,buffer + from,len);
	p.sIP = atoi(tmp);
	from += len + 1;

	//get source port
	memset(tmp,'\0',sizeof(tmp));
	len = strcspn(buffer + from,":");
	strncpy(tmp,buffer + from,len);
	p.sP = atoi(tmp);
	from += len + 1;

	//get destination ip
	memset(tmp,'\0',sizeof(tmp));
	len = strcspn(buffer + from,":");
	strncpy(tmp,buffer + from,len);
	p.dIP = atoi(tmp);
	from += len + 1;

	//get destination port
	memset(tmp,'\0',sizeof(tmp));
	len = strcspn(buffer + from,":");
	strncpy(tmp,buffer + from,len);
	p.dP = atoi(tmp);
	from += len + 1;

	//get packet type
	memset(tmp,'\0',sizeof(tmp));
	len = strcspn(buffer + from,":");
	strncpy(tmp,buffer + from,len);
	p.pType = atoi(tmp);
	from += len + 1;

	//get client name
	len = strcspn(buffer + from,":");
	strncpy(p.cName,buffer + from,len);
	from += len + 1;

	//get packet data
	len = strcspn(buffer + from,"\n\r");
	strncpy(p.pData,buffer + from,len);

	return p;
}
