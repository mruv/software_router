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

#include "server.h"


//main function
int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		//error
		fprintf(stderr, "Usage : %s portNumber\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	//server port
	short port = atoi(argv[1]);

	printf(">> initializing password database...\n");

	int dbS;//database size
	//initialize database
	Database *dB = init_database(&dbS);
	Register *reg;

	if(dB == NULL)
	{
		//database not initialized
		//print error message
		fprintf(stderr, ">> %s\n",strerror(errno));;
		//exit
		exit(EXIT_FAILURE);
	}
	printf("   initialized\n");

	//print database contents
	printf("   %-20s%s\n","CLIENT NAME","PASSWORD");
	for(int i = 0;i < dbS;++i)
	{
		printf("   %-20s%s\n",dB[i].cName,dB[i].cPwd);
	}

	printf("\n");

	//create a udp server socket
	struct sockaddr_in svr_addr;
	int sock;

	printf(">> opening a UDP server socket...\n");
	if((sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
	{
		//failed to create socket
		perror(">> socket");
		exit(EXIT_FAILURE);
	}

	//set socket options
	//enable address reuse
	int opt = 1;
	socklen_t opt_len = sizeof(int);
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,opt_len) != 0)
	{
		//failed to set
		perror(">> setsockopt");
	}

	//bind socket to ip and port
	memset(&svr_addr,'\0',sizeof(struct sockaddr_in));
	svr_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//bind to all interfaces i.e eth0, wlan0 and lo
	svr_addr.sin_port = htons(port);
	svr_addr.sin_family = AF_INET;

	if(bind(sock,(struct sockaddr*)&svr_addr,sizeof(struct sockaddr)) != 0)
	{
		//bind() failed
		perror(">> bind");
		exit(EXIT_FAILURE);
	}

	//server socket ready for receiving and sending data
	printf("   socket opened\n");

	//register clients
	Packet p_from, p_to;
	reg = (struct Register*)malloc(sizeof(Register)*10);//register upto 10 clients

	struct sockaddr_in to_addr;
	struct in_addr src, dest;//source and destination IPs
	int registered = 0;//no. of registered clients

	while(1)
	{
		//get packet from a client
		printf(">> waiting for a REG packet from a client...\n");

		//clear memory
		memset(&p_from,'\0',sizeof(Packet));
		p_from = _recv(sock);

		if(p_from.pType == REG)
		{
			printf(">> received a REG packet from %s\n",p_from.cName);
			//packet data
			src.s_addr = p_from.sIP;
			dest.s_addr = p_from.dIP;
			printf("   sourceIp : %s\n   sourcePort : %d\n"
				   "   destIp : %s\n   destPort : %d\n"
				   "   type : REG\n"
				   "   clientName : \"%s\"\n"
				   "   password : \"%s\"\n",inet_ntoa(src),p_from.sP,
				   inet_ntoa(dest),p_from.dP,p_from.cName,p_from.pData);
		}
		else
		{
			fprintf(stderr, "%s\n",">> Unexpected packet type");
			continue;
		}

		//prepare packet to send to client (either ERROR or CONF)
		memset(&p_to,'\0',sizeof(Packet));
		p_to.sIP = svr_addr.sin_addr.s_addr;
		p_to.sP = ntohs(svr_addr.sin_port);
		p_to.dIP = p_from.sIP;//set dest ip to the ip it came from
		p_to.dP = p_from.sP;
		strncpy(p_to.cName,p_from.cName,strlen(p_from.cName));

		//source ip infor
		memset(&to_addr,'\0',sizeof(struct sockaddr_in));
		to_addr.sin_addr.s_addr = (in_addr_t)p_from.sIP;
		to_addr.sin_port = htons(p_from.sP);
		to_addr.sin_family = AF_INET;

		//ip addresses
		src.s_addr = p_to.sIP;
		dest.s_addr = p_to.dIP;

		//authenticate client
		if(auth(dB,dbS,p_from.cName,p_from.pData) == 0)
		{
		    //register client
			reg[registered].cIP = p_from.sIP;
			reg[registered].cPort = p_from.sP;
			strncpy(reg[registered].cName,p_from.cName,strlen(p_from.cName));

			//set packet type to CONF
			p_to.pType = CONF;
			//payload becomes the payload of the received packet
			strncpy(p_to.pData,p_from.pData,strlen(p_from.pData));
			//update counter
			++registered;
			printf(">> sending a CONF packet...\n");

		}

		else
		{
		//set packet type to ERROR
			p_to.pType = ERROR;
			//payload is the error message
			memset(p_to.pData,'\0',sizeof(p_to.pData));
			char msg[] = "Invalid user name or password";
			strncpy(p_to.pData,msg,strlen(msg));
			printf(">> invalid credentials. Sending an ERROR packet...\n");

			//send packet
			_send(p_to,(struct sockaddr*)&to_addr,sock);
			//packet infor
			printf("   sourceIp : %s\n   sourcePort : %d\n"
				   "   destIp : %s\n   destPort : %d\n"
				   "   type : ERROR\n"
				   "   clientName : \"%s\"\n"
				   "   error_message : \"%s\"\n",inet_ntoa(src),p_to.sP,
				   inet_ntoa(dest),p_to.dP,p_to.cName,p_to.pData);
			continue;	
		}

		//if execution gets this far, the client is registered
		//lets send a CONF packet
		
		//send packet
		_send(p_to,(struct sockaddr*)&to_addr,sock);
		//packet infor
		printf("   sourceIp : %s\n   sourcePort : %d\n"
			   "   destIp : %s\n   destPort : %d\n"
			   "   type : CONF\n"
			   "   clientName : \"%s\"\n"
			   "   data : \"%s\"\n",inet_ntoa(src),p_to.sP,
				   inet_ntoa(dest),p_to.dP,p_to.cName,p_to.pData);

		//receive a REQ packet from the client
		p_from = _recv(sock);

		int found = 0;
		int i;

		if(p_from.pType == REQ)
		{//a request
		//packet infor
			printf(">> received a REQ packet from %s\n", p_from.cName);
			printf(">> client name to be used to search for an ip address\n"
				   "   \"%s\"\n",p_from.pData);

			//check whether a client is registered
			for(i = 0;i < registered;++i)
			{
				//search
				if(strcmp(reg[i].cName,p_from.pData) == 0)
				{
					++found;
				}
			}
		}
		else
		{
			fprintf(stderr, ">> unexpected packet type\n");
			continue;
		}

		//if registered , send REPLY packet with the ip address of the other client
		//source ip infor
		memset(&to_addr,'\0',sizeof(struct sockaddr_in));
		to_addr.sin_addr.s_addr = (in_addr_t)p_from.sIP;
		to_addr.sin_port = htons(p_from.sP);
		to_addr.sin_family = AF_INET;

		//ip addresses
		src.s_addr = to_addr.sin_addr.s_addr;
		dest.s_addr = p_from.sIP;

		memset(p_to.pData,'\0',sizeof(p_to.pData));
		if (found > 0)
		{
			//send a REPLY packet
			struct in_addr ip;
			ip.s_addr = reg[i].cIP;
			p_to.pType = REPLY;
			strcpy(p_to.pData, inet_ntoa(ip));

			printf(">> sending a REPLY packet...\n");
		}
		else
		{
			//send ERROR packet
			p_to.pType = ERROR;
			strcpy(p_to.pData, "client not registered");

			printf(">> sending an ERROR packet...\n");
		}

		//send
		_send(p_to,(struct sockaddr*)&to_addr,sock);
		//packet infor
		printf("   sourceIp : %s\n   sourcePort : %d\n"
			   "   destIp : %s\n   destPort : %d\n"
			   "   type : REPLY\n"
			   "   clientName : \"%s\"\n"
			   "   data : \"%s\"\n",inet_ntoa(src),p_to.sP,
				   inet_ntoa(dest),p_to.dP,p_to.cName,p_to.pData);

	}

	//deallocate memory
	free(dB);
	free(reg);
	return 0;
}


////////////////////////////////////////////////////////////
///////////////provide function implementation//////////////

//initialize password database
Database* init_database(int* s)
{
	//open file
	FILE *pwdFile = fopen("password.txt","r");
	Database* db;
	char buff[64];
	int size;

	if(pwdFile == NULL){
		return NULL;
	}

	//get db size ==> first line of the file
	fgets(buff,sizeof(buff),pwdFile);
	size = atoi(buff);

	*s = size;

	//allocate memory
	db = (Database*) malloc(sizeof(Database)*size);

	//opened
	int i = 0;
	while(!feof(pwdFile))
	{
		//get a line
		memset(buff,'\0',sizeof(buff));
		fgets(buff,sizeof(buff),pwdFile);

		if(strlen(buff) < 5) break;
		//get name
		int from = 0, len = strcspn(buff + from,":");
		strncpy(db[i].cName,buff + from, len);
		//update
		from += len + 1;
		len = strcspn(buff + from,"\n");
		strncpy(db[i].cPwd,buff + from, len);

		++i;
	}

	fclose(pwdFile);
	//return
	return db;
}

//try to authenticate a client
int auth(Database* db, int size, char* n, char* p)
{
	//check whether a combination of the name and password provided exits in the
	//db
	for(int i = 0;i < size;++i)
	{
		if(strcmp(db[i].cName,n) + strcmp(db[i].cPwd,p) == 0)
		{
			return 0;//true
		}
	}
	return -1;//false
}

//is this client registered?
int is_registered(Register* rgstr,int c,const char* n, short p, unsigned int ip)
{
	for(int i = 0;i < c;++i)
	{
		if(strcmp(rgstr[i].cName,n) == 0 && rgstr[i].cPort == p
			&& rgstr[i].cIP == ip)
		{
			return 0;//true
		}
	}
	return -1;//false
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
	char tmp[sizeof(long)];
	int len, from = 0;

	//receive
	recvfrom(s,buffer,sizeof(buffer),0,&peerAddr,&size);

	//break down buffer into packet
	//fields delimited by ":"
	memset(&p,'\0',sizeof(Packet));
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

