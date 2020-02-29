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

// default port for chat server
// TODO: export to configuration file
#define PORT "9900"
#define REQUEST_QUEUE_LENGTH 1000

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

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("Server Setup::: sigaction -->> error reaping all dead processes");
        perror("Server Will Shutdown");
        exit(1);
    }

    printf("Server Setup Successful:: Waiting for Connections...\n");

    // main accept() loop :: infinite
    while(1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("Server Inbound Connection::: Accept Connection Error");
            perror("Server Inbound Connection::: Will Accept Next Connection From Queue");
            continue;
        }

        // TODO: don't know what this does
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        printf("Server Inbound Connection::: Got Connection from %s\n", s);

        // fork() in linux creates child process, ie. new thread
        // fork() returns the pid of process to parent thread/process, ie. positive number
        // after calling fork() the child process/thred also starts from same line
        // and for child process/thread fork() will return 0 (or false)
        // on error fork() will return -1 to parent process/thread
        if (!fork()) // go inside if block only if current thread is child thread/process
        {
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello World!!!", 13, 0) == -1)
            {
                perror("Server Outbound::: Error Sending Message to Connection");
            }
            close(new_fd); // after sending one message close new socket in child thread
            exit(0); // exit child thread
        }

        close(new_fd); // parent doesn't need new socket now
    }

    return 0;
}
