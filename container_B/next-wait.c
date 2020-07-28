#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <errno.h>
#include "relay.h"

void log_msg(char* msg) {
    printf("next-wait:\t%s\n", msg);
    fflush(stdout);
}
void log_warning(char* msg) {
    printf("next-wait [WARNING]:\t%s\n", msg);
    fflush(stdout);
}
void log_error(char* msg, int signal) {
    fprintf(stderr, "next-wait [ERROR]:\t%s\n", msg);
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
            debug = 0;
    uint16_t s_port;
    struct sockaddr_in server_addr, client_addr; 
    struct hostent *he;
    struct timeval timeout;
    int sockfd, n, len = sizeof(server_addr);
    while((n = getopt(argc, argv, "vdh")) != -1) {  
        if (n=='v') {
            verbose = 1;    
        } else if (n=='d') {
            debug = 1;
        } else if (n=='h') {
            printf("\t%s\n", "");
            printf("\t%s\n", "./next-wait [-v] [-d] [-h] REQUEST_HOST REQUEST_PORT");
            printf("\t%s\n", "===========================================================");
            printf("\t%s\n", "");
            printf("\t%s\n", "OPTIONS:");
            printf("\t%s\n", "-v (Verbose)\t- to print actions");
            printf("\t%s\n", "-d (Debug)\t- to print warnings");
            printf("\t%s\n", "-h (Help)\t- to print help page");
            printf("\t%s\n", "");
            printf("\t%s\n", "ARGUMENTS:");
            printf("\t%s\n", "REQUEST_HOST\t- service name or network host for the container for which this one is waiting");
            printf("\t%s\n", "REQUEST_PORT\t- port for the container for which this one is waiting");
            printf("\t%s\n", "");
            exit(0);
        } else {
            exit(1);
        }
    }
    if (argc-optind!=2) {
        log_error("usage: ./next-wait [-v] [-d] [-h] REQUEST_HOST REQUEST_PORT", 1);
    }
    if ((n = atoi(argv[optind+1])) == 0) {
        log_error("Second argument is not readable port number.", 1);
    } else if (n<0 || n>25535) {
        log_error("Second argument is invalid sending port.", 1);
    } else {
        s_port = (uint16_t)n;
    }
  
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0) { 
        log_error("Cannot create sending socket", EXIT_FAILURE);
    }
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout))<0) {
        log_error("Cannot set socket options", EXIT_FAILURE);
    }

    if ((he = gethostbyname(argv[optind]))==NULL) {
        log_error("Cannot resolve hostname", EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr)); 
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(s_port);

    relayItemType buffer, msg;
    if (verbose||debug) log_msg("Waiting for ready reply");
    while (1) {
        memset(&client_addr, 0, sizeof(client_addr)); 
        msg = FIND_MSG;
        sendto(sockfd, &msg, sizeof(msg), MSG_CONFIRM, (const struct sockaddr*)&server_addr, sizeof(server_addr)); 
        n = recvfrom(sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr*)&client_addr, &len);
        if (n==-1&&(errno==EAGAIN||errno==EWOULDBLOCK)) {
            if (debug) log_warning("Timed-out waiting for ready");
        } else if (n < 0) {
            find_net_error();
        } else if (n == 0) {
            if (debug) log_warning("No data read");
        } else if (buffer==READY_MSG&&memcmp(&client_addr.sin_addr, he->h_addr_list[0], he->h_length)==0&&ntohs(client_addr.sin_port)==s_port) {
            if (verbose||debug) log_msg("Wait is done.");
            break;
        } else if (debug) {
            log_warning("Data unknown");
        }
    }
  
    close(sockfd);
    return 0; 
}