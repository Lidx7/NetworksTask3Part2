#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <time.h>
#include "RUDP_API.c"

#define BUFFER_SIZE 4096
#define FALSE 0
#define TRUE 1


int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    char buffer[BUFFER_SIZE];
    int seq_num = 0;

    //creating socket
    struct sockaddr_in receive_addr;
    int recv_socket = rudp_socket();

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    setsockopt(recv_socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));

    memset(&receive_addr, 0, sizeof(receive_addr));
    receive_addr.sin_family = AF_INET;
    receive_addr.sin_addr.s_addr = INADDR_ANY;
    receive_addr.sin_port = htons(port);
    if ((bind(recv_socket, (struct sockaddr*)&receive_addr, sizeof(receive_addr))) < 0 ) {
        perror("Error binding socket");
        exit(1);
    }

    printf("Server is listening on port %d...\n", port);

    if(receiverHandshake(recv_socket, &receive_addr) != 0){
        printf("handshake error. aborting\n");
        exit(1);
    }
    else{
        printf("handshake successful\n");
    }

    gettimeofday(&start_time, NULL); 

    while (1) {
        int bytes_received = rudp_recv(recv_socket, &receive_addr);
        if (bytes_received < 0) {
            perror("Error receiving data");
            exit(1);
        }
        else if (bytes_received == 3) {
            break;
        }
        printf("finished receiving file\n");
        
             
    }

    return 0;
    
}


