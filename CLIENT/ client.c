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

#include "client.h"



int main(int argc, char *argv[])
{
	printf(">> parsing command line arguments...\n");
	//check argc
	if(argc != 19)
	{
		//print usage
		_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	Args *args = parse_args(argc,argv);

	if(args == NULL)
	{
		//failed to parse
		printf(">> failed\n");
		_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	printf(">> succeeded\n");

	//socket fd
	int sock, opt = 1;
	socklen_t size = sizeof(opt);
	struct sockaddr_in my_addr;//ip address of this PC
	struct sockaddr_in peer_addr;//server, router

	struct in_addr src, dest;

	//create socket
	printf(">> creating socket...\n");
	if((sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
	{
		//error
		perror(">> socket");
		free(args);
		exit(EXIT_FAILURE);
	}

	//set socket options
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,size) != 0)
	{
		//error
		perror(">> setsockopt");
		free(args);
		close(sock);
		exit(EXIT_FAILURE);
	}

	//initialize client socket address parameters
	memset(&my_addr,'\0',sizeof(struct sockaddr_in));
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	my_addr.sin_port = htons(args->cP);
	my_addr.sin_family = AF_INET;

	if(bind(sock,(struct sockaddr*)&my_addr,sizeof(struct sockaddr)) != 0)
	{
		//error
		perror(">> bind");
		exit(EXIT_FAILURE);
	}

	printf(">> socket created\n");

	//initialize server address
	memset(&peer_addr,'\0',sizeof(struct sockaddr_in));
	peer_addr.sin_addr.s_addr = inet_addr(args->sIP);
	peer_addr.sin_port = htons(args->sP);
	peer_addr.sin_family = AF_INET;

	//time to send REG packet
	Packet p_to, p_from;

	memset(&p_to,'\0',sizeof(Packet));
	//initialize packet
	p_to.sIP = my_addr.sin_addr.s_addr;
	p_to.sP = ntohs(my_addr.sin_port);
	p_to.dIP = peer_addr.sin_addr.s_addr;
	p_to.dP = ntohs(peer_addr.sin_port);
	p_to.pType = REG;
	strncpy(p_to.cN,args->cN,strlen(args->cN));
	strncpy(p_to.pData,args->cPwd,strlen(args->cPwd));

	printf(">> sending a REG packet to server [ %s : %d ]\n",args->sIP,args->sP);
	_send(p_to,(struct sockaddr*)&peer_addr,sock);

	printf("   sourceIp : %s\n   sourcePort : %d\n"
		   "   destIp : %s\n   destPort : %d\n"
		   "   type : %s\n" 
		   "   clientName : \"%s\"\n"
		   "   password : \"%s\"\n",inet_ntoa(my_addr.sin_addr),p_to.sP,
		   inet_ntoa(peer_addr.sin_addr),p_to.dP,"REG",p_to.cN,p_to.pData);

	p_from = _recv(sock);

	if(p_from.pType == CONF)
	{
		//confirmation message
		printf(">> received a CONF packet from server\n");
	}
	else{
		printf(">> received an ERROR packet from server\n");
		printf("   error_message : %s\n", p_from.pData);
		//exit
		printf(">> exiting...\n");
		close(sock);
		free(args);
		exit(EXIT_FAILURE);
	}

	src.s_addr = p_from.sIP;
	dest.s_addr = p_from.dIP;

	//client refistered on server
	printf("   sourceIp : %s   sourcePort : %d\n"
		   "   destIp : %s   destPort : %d\n"
		   "   type : %s\n"
		   "   clientName : \"%s\"\n"
		   "   password : \"%s\"\n",inet_ntoa(src),p_from.sP,
		   inet_ntoa(dest),p_from.dP,"CONF",p_from.cN,p_from.pData);

	//send a REQ packet to server
	memset(p_to.pData,'\0',sizeof(p_to.pData));
	strncpy(p_to.pData,args->ocN,strlen(args->ocN));
	p_to.pType = REQ;

	printf(">> sending a REQ packet to server [ %s : %d ]\n", args->sIP,args->sP);

	_send(p_to,(struct sockaddr*)&peer_addr,sock);
	printf("   sourceIp : %s\n   sourcePort:%d\n"
		   "   destIp : %s\n   destPort:%d\n"
		   "   type : %s\n"
		   "   clientName : \"%s\"\n"
		   "   other-client-name : \"%s\"\n",inet_ntoa(my_addr.sin_addr),p_to.sP,
		   inet_ntoa(peer_addr.sin_addr),p_to.dP,"REQ",p_to.cN,p_to.pData);

	//receive a packet from server (can be ERROR / REPLY)
	p_from = _recv(sock);

	if(p_from.pType == ERROR)
	{
		printf(">> received an ERROR packet from server\n");
		printf("   error_message : %s\n", p_from.pData);
		//exit
		printf(">> exiting...\n");
		close(sock);
		free(args);
		exit(EXIT_FAILURE);		
	}
	else if(p_from.pType != REPLY)
	{
		printf(">> packet type unexpected\n>> exiting...");
		close(sock);
		free(args);
		exit(EXIT_FAILURE);
	}

	//REPLY packet received

	printf(">> received a REPLY packet from server\n");
	src.s_addr = p_from.sIP;
	dest.s_addr = p_from.dIP;

	//other client's ip address retrived
	printf("   sourceIp : %s   sourcePort : %d\n"
		   "   destIp : %s   destPort : %d\n"
		   "   type : %s\n"
		   "   clientName : \"%s\"\n"
		   "   other-client-ip : \"%s\"\n",inet_ntoa(src),p_from.sP,
		   inet_ntoa(dest),p_from.dP,"REPLY",p_from.cN,p_from.pData);

	//time to send packets to the other client via a router
	//open a file to send 
	printf(">> opening a file to send to the other client [ %s ]\n", args->ocN);
	FILE *stream = fopen("send.txt","r");
	if(stream == NULL)
	{
		perror(">> fopen");
		printf(">> exiting...\n");
		free(args);
		close(sock);
		exit(EXIT_FAILURE);
	}

	printf(">> opened\n");

	char line[64];
	//initialize peer addr to the address of the router this client
	//is directly connected to
	memset(&peer_addr,'\0',sizeof(struct sockaddr_in));
	peer_addr.sin_addr.s_addr = inet_addr(args->rIP);
	peer_addr.sin_port = htons(args->rP);
	peer_addr.sin_family = AF_INET;

	//the following fields of the packet to be send will remain constant
	memset(&p_to,'\0',sizeof(Packet));

	p_to.sIP = my_addr.sin_addr.s_addr;
	p_to.sP = ntohs(my_addr.sin_port);
	p_to.dIP = inet_addr(p_from.pData);
	p_to.dP = args->ocP;
	p_to.pType = DATA;
	strncpy(p_to.pData,args->cN,strlen(args->cN));

	src.s_addr = p_to.sIP;
	dest.s_addr = p_to.dIP;
	//read all data, send line after line
	while(!feof(stream))
	{
		memset(line,'\0',sizeof(line));
		//read line
		fgets(line,sizeof(line),stream);

		if(strlen(line) < 5)//no data
		{
			break;
		}
		memset(p_to.pData,'\0',sizeof(p_to.pData));
		strncpy(p_to.pData,line,strlen(line));
		//send packet
		printf(">> sending a DATA packet to another client [ %s ]\n", args->ocN);
		_send(p_to,(struct sockaddr*)&peer_addr,sock);
		printf("   sourceIp : %s\n   sourcePort : %d\n"
			   "   destIp : %s\n   destPort : %d\n"
			   "   type : %s\n"
			   "   clientName : \"%s\"\n"
			   "   data : \"%s\"\n",inet_ntoa(src),p_to.sP,
			   inet_ntoa(dest),p_to.dP,"DATA",p_to.cN,p_to.pData);

		//receive ACK or DATA packet
		p_from = _recv(sock);

		if(p_from.pType == ACK)//acknowledgement
		{
			//print
			printf(">> received a ACK packet from client [ %s ] via router [ %s : %d ]\n",
				args->ocN, inet_ntoa(peer_addr.sin_addr),ntohs(peer_addr.sin_port));
			printf("   sourceIp : %s   sourcePort : %d\n"
				   "   destIp : %s   destPort : %d\n"
				   "   type : %s\n"
				   "   clientName : \"%s\"\n"
				   "   data : \"%s\"\n",inet_ntoa(src),p_from.sP,
				   inet_ntoa(dest),p_from.dP,"ACK",p_from.cN,p_from.pData);
			continue;//send another packet
		}
		else if(p_from.pType == DATA)
		{
			//send acknowledgement
			memset(p_to.pData,'\0',sizeof(p_to.pData));
			strcpy(p_to.pData,"acknowledgement");
			//set packet type to ACK
			p_to.pType = ACK;
			//send
			printf(">> sending an ACK packet to another client [ %s ] via router [ %s : %d ]\n",
				args->ocN,inet_ntoa(peer_addr.sin_addr),ntohs(peer_addr.sin_port));
			_send(p_to,(struct sockaddr*)&peer_addr,sock);
			printf("   sourceIp : %s\n   sourcePort : %d\n"
				   "   destIp : %s\n   destPort : %d\n"
				   "   type : %s\n"
				   "   clientName : \"%s\"\n"
				   "   data : \"%s\"\n",inet_ntoa(src),p_to.sP,
				   inet_ntoa(dest),p_to.dP,"DATA",p_to.cN,p_to.pData);
		}
	}

	fclose(stream);
	free(args);
	close(sock);
	return 0;
}

//parse args
Args* parse_args(int argc, char** argv)
{
	Args *args = (Args*)malloc(sizeof(Args));
	struct option long_opts[] = {
		{"client-port",required_argument,0,'p'},
		{"client-name",required_argument,0,'n'},
		{"client-pwd",required_argument,0,'k'},
		{"server-ip",required_argument,0,'S'},
		{"server-port",required_argument,0,'s'},
		{"router-ip",required_argument,0,'R'},
		{"router-port",required_argument,0,'r'},
		{"other-client-name",required_argument,0,'N'},
		{"other-client-port",required_argument,0,'P'}
	};


	int opt = 0;
	int long_index = 0;

	//parse
	while((opt = getopt_long(argc, argv,"",
		long_opts, &long_index )) != -1)
	{
		//switch
		switch(opt)
		{
			case 'p': args->cP = atoi(optarg);
			break;
			case 'n': strncpy(args->cN,optarg,strlen(optarg));
			break;
			case 'k': strncpy(args->cPwd,optarg,strlen(optarg));
			break;
			case 'S': strncpy(args->sIP,optarg,strlen(optarg));
			break;
			case 's': args->sP = atoi(optarg);
			break;
			case 'R': strncpy(args->rIP,optarg,strlen(optarg));
			break;
			case 'r': args->rP = atoi(optarg);
			break;
			case 'N': strncpy(args->ocN,optarg,strlen(optarg));
			break;
			case 'P': args->ocP = atoi(optarg);
			break;

			default:
			return NULL;
		}//switch
	}//while
	return args;
}

//print usage message
void _usage(const char* n)
{
	printf("usage : %s [ --%-30s clientPort ]\n"
		   "              [ --%-30s clientName ]\n"
		   "              [ --%-30s clientPassword ]\n"
		   "              [ --%-30s serverIpAddress ]\n"
		   "              [ --%-30s serverPort ]\n"
		   "              [ --%-30s routerIpAddress ]\n"
		   "              [ --%-30s routerPort ]\n"
		   "              [ --%-30s otherClientName ]\n"
		   "              [ --%-30s otherClientPort ]\n",
		   n,"client-port","client-name","client-pwd",
		   "server-ip","server-port","router-ip",
		   "router-port","other-client-name",
		   "other-client-port");
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
	strcat(buffer,p.cN);
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

	memset(&p,'\0',sizeof(Packet));
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
	strncpy(p.cN,buffer + from,len);
	from += len + 1;

	//get packet data
	len = strcspn(buffer + from,"\n\r");
	strncpy(p.pData,buffer + from,len);

	return p;
}
