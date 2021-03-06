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
    int *sockfd;
    int c_ter;
    Registry *reg;
    Registry *reg2;
    Registry *reg3;
    pthread_mutex_t *accept_reg_lock;
    pthread_mutex_t *confirm_reg_lock;
    pthread_mutex_t *chat_reg_lock;
};


void *registerConnection(void *);
int confirmConSuccessfull(int);
void *confirmConnectionLoop();
void *acceptConnectionLoop(void *);
void *get_in_addr(struct sockaddr *);
void sigchld_handler(int);


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


int main()
{
    printf("Telnet Chat Server Started");
    nextLine();

    pthread_mutex_t accept_reg_lock;
    pthread_mutex_t confirm_reg_lock;
    pthread_mutex_t chat_reg_lock;

    Registry* accept_reg = createRegister(&accept_reg_lock);
    Registry* confirm_reg = createRegister(&confirm_reg_lock);
    Registry* chat_reg = createRegister(&chat_reg_lock);

    int accept_ter_flag = 0;
    int accept_exit_flag = 0;

    // listen on sockfd, new connection on new_fd
    int sockfd, new_fd;

    // addrinfo struct contains machine address data enough to create socket
    struct addrinfo hints, *servinfo, *p;

    // connector's address information
    struct sockaddr_storage their_addr;

    socklen_t sin_size;

    struct sigaction sa;

    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    // void *memset(void *s, int c, size_t n);
    // The  memset()  function  fills  the  first  n bytes of the memory area
    // pointed to by s with the constant byte c
    memset(&hints, 0, sizeof hints);

    // TODO: ecport to configuration files
    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_PASSIVE;

    int addr_verify_status = getaddrinfo(NULL, PORT, &hints, &servinfo);

    // check if there is no error for getaddrinfo() operation

    if (addr_verify_status != 0)
    {
        printf("Unable to resolve Server Address");
        printf("Unable to setup Chat Server");

        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_verify_status));

        exit(1);
    }

    // loop through all the results and bind to the first we can

    int create_socket_try_count = 1;

    for (p = servinfo; p != NULL; p = p->ai_next, create_socket_try_count++)
    {
        // setup socket and store in sockfd, check for errors
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("Server Setup::: Create Socket Error");
            continue;
        }

        printf("Server Socket_FD Aquired : %d", sockfd);
        nextLine();

        // if port already in use, set the property to change config and reuse the same port
        // TODO: I think should disable this feature/process for production(or anywhere really)
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("Server Setup::: Reuse Already Occupied Port, Config Setup Error");
            perror("Server Will Shutdown");
            exit(1);
        }

        // Bind the socket to specified port and check for errors
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            // if binding fails, close socket file discriptor
            close(sockfd);
            perror("Server Setup::: Failed to Bind Socket to Port");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    // if p is null, server is not setup, shutdown server
    if (p == NULL)
    {
        fprintf(stderr, "Server Setup::: Failed to Bind Socket to Port");
        perror("Server Will Shutdown");
        exit(1);
    }

    // listen on specified port, shutdown server on error
    if (listen(sockfd, REQUEST_QUEUE_LENGTH) == -1)
    {
        perror("Server Setup::: Error to Setup Listen on Port");
        exit(1);
    }

    printf("Server Listening to Port : %s", PORT);
    nextLine();

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("Server Setup::: sigaction -->> error reaping all dead processes");
        perror("Server Will Shutdown");
        exit(1);
    }   

    // ---------- ### Setup Done :: Waiting for Connection ### -----------------------------------------------//



    printf("Server Setup Successful:: Waiting for Connections...\n");
    nextLine();

    pthread_t acc_t, reg_t;
    pthread_attr_t attr1, attr2;
    int rc;

    struct package p1;
    p1.sockfd = &sockfd;
    p1.c_ter = 100;
    p1.reg = accept_reg;
    p1.accept_reg_lock = &accept_reg_lock;

    struct package p2;
    p2.reg = accept_reg;
    p2.reg2 = confirm_reg;
    p2.confirm_reg_lock = &confirm_reg_lock;


    rc = pthread_attr_init(&attr1);
    rc = pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&acc_t, &attr1, acceptConnectionLoop, (void *)(&p1));

    printf("Server Thread :: AcceptConnectionLoop Started ... :: Response Code : %d", rc);  
    nextLine();

    rc = pthread_attr_init(&attr2);
    rc = pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&reg_t, &attr2, confirmConnectionLoop, (void *)(&p2));

    printf("Server Thread :: ConfirmConnectionLoop Started ... :: Response Code : %d", rc);
    nextLine();

    while(1)
    {
        sleep(10);
        printf("Accept Registry");
        nextLine();
        printReg(accept_reg);
        printf("Confirm Registry");
        nextLine();
        printReg(confirm_reg);
    }


    close(sockfd);

    return 0;
}

/**
 *  Accept Connection Loop, will accept all connection and store the clientfd in Registry "acceptreg" collection.
 *  Will terminate when error count reaches second argument or acceptTer global variable is set 1.
 *
 *  Arg -   int server_sock_fd :  server socket file discriptor int
 *      -   int ter_count : after the error count reaches this number acceptConnectionLoop() function will terminate
 *
 *  return -   void :   if accept_exit_flag is -1 then terminated due to reaching max error count,
 *                      if accept_exit_flag is 1 then terminated due to accept_ter_flag,
 *                      if accept_exit_flag is 2 then terminated due to unknown reason.
 */
void *acceptConnectionLoop(void *d)
{
    printf("Thread :: AcceptConnectionLoop :: Started Accepting Connection");
    nextLine();

    struct package *iii = (struct package *)d;

    int *server_sock_fd = iii->sockfd;

    int ter_count = iii->c_ter;

    Registry *accept_reg = iii->reg;

    pthread_mutex_t *accept_reg_lock = iii->accept_reg_lock;

    // TODO: take from input
    int accept_ter_flag = 100;

    int error_count = 0;
    struct sockaddr_storage client_addr;
    socklen_t addr_len;
    int client_fd = -1;
    char client_ip[INET6_ADDRSTRLEN];

    while((error_count < ter_count) || (accept_ter_flag == 0))
    {
        addr_len = sizeof client_addr;

        printf("Thread :: AcceptConnectionLoop :: Blocking Call to accept()");
        nextLine();

        client_fd = accept(*server_sock_fd, (struct sockaddr *) &client_addr, &addr_len);

        printf("Thread :: AcceptConnectionLoop :: Accepted Client FD : %d", client_fd);
        nextLine();

        if(client_fd == -1)
        {
            perror("Server Accept Connection ::: Accept Connection Error");
            nextLine();
            perror("Server Accept Connection ::: Will Accept Next Connection From Queue");
            nextLine();
            error_count++;
            continue;
        }

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *) &client_addr), client_ip, sizeof client_ip);

        printf("Thread :: AcceptConnectionLoop ::: Connection Accepted From %s", client_ip);
        nextLine();

        Json client_json= createJson();

        char *oot = calloc(100, sizeof(char));
        char *oo = calloc(100, sizeof(char));
        char *t = "client_sockfd:";
        char *client_fd_s = calloc(100, sizeof(char));
        sprintf(client_fd_s, "%d", client_fd);
        strcat(oot, t);
        strcat(oot, client_fd_s);

        printf("Thread :: AcceptConnectionLoop ::: c_sockfd_string : %s", oot);
        nextLine();

        String client_sockfd_string = getStringFrom(oot);

        char *tt = "client_ip:";
        strcat(oo, tt);
        strcat(oo, client_ip);

        printf("Thread :: AcceptConnectionLoop ::: c_ip_string : %s", oo);
        nextLine();

        String client_ip_string = getStringFrom(oo);

        addJson(&client_json, client_sockfd_string);
        addJson(&client_json, client_ip_string);

        printf("Thread :: AcceptConnectionLoop :: Add new connection to accept_reg");
        nextLine();

        addReg(accept_reg, &client_json, accept_reg_lock);
    }


    printf("Thread :: AcceptConnectionLoop :: Exit");
    nextLine();

    pthread_exit(0);
}

/**
 *  Loop accept_reg registry and for each client_fd register username and user_id for client,
 *  and store it in confirm_reg registry.
 */
void *confirmConnectionLoop(void *pkg)
{
    printf("Thread :: ConfirmConnectionLoop :: Start");
    nextLine();

    Registry *accept_reg = ((struct package*) pkg)->reg;
    Registry *confirm_reg = ((struct package*) pkg)->reg2;

    pthread_mutex_t *confirm_reg_lock = ((struct package*) pkg)->confirm_reg_lock;

    // accept_reg count (next position to confirm)
    int c_reg = 0;
    // loop termination flag
    int f_loop = 1;
    // flag to sleep because no new records/clients in accept_reg registry
    int f_sleep;

    // infinite loop
    while(f_loop)
    {
        if(c_reg < accept_reg->size)
        {
            printf("Thread :: ConfirmConnectionLoop :: Client Found");
            nextLine();

            Json j_client = accept_reg->reg[c_reg];

            printf("Thread :: ConfirmConnectionLoop :: Json Found");
            nextLine();

            String client_sockfd_string = getStringFrom("client_sockfd");
            
            printf("Find Key : ");
            printS(&client_sockfd_string);
            nextLine();

            String client_fd_s = getVal(j_client, client_sockfd_string);

            printf("Thread :: ConfirmConnectionLoop :: Register Client :: ");
            printS(&client_fd_s);
            nextLine();

            printf("Thread :: ConfirmConnectionLoop :: Confirm Connection For Client Socket : %s", client_fd_s.string);
            nextLine();

            const char *dd = client_fd_s.string;

            int client_fd = atoi(dd);
            
            // TODO: accept return value and use it
            confirmConSuccessfull(client_fd);
    
            pthread_t reg_con_thread;
            pthread_attr_t attr;
            int rc;

            ((struct package*) pkg)->sockfd = &client_fd;

            rc = pthread_attr_init(&attr);
            rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            rc = pthread_create(&reg_con_thread, &attr, registerConnection, (void *) pkg);

            printf("Thread :: ConfirmConnectionLoop :: Starting Thread : Register Connection :: for socket : %s", client_fd_s.string);
            nextLine();

            //registerConnection(client_fd);

            String sss = getStringFrom("confirm_msg_send:true");

            addJson(&j_client, sss);        
            accept_reg->reg[c_reg++] = j_client;
        }
        else
        {
            printf("Thread :: ConfirmConnectionLoop :: nothing to confirm Sleep 10 sec");
            nextLine();

            sleep(10);
        }
    }

    printf("Thread :: ConfirmConnectionLoop :: EXIT");
    nextLine();

    pthread_exit(0);
}


/*
 * Send the first message to client, that connection is successful
 * and now the client can register with the server.
 */
int confirmConSuccessfull(int client_fd)
{

    char *msg = "connection:true,msg:please register to server with a username";
    int mlen = strLen(msg);

    printf("Send Message :: %s", msg);
    nextLine();

    int msendlen = 0;
    if((msendlen = send(client_fd, msg, mlen, 0)) == -1)
    {
        perror("Server Outbound ::: Connection_Success_Msg :: Error Sending Message to Client");
        nextLine();
    }

    if(msendlen == mlen)
    {
        printf("Server Outbound ::: Connection Confirmation Message Send Successfully to Socket_FD:: %d", client_fd);
        nextLine();

        return 1;
    }
    else
    {
        printf("Server Outbound ::: Connection Confirmation Message Send Unsuccessfully to Socket_FD:: %d", client_fd);
        nextLine();

        return 0;
    }
}

void *registerConnection(void *pkg)
{
    printf("Thread :: RegisterConnection :: Start");
    nextLine();

    int client_fd = *(((struct package*) pkg)->sockfd);
    Registry *confirm_reg = ((struct package*) pkg)->reg2;
    pthread_mutex_t *confirm_reg_lock = ((struct package *) pkg)->confirm_reg_lock;

    // flag for termination, when c_ter reaches this number terminate
    int f_ter = 10;
    // count for termination
    int c_ter = 0;
    // count for sleep
    int c_sleep = 2;
    // message will be received in this char array
    char *buf = malloc(100 * sizeof(char));
    // received message length
    int len = 100;

    // set socket non-blocking
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    while(c_ter < f_ter)
    {
        if(recv(client_fd, buf, len, 0) == -1)
        {
            perror("Server Inbound ::: Register Connection :: Register Message Not Received");
            nextLine();
            printf("Server Inbound ::: Register Connection :: Client_FD : %d :: Wait Count : %d ", client_fd, c_ter);
            nextLine();
            c_ter++;
            sleep(c_sleep);
            continue;
        }

        printf("Server Inbound ::: Register Connection :: Redister Username : %s", buf);
        nextLine();

        Json new_reg = createJson();
        char *temp = malloc(100 * sizeof(char)); 
        char *jj = "client_sockfd:";
        char cc[10];

        sprintf(cc, "%d", client_fd);

        strcat(temp, jj);
        strcat(temp, cc);

        String cfd = getStringFrom(temp);
        addJson(&new_reg, cfd);
        String username = getStringFrom(buf);
        addJson(&new_reg, username);

        uuid_t binuuid;
        uuid_generate_random(binuuid);
        char *uuid = malloc(37);
        uuid_unparse(binuuid, uuid);
        char *uid = "user_id:";

        char *ttt = calloc(100, sizeof(char));

        strcat(ttt, uid);
        strcat(ttt, uuid);
        String userid = getStringFrom(ttt);

        addJson(&new_reg, userid);

        addReg(confirm_reg, &new_reg, confirm_reg_lock);

        char *msg = "registeration:true";
        int mlen = strLen(msg);

        int msendlen = 0;

        printf("Server Outbound ::: Register Connection :: Msg send : %s", msg);
        nextLine();

        if((msendlen = send(client_fd, msg, mlen, 0)) == -1)
        {
                perror("Server Outbound ::: Connection_Success_Msg :: Error Sending Message to Client");
                nextLine();
        }

        break;
    }

    printf("Thread :: ConfirmConnectionLoop :: EXIT");
    nextLine();

    pthread_exit(0);
}
