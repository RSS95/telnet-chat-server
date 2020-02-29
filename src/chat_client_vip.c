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
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <uuid/uuid.h>
#include "./../include/string_array.h"
#include "./../include/registry.h"
#include "./../include/json.h"

#define PORT "9900"
#define SERVER_IP "192.168.1.7"
#define MAXDATASIZE 100 // max number of bytes we can get at once


short  registerToServer(int);
void  *get_in_addr(struct sockaddr *);
char  *randstr(char *, size_t);
int    randint();
void  *testchat(void *);
int    recvFromSocket(int, char *);
void   processMsgFromClient(Registry *, pthread_mutex_t *, char *);
void   createColonString(char *, char *, String *);
void   syncMsgToClient(int, Registry *, int *);
void   sendMsg(int, char *);


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

	int numbytes;
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
	
	//while(1){

        int *sockfd = malloc(sizeof(int));

		// loop through all the results and connect to the first we can
		for(p = servinfo; p != NULL; p = p->ai_next) 
		{
			if ((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
			{
				perror("client: socket");
				continue;
			}
	
			if (connect(*sockfd, p->ai_addr, p->ai_addrlen) == -1) 
			{
				close(*sockfd);
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
	
        // registerToServer(sockfd);

        pthread_t testchat_t;
        pthread_attr_t testchat_attr;
        int rc;

        rc = pthread_attr_init(&testchat_attr);
        // rc = pthread_attr_setdetachstate(&testchat_attr, PTHREAD_CREATE_DETACHED);
        rc = pthread_create(&testchat_t, &testchat_attr, testchat, (void *)(sockfd));
	//}
    
    pthread_join(testchat_t, NULL);

    printf("Client Shutdown");

	return 0;
}


char *randstr(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";

    if (size) 
    {
        --size;

        for (size_t n = 0; n < size; n++) 
        {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }

        str[size] = '\0';
    }

    return str;
}


int randint() 
{ 
    int i;
    int upper = 10;
    int lower = 0;

    return (rand() % (upper - lower + 1)) + lower;
} 


void *testchat(void *sockfd)
{
    int server_sockfd = *((int *)sockfd);
    free(sockfd);

    // set server socket as non-blocking
    fcntl(server_sockfd, F_SETFL, O_NONBLOCK);

    while(1)
    {
        sleep(randint());

        char *sendmsg = calloc(10, sizeof(char));
        size_t n = 10;
        
        randstr(sendmsg, n);

        String msg;
        createColonString("msg", sendmsg, &msg);
        
        sendMsg(server_sockfd, msg.string);

        printf("msg send: %s", msg.string);
        nextLine();

        char *recvmsg = calloc(20, sizeof(char));

        recvFromSocket(server_sockfd, recvmsg);

        printf("msg received: %s", recvmsg);
        nextLine();
    }

    close(server_sockfd);
    pthread_exit((void *) 1);
}


int recvFromSocket(int client_sockfd, char *buf)
{
    int flag = 0;

    int len = 20;

    int ret = recv(client_sockfd, buf, len, 0);

    if(ret < 0)
    {
        flag = -1;
    }
    else if(ret == 0)
    {
        perror("socket:recv:client shutdown");
        flag = 1;
    }

    return flag;
}


void createColonString(char *key, char *value, String *res)
{
    int keylen = strLen(key);
    int valuelen = strLen(value);
        
    char *container = calloc((keylen + valuelen + 1), sizeof(char));

    strcat(container, key);
    strcat(container, ":");
    strcat(container, value);

    *res = getStringFrom(container);
}


void syncMsgToClient(int client_sockfd, Registry *chat_reg, int *msgcounter)
{
    while((chat_reg->size + 1) > *msgcounter)
    {
        Json chat = chat_reg->reg[(*msgcounter)++];

        String keyString = getStringFrom("msg");
        String valueString = getVal(chat, keyString);

        String res;
        createColonString(keyString.string, valueString.string, &res);
        char *sendmsg = res.string;

        sendMsg(client_sockfd, sendmsg);
    }
}


void sendMsg(int sockfd, char *msg)
{
    int len = strLen(msg);

    int msendlen = 0;
    if((msendlen = send(sockfd, msg, len, 0)) == -1)
    {
        perror("socket:send:error sending message to given socket");
        nextLine();
    }

    if(msendlen != len)
    {
        printf("socket:send:incomplete msg send");
        nextLine();
    }
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
