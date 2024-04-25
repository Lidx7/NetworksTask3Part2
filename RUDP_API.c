#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>

#define MAX_DATA_SIZE 1024
#define TRUE 1
#define FALSE 0

typedef struct {
    int seq_num;
    char data[MAX_DATA_SIZE];
    uint16_t checksum;
} Packet;


/*TODO: disolve these globals into function-specific variables*/
int seq_num = 0;
//could possibly add a packet loss send retries counter here


int rudp_socket(struct sockaddr_in addr, int port, char* ip) {
    int sockfd;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }


    // // Configure server address
    // memset(&addr, 0, sizeof(addr));
    // addr.sin_family = AF_INET;
    // addr.sin_port = htons (port);
    // addr.sin_addr.s_addr = (ip);

    // // Bind socket to server address
    // if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    //     perror("Socket bind failed");
    //     exit(1);
    // }

    return sockfd;
    
}


int rudp_recv(){
    int sockfd;
    struct sockaddr_in temp_addr;
    socklen_t addr_len = sizeof(struct sockaddr);
    char buffer[MAX_DATA_SIZE + sizeof(int)]; //TODO: change the sizeof section to represent the header (should use a MACRO)

    while (TRUE) {
        // Receive packet from client
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&temp_addr, &addr_len);
        if (bytes_received == -1) {
            perror("Receive error");
            continue;
        }

        Packet *packet = (Packet *)buffer;
        

        // Check sequence number
        if (packet->seq_num == seq_num) {
            printf("Received packet with sequence number %d\n", seq_num);

            // Acknowledge received packet
            sendto(sockfd, &seq_num, sizeof(int), 0, (struct sockaddr *)&temp_addr, addr_len);

            // Print received data
            printf("Data: %s\n", packet->data);

            // Update sequence number
            seq_num = (seq_num + 1) % 2;
        } else {
            printf("Received out-of-order packet. Discarding...\n");
        }

    }
}


void rudp_send(const char *data, int sockfd) {
    struct sockaddr_in temp_addr;
    socklen_t addr_len = sizeof(struct sockaddr);
    Packet packet;
    packet.seq_num = seq_num;
    strncpy(packet.data, data, MAX_DATA_SIZE);

    // Send packet
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&temp_addr, addr_len);
    
    // Wait for acknowledgment
    while (TRUE) {
        // Set timeout for acknowledgment
        struct timeval tv;
        tv.tv_sec = 1; // 1 second timeout
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        int ack_seq_num;
        int bytes_received = recvfrom(sockfd, &ack_seq_num, sizeof(int), 0, (struct sockaddr *)&temp_addr, &addr_len);
        if (bytes_received != -1 && ack_seq_num == seq_num) {
            printf("Packet with sequence number %d acknowledged.\n", seq_num);
            break;
        } else {
            printf("Timeout. Retransmitting packet...\n");
            sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&temp_addr, addr_len);
            
        }
    }
}


void rudp_close(int socket){
    close(socket);
    return;
}

