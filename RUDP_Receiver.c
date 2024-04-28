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
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    int curr_port = atoi(argv[1]);

    //creating socket
    struct sockaddr_in clientAddr;
    socklen_t addr_len = sizeof(struct sockaddr);
    Packet current_packet;
    int recv_socket = rudp_socket(clientAddr, curr_port, INADDR_ANY);
    char buffer[BUFFER_SIZE];

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(curr_port);
    if ((bind(recv_socket, (struct sockaddr*)&clientAddr, sizeof(clientAddr))) < 0 ) {
        perror("Error binding socket");
        exit(1);
    }
    printf("Server is listening on port %d...\n", curr_port);
    
    ssize_t total_bytes_received = 0;
    ssize_t bytes_received;

    // the time meassure
    clock_t start_time, end_time;
    double total_time;
    start_time = clock();   

    if(receiverHandshake(recv_socket, &clientAddr) != 0){
        printf("handshake error. aborting\n");
        exit(1);
    }
    else{
        printf("handshake successful\n");
    }


    printf("trying to receive\n");////////////////////////////////////////////////////

    /*Note to self: the problem is in this loop!!!*/
    while (1) {
    // the receiving process
    // we're receiving the buffers one by one and writing them down on the new file 
    while ((bytes_received = recvfrom(recv_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addr_len)) > 0) {
        total_bytes_received += bytes_received;

        if (bytes_received < 0) {
            perror("Error receiving data");
            exit(1);
        }
        
        // Check if the buffer contains "\exit"
        if (strstr(buffer, "\exit") != NULL) {
            // Process the received data containing "\exit"
            char* exit_position = strstr(buffer, "\exit");
            size_t bytes_to_write = exit_position - buffer;

            end_time = clock();
            total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
           // printf("File received and saved as %s (Time taken: %.8f seconds)\n", name, total_time);
            
            double average_bandwidth = (total_bytes_received * 8) / (total_time * 1024 * 1024); // in Mbps
            printf("Average Bandwidth: %.2f Mbps\n", average_bandwidth);
            
            start_time = clock();
            break; // Exit the inner loop
        }
    } 
}

    // Close sockets
    rudp_close(recv_socket);

    return 0;
}
