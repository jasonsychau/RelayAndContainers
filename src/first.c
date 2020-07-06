#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <errno.h>
#include "relay.h"


void log_msg(char* msg) {
    printf("first:\t%s\n", msg);
    fflush(stdout);
}
void log_warning(char* msg) {
    printf("first [WARNING]:\t%s\n", msg);
    fflush(stdout);
}
void log_error(char* msg, int signal) {
    fprintf(stderr, "first [ERROR]:\t%s\n", msg);
    exit(signal);
}
void find_net_error() {
    switch (errno) {
        case EWOULDBLOCK:
            log_error("Timeout error", 1);
            break;
        case EBADF:
            log_error("Invalid file descriptor for socket", 1);
            break;
        case ECONNREFUSED:
            log_error("Remote host refused network connection.", 1);
            break;
        case EFAULT:
            log_error("Buffer pointers are not own.", 1);
            break;
        case EINTR:
            log_error("Receive is interrupted by signal.", 1);
            break;
        case EINVAL:
            log_error("Invalid argument", 1);
            break;
        case ENOMEM:
            log_error("Could not allocate memory", 1);
            break;
        default:
            log_error("Other error", 1);
            break;
    }
}

int main(int argc, char* argv[]) {
    uint8_t verbose = 0,
            debug = 0,
            closing = 0;
    uint16_t l_port, s_port;
    struct sockaddr_in server_addr, client_addr; 
    int n, sockfd, len = sizeof(server_addr);
    relayItemType buffer, msg;
    while((n=getopt(argc, argv, "vdh")) != -1) {  
        if (n=='v') {
            verbose = 1;
        } else if (n=='d') {
            debug = 1;
        } else if (n=='h') {
            printf("\t%s\n", "");
            printf("\t%s\n", "./first [-v] [-d] [-h] MY_PORT [NEXT_PORT]");
            printf("\t%s\n", "===========================================================");
            printf("\t%s\n", "");
            printf("\t%s\n", "OPTIONS:");
            printf("\t%s\n", "-v (Verbose)\t- to print actions");
            printf("\t%s\n", "-d (Debug)\t- to print warnings");
            printf("\t%s\n", "-h (Help)\t- to print help page");
            printf("\t%s\n", "");
            printf("\t%s\n", "ARGUMENTS:");
            printf("\t%s\n", "MY_PORT\t\t- this container or program port");
            printf("\t%s\n", "NEXT_PORT\t- the container or program port from which this program is expecting a close request");
            printf("\t%s\n", "");
        } else {
            exit(1);
        }
        exit(0);
    }
    if (argc-optind!=1&&argc-optind!=2) {
        log_error("usage: ./first [-v] [-d] [-h] MY_PORT [NEXT_PORT]", 1);
    }
    if ((n=atoi(argv[optind]))==0) {
        log_error("First argument is not readable port number.", 1);
    } else if (n<0||n>25535) {
        log_error("First argument is invalid listening port.", 1);
    } else {
        l_port = (uint16_t)n;
    }
    if (argc-optind==2) {
        closing = 1;
        if ((n = atoi(argv[optind+1])) == 0) {
            log_error("Second argument is not readable port number.", 1);
        } else if (n<0||n>25535) {
            log_error("Second argument is invalid sending port.", 1);
        } else {
            s_port = (uint16_t)n;
        }
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0) {
        log_error("Cannot create listening socket", EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(l_port); 
    if (bind(sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr))<0) { 
        log_error("Cannot bind socket to network address", EXIT_FAILURE);
    }
    
    if (verbose||debug) log_msg("Preparing to pass ready signal.");
    while (1) {
        memset(&client_addr, 0, sizeof(client_addr)); 
        n = recvfrom(sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr*)&client_addr, &len);
        if (n<0) {
            find_net_error();
        } else if (n==0) {
            if (debug) log_warning("No data read");
        } else if (buffer==FIND_MSG) {
            msg = READY_MSG;
            sendto(sockfd, &msg, sizeof(msg), MSG_CONFIRM, (const struct sockaddr*)&client_addr, len);
            if (verbose||debug) log_msg("Sent ready signal");
        } else if (buffer==CLOSE_MSG&&closing==1&&ntohs(client_addr.sin_port)==s_port) {
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout))<0) {
                log_error("Cannot set socket options", EXIT_FAILURE);
            }

            memset(&server_addr, 0, sizeof(server_addr)); 
            server_addr.sin_family = AF_INET; 
            server_addr.sin_port = htons(s_port); 
            server_addr.sin_addr.s_addr = INADDR_ANY;

            uint8_t attempts = 0;
            if (verbose||debug) log_msg("Preparing pass");
            while (1) {
                memset(&client_addr, 0, sizeof(client_addr)); 
                msg = CLOSING_MSG;
                sendto(sockfd, &msg, sizeof(msg), MSG_CONFIRM, (const struct sockaddr*)&server_addr, sizeof(server_addr));
                n = recvfrom(sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr*)&client_addr, &len); 
                if (n==-1&&(errno==EAGAIN||errno==EWOULDBLOCK)) {
                    attempts += 1;
                    if (attempts==MAX_REQUESTS) {
                        log_error("Max closing messages reached", 1);
                    } else if (debug) {
                        log_warning("Timed-out waiting for close confirm");
                    }
                } else {
                    attempts = 0;
                    if (n < 0) {
                        find_net_error();
                    } else if (n == 0) {
                        if (debug) log_warning("No data read");
                    } else if (buffer==FIND_MSG) {
                        msg = READY_MSG;
                        sendto(sockfd, &msg, sizeof(msg), MSG_CONFIRM, (const struct sockaddr*)&client_addr, len);
                        if (verbose||debug) log_msg("Sent ready signal");
                    } else if (buffer==CONFIRM_MSG&&ntohs(client_addr.sin_port)==s_port) {
                        if (verbose||debug) log_msg("Closing");
                        break;
                    } else if (buffer==CONFIRM_MSG) {
                        if (debug) log_warning("Received unexpected close confirm");
                    } else if (debug) {
                        log_warning("Data unknown");
                    }
                }
            }
            break;
        } else if (buffer==CLOSE_MSG) {
            if (debug) log_warning("Received unexpected close request");
        } else if (debug) {
            if (debug) log_warning("Data unknown");
        }
    }

    close(sockfd);

    return 0; 
} 
