#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <uuid/uuid.h>
#include "./../include/string_array.h"

#define PORT "9900"
#define SERVER_IP "192.168.1.7"
#define MAXDATASIZE 100 // max number of bytes we can get at once


short registerToServer(int);
void *get_in_addr(struct sockaddr *);


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main()
{
    printf("Telnet Client Started");
    nextLine();

	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(SERVER_IP, PORT, &hints, &servinfo)) != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	while(1){

		// loop through all the results and connect to the first we can
		for(p = servinfo; p != NULL; p = p->ai_next) 
		{
			if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
			{
				perror("client: socket");
				continue;
			}
	
			if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
			{
				close(sockfd);
				perror("client: connect");
				continue;
			}
	
			break;
		}

	
		if (p == NULL) 
		{
			fprintf(stderr, "client: failed to connect\n");
			return 2;
		}

		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
		printf("Client: connecting to %s\n", s);
	
		//freeaddrinfo(servinfo); // all done with this structure
	
	
		
		//if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) 
		//{
		//	perror("recv");
		//	exit(1);
		//}

		//buf[numbytes] = '\0';

		//printf("client: received '%s'\n",buf);
	
        registerToServer(sockfd);

		close(sockfd);

        break;
	}

    printf("Client Shutdown");

	return 0;
}

/*
 * This function will take some inputs and register to server.
 * return 1 : True  -> for registeration successful
 * return 0 : False -> for registeration unsuccessful
 */
short registerToServer(int sockfd)
{
    char *buf = malloc(100 * sizeof(char));

    printf("Waiting to Receive Confirmation to register");
    nextLine();

    if(recv(sockfd, buf, 100, 0) == -1)
    {
        perror("Client Receive ::");
        //return 0;
        exit(0);
    }

    printf("Received :: ");
    printC(buf);
    nextLine();

    char *rss = "RSS_";

    uuid_t binuuid;
    uuid_generate_random(binuuid);
    char *uuid = malloc(37);
    uuid_unparse(binuuid, uuid);
    char *uid = "user_name:";

    char *temp = malloc(100 * sizeof(char));

    strcat(temp, rss);
    strcat(temp, uuid);
    
    char *temp2 = malloc(100 * sizeof(char));

    strcat(temp2, uid);
    strcat(temp2, temp);

    String userid = getStringFrom(temp2);
    printC(userid.string);
    nextLine();

    char *dd = userid.string;
    int leen = strLen(dd);

    if (send(sockfd, dd, leen, 0) == -1)
    {
        perror("Server Outbound::: Error Sending Message to Connection");
    }

    printf("register message send");
    nextLine();

    buf = malloc(100 * sizeof(char));

    printf("waiting for response");
    nextLine();

    if(recv(sockfd, buf, 100, 0) == -1)
    {
        perror("Client Receive ::");
        //return 0;
        exit(0);
    }

    printf("Received :: ");
    printC(buf);
    nextLine();

    return 1;
}
