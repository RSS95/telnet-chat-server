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


// default port for chat server
// TODO: export to configuration file
#define PORT "9900"
#define REQUEST_QUEUE_LENGTH 1000


struct package
{
    int sockfd;
    Registry *chat_reg;
    pthread_mutex_t *chat_reg_lock;
};


void *registerConnection(void *);
int   confirmConSuccessfull(int);
void *confirmConnectionLoop();
void *acceptConnectionLoop(void *);
void *get_in_addr(struct sockaddr *);
void  sigchld_handler(int);
int   initAddrInfoHints(struct addrinfo *);
int   getAvailableServerAddrInfoList(struct addrinfo *, struct addrinfo *);
int   createAndBindSocket(struct addrinfo *);
void *vip_accept(void *);
void *vip_reg(void *);
void  manageChat(struct package *);
int   recvFromSocket(int, char *);
void  processMsgFromClient(Registry *, pthread_mutex_t *, char *);
void  createColonString(char *, char *, String *);
void  syncMsgToClient(int, Registry *, int *);
void  sendMsg(int, char *);


/* TODO: Know what this function does
 * Don't know the use of this function, author(Beej) says it is for
 * removing dead/zombie threads from fork()
 */
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


/*
 * get sockaddr, IPv4 or IPv6:
 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
 *  Initialize addrinfo hints struct.
 *  Curruntly hardcodded, need to get from config files.
 */
int initAddrInfoHints(struct addrinfo *hints)
{
    hints = malloc(sizeof(struct addrinfo));

    // memset initializes the first n positions (given by third arg) with 0 (second arg),
    // from memory location/pointer (given by first arg)
    memset(hints, 0, sizeof(struct addrinfo));

    // init hints, TODO: can be picked from configuration files
    hints->ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints->ai_socktype = SOCK_STREAM; /* Stream socket */
    hints->ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints->ai_protocol = 0;          /* Any protocol */
    hints->ai_canonname = NULL;
    hints->ai_addr = NULL;
    hints->ai_next = NULL;

    struct addrinfo *servinfolist;

    return getAvailableServerAddrInfoList(servinfolist, hints);

    // return createAndBindSocket(servinfolist);
}

/*
 *  Using getaddrinfo, this function will get a array of available addrinfo given by system
 *  and store it in servinfolist pointer.
 *
 *  On any error it will run exit() and the application will shutdown.
 */
int getAvailableServerAddrInfoList(struct addrinfo *servinfolist, struct addrinfo *hints)
{
    /*    
        int getaddrinfo(const char *node, const char *service,
                        const struct addrinfo *hints,
                        struct addrinfo **res);

        Given  node(ip address)  and  service(port), which identify an Internet host and a service,
        getaddrinfo() returns one or more addrinfo structures, 
        each of which contains an Internet address that can be specified in a call 
        to  bind(2) or  connect(2).

        If node is null then that means, the returned addrinfo's can be used
        for bind() by a server
    */

    int status = getaddrinfo(NULL, PORT, hints, &servinfolist);

    printf("server_setup: getaddrinfo status: %d", status);
    nextLine();

    if (status != 0)
    {
        printf("server_setup: unable to resolve server address");
        nextLine();
        printf("server_setup: unable to setup chat server");
        nextLine();
        printf("server_setup: server will shutdown");
        nextLine();

        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));

        exit(1);
    }

    return createAndBindSocket(servinfolist);
}


int createAndBindSocket(struct addrinfo *servinfolist)
{
    int count = 0;
    struct addrinfo *servinfo;
    // sockfd == -1, means error
    int server_sockfd = -1;
    int optval = 1;

    for(servinfo = servinfolist; servinfo != NULL; servinfo = (servinfolist)->ai_next, count++)
    {
        // setup socket and store in server_sockfd, check for errors     
        if ((server_sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
        {
            perror("server_setup: create socket error");
            nextLine();
            continue;
        }

        printf("server: socket fd: %d", server_sockfd);
        nextLine();
        
        // if port already in use, set the property to change config and reuse the same port
        //  TODO: I think should disable this feature/process for production(or anywhere really)
        if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
        {
            perror("server_setup: already occupied port");
            nextLine();
            perror("server will shutdown");
            nextLine();
            exit(1);
        }

        // Bind the socket to specified port and check for errors
        if (bind(server_sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
        {
            // if binding fails, close socket file discriptor
            close(server_sockfd);
            perror("server_setup: failed to bind socket to port");
            nextLine();
            continue;
        }

        break;
    }

    printf("server_setup: count of tries to bind socket: %d", count);
    nextLine();

    // if p is null, server is not setup, shutdown server
    if (servinfo == NULL)
    {
        fprintf(stderr, "server_setup: failed to find a bindable socket");
        nextLine();
        perror("server will shutdown");
        nextLine();
        exit(1);
    }

    return server_sockfd;
}


int main()
{
    printf("VIP Chat Server");
    nextLine();
    printf("Server Discription :");
    nextLine();
    printf("\tVIP Chat Server is an implementation in which,");
    printf(" each client will have a specific thread in server.");
    nextLine();
    printf("\tAll Client communication operations will be handled by these vip threads.");
    nextLine();

    // lock objects/structs for all registrys
    pthread_mutex_t chat_reg_lock;

    // registry struct pointer to store useful data
    Registry* chat_reg = createRegister(&chat_reg_lock);

    // listen on server_sockfd
    int server_sockfd;

    // addrinfo struct contains machine address data enough to create socket
    struct addrinfo *hints;
    
    // init hints to get servinfolist
    server_sockfd = initAddrInfoHints(hints);

    // from hints get servinfolist from system, exit if error
    // getAvailableServerAddrInfoList(servinfolist, hints);

    // loop through all the results and bind to the first we can
    // server_sockfd = createAndBindSocket(servinfolist);

    // all done with this structure
    // freeaddrinfo(*servinfolist);

    // listen on specified port, shutdown server on error
    if (listen(server_sockfd, REQUEST_QUEUE_LENGTH) == -1)
    {
        perror("Server Setup::: Error to Setup Listen on Port");
        exit(1);
    }

    printf("Server Listening to Port : %s", PORT);
    nextLine();

    // reap all dead processes ( zombie process )
    /*
    sa.sa_handler = sigchld_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("Server Setup::: sigaction -->> error reaping all dead processes");
        perror("Server Will Shutdown");
        exit(1);
    } 
    */

    // ---------- ### Setup Done :: Waiting for Connection ### --------- //


    printf("Server Setup Successful:: Waiting for Connections...\n");
    nextLine();

    pthread_t accept_thread;
    pthread_attr_t accept_attr;
    int rc;

    struct package p;
    p.sockfd = server_sockfd;
    p.chat_reg = chat_reg;
    p.chat_reg_lock = &chat_reg_lock;

    rc = pthread_attr_init(&accept_attr);
    // rc = pthread_attr_setdetachstate(&accept_attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&accept_thread, &accept_attr, vip_accept, (void *)(&p));

    printf("Server Thread :: AcceptConnectionLoop Started ... :: Response Code : %d", rc);  
    nextLine();

    pthread_join(accept_thread, NULL);

    close(server_sockfd);

    return 0;
}


void *vip_accept(void *pkg)
{
    int server_sockfd = ((struct package *) pkg)->sockfd;
    struct sockaddr_storage client_addr;
    socklen_t addr_len;
    int client_sockfd = -1;
    char client_ip[INET6_ADDRSTRLEN];

    // set server socket as non-blocking
    fcntl(server_sockfd, F_SETFL, O_NONBLOCK);

    while(1)
    {
        addr_len = sizeof client_addr;

        client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &addr_len);

        if(client_sockfd == -1)
        {
            //perror("Server Accept Connection ::: Accept Connection Error");
            //perror("Server Accept Connection ::: Will Accept Next Connection From Queue");
            sleep(3);
            continue;
        }

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *) &client_addr), client_ip, sizeof client_ip);
        
        pthread_t reg_t;
        pthread_attr_t attr1;
        int rc;

        struct package *p = malloc(sizeof(struct package));
        p->sockfd = client_sockfd;
        p->chat_reg = ((struct package *) pkg)->chat_reg;
        p->chat_reg_lock = ((struct package *) pkg)->chat_reg_lock;

        rc = pthread_attr_init(&attr1);
        rc = pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
        rc = pthread_create(&reg_t, &attr1, vip_reg, (void *)(p));

        printf("server: connection accepted from: %s", client_ip);
        nextLine();

    }

    pthread_exit((void *) 1);
}


void *vip_reg(void *pkg)
{
    struct package p = *((struct package *) pkg);
    free(pkg);

    int client_sockfd = p.sockfd;

    // int res = confirmConSuccessfull(client_sockfd);

    int res = 1;

    if(res == 0)
    {
        pthread_exit(0);
    }
    else
    {
        manageChat(&p);
        pthread_exit((void *) 1);
    }
}


void manageChat(struct package *pkg)
{
    int msgcounter = 0;

    int client_sockfd = pkg->sockfd;

    Registry *chat_reg = pkg->chat_reg;

    pthread_mutex_t *chat_reg_lock = pkg->chat_reg_lock;

    // make client sockfd non-blocking
    fcntl(client_sockfd, F_SETFL, O_NONBLOCK);

    while(1)
    {

        sleep(4);

        if (chat_reg->size > msgcounter)
        {
            syncMsgToClient(client_sockfd, chat_reg, &msgcounter);
        }

        // string always calloc
        char *buf = calloc(20, sizeof(char));

        int flag = recvFromSocket(client_sockfd, buf);

        if(flag > 0)
        {
            printf("server: connection closed from clientside"); 
            close(client_sockfd);
            free(buf);
            return;
        }
        else if(flag == 0)
        {
            printf("msg received: %s", buf);
            nextLine();
            
            processMsgFromClient(chat_reg, chat_reg_lock, buf);
            msgcounter++;
        }
        else 
        {
            // no message or error
        }

        free(buf);
    }
}


int recvFromSocket(int client_sockfd, char *buf)
{
    int flag = 0;

    int len = 20; 

    int ret = recv(client_sockfd, buf, len, 0);

    if(ret < 0)
    {
        flag = -1;
        // perror("socket:recv:message not received");
        // nextLine();
    }
    else if(ret == 0)
    {
        perror("socket:recv:client shutdown");
        flag = 1;
    }

    return flag;
}


void processMsgFromClient(Registry *chat_reg, pthread_mutex_t *chat_reg_lock, char *msg)
{
    Json chat = createJson();
    String res = getStringFrom(msg);

    //createColonString("msg", msg, &res);

    addJson(&chat, res);

    String key = getStringFrom("msg");

    if(jsonContainsKey(&chat, key))
    {
        addReg(chat_reg, &chat, chat_reg_lock);
    }

}


void createColonString(char *key, char *value, String *res)
{
    int keylen = strLen(key);
    int valuelen = strLen(value);

    // TODO : find why am i using calloc
    char *container = calloc((keylen + valuelen + 1), sizeof(char));

    strcat(container, key);
    strcat(container, ":");
    strcat(container, value);

    *res = getStringFrom(container);
}


void syncMsgToClient(int client_sockfd, Registry *chat_reg, int *msgcounter)
{
    //printf("reg size: %d : msgcounter: %d", chat_reg->size, *msgcounter);

    while((chat_reg->size) > *msgcounter)
    {
        printf("reg size: %d : msgcounter: %d", chat_reg->size, *msgcounter);
        nextLine();

        Json chat = chat_reg->reg[(*msgcounter)];

        (*msgcounter) = (*msgcounter) + 1;

        printf("k: %s : v: %s", chat.entry->key.string, chat.entry->value.string);
        nextLine();

        String keyString = getStringFrom("msg");
        String valueString = getVal(&chat, keyString);

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
        // TODO : do something, send remaining msg
        printf("socket:send:incomplete msg send");
        nextLine();
    }
}



/**
 *  Loop accept_reg registry and for each client_fd register username and user_id for client,
 *  and store it in confirm_reg registry.
 */
void *confirmConnectionLoop(void *pkg)
{
//    printf("Thread :: ConfirmConnectionLoop :: Start");
//    nextLine();
//
//    Registry *accept_reg = ((struct package*) pkg)->reg;
//    Registry *confirm_reg = ((struct package*) pkg)->reg2;
//
//    pthread_mutex_t *confirm_reg_lock = ((struct package*) pkg)->confirm_reg_lock;
//
//    // accept_reg count (next position to confirm)
//    int c_reg = 0;
//    // loop termination flag
//    int f_loop = 1;
//    // flag to sleep because no new records/clients in accept_reg registry
//    int f_sleep;
//
//    // infinite loop
//    while(f_loop)
//    {
//        if(c_reg < accept_reg->size)
//        {
//            printf("Thread :: ConfirmConnectionLoop :: Client Found");
//            nextLine();
//
//            Json j_client = accept_reg->reg[c_reg];
//
//            printf("Thread :: ConfirmConnectionLoop :: Json Found");
//            nextLine();
//
//            String client_sockfd_string = getStringFrom("client_sockfd");
//            
//            printf("Find Key : ");
//            printS(&client_sockfd_string);
//            nextLine();
//
//            String client_fd_s = getVal(j_client, client_sockfd_string);
//
//            printf("Thread :: ConfirmConnectionLoop :: Register Client :: ");
//            printS(&client_fd_s);
//            nextLine();
//
//            printf("Thread :: ConfirmConnectionLoop :: Confirm Connection For Client Socket : %s", client_fd_s.string);
//            nextLine();
//
//            const char *dd = client_fd_s.string;
//
//            int client_fd = atoi(dd);
//            
//            // TODO: accept return value and use it
//            confirmConSuccessfull(client_fd);
//    
//            pthread_t reg_con_thread;
//            pthread_attr_t attr;
//            int rc;
//
//            ((struct package*) pkg)->sockfd = &client_fd;
//
//            rc = pthread_attr_init(&attr);
//            rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//            rc = pthread_create(&reg_con_thread, &attr, registerConnection, (void *) pkg);
//
//            printf("Thread :: ConfirmConnectionLoop :: Starting Thread : Register Connection :: for socket : %s", client_fd_s.string);
//            nextLine();
//
//            //registerConnection(client_fd);
//
//            String sss = getStringFrom("confirm_msg_send:true");
//
//            addJson(&j_client, sss);        
//            accept_reg->reg[c_reg++] = j_client;
//        }
//        else
//        {
//            printf("Thread :: ConfirmConnectionLoop :: nothing to confirm Sleep 10 sec");
//            nextLine();
//
//            sleep(10);
//        }
//    }
//
//    printf("Thread :: ConfirmConnectionLoop :: EXIT");
//    nextLine();
//
//    pthread_exit(0);
}


