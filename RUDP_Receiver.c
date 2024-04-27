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

    int server_socket;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];

    // Create server socket
    server_socket = rudp_socket(server_addr, curr_port, INADDR_ANY);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(curr_port);
    if ((bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0 ) {
        perror("Error binding socket");
        exit(1);
    }
    printf("Server is listening on port %d...\n", curr_port);
    
    // Receive file data
    int repeat_counter = 0;
    char name[50];
    sprintf(name, "received_file%d.txt", repeat_counter);
    FILE *file = fopen(name, "wb");
    if (file == NULL) {
        perror("Error creating file");
        exit(1);
    }
    
    ssize_t total_bytes_received = 0;
    ssize_t bytes_received;

    // the time meassure
    clock_t start_time, end_time;
    double total_time;
    start_time = clock();   

    printf("waiting for a SYN from sender...\n");
    while(1){
        // while ((bytes_received = recvfrom(server_socket, buffer, sizeof("SYN"), 0, (struct sockaddr *)&server_addr, &addr_len)) > 0){
        //     if(strncmp(buffer, "SYN", 3) == 0){
        //         printf("SYN received, sending ACK...\n");
        //         sendto(server_socket, "ACK", sizeof("ACK"), 0, (struct sockaddr *)&server_addr, addr_len);
        //         break;
        //     }
        // }   

        // while ((bytes_received = recvfrom(server_socket, buffer, sizeof("ACK"), 0, (struct sockaddr *)&server_addr, &addr_len)) > 0){
        //     if(strncmp(buffer, "ACK", 3) == 0){
        //         printf("ACK received, started communicating:\n");
        //         break;
        //     }
        // }

        int getting_handshke = rudp_recv(server_socket, server_addr);

        // the recivieng process
        // we're receveing the buffers one by one and writing them down on the new file 
        while ((bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len)) > 0) {
            total_bytes_received += bytes_received;

            if (bytes_received < 0) {
                perror("Error receiving data");
                exit(1);
            }

            if (strstr(buffer, "\exit") != NULL) {
                char* exit_position = strstr(buffer, "\exit");
                size_t bytes_to_write = exit_position - buffer;
                fwrite(buffer, 1, bytes_to_write, file);
                fclose(file);

                end_time = clock();
                total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
                printf("File received and saved as %s (Time taken: %.8f seconds)\n", name, total_time);
                
                double average_bandwidth = (total_bytes_received * 8) / (total_time * 1024 * 1024); // in Mbps
                printf("Average Bandwidth: %.2f Mbps\n", average_bandwidth);

                repeat_counter++;
                sprintf(name, "received_file%d.txt", repeat_counter); 
                file = fopen(name, "wb");
                if (file == NULL) {
                    perror("Error creating file");
                    exit(1);
                }
                
                fwrite(exit_position + strlen("\exit") + 1, 1, bytes_received - bytes_to_write - strlen("\exit") - 1, file);
                start_time = clock();
                continue;
            } else {
                fwrite(buffer, 1, bytes_received, file);
                break;
            }
        } 
    }
    // closing the file 
    fclose(file);

    // Close sockets
    rudp_close(server_socket);
    rudp_close(server_socket);

    return 0;
}
