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
    uint16_t checksum;
    uint16_t length;
    __u_short flag;
    char data[MAX_DATA_SIZE];
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

    return sockfd;
    
}


void rudp_send(const char *data, int sockfd, __u_short flag, struct sockaddr_in temp_addr) {;
    socklen_t addr_len = sizeof(struct sockaddr);
    Packet packet;
    packet.seq_num = seq_num;
    packet.flag = flag;
    strncpy(packet.data, data, MAX_DATA_SIZE);
    packet.length = sizeof(packet);
    Packet tempPacket; 

    // Send packet
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&temp_addr, addr_len);
    
    // Wait for acknowledgment
    while (TRUE) {
        // Set timeout for acknowledgment
        struct timeval tv;
        tv.tv_sec = 6; // 1 second timeout
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        int bytes_received = recvfrom(sockfd, &tempPacket, sizeof(int), 0, (struct sockaddr *)&temp_addr, &addr_len);
        if (bytes_received != -1 && tempPacket.seq_num == seq_num) {
            printf("Packet with sequence number %d acknowledged.\n", seq_num);
            break;
        } else {
            printf("Timeout. Retransmitting packet...\n");
            sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&temp_addr, addr_len);
            
        }
    }
}


int rudp_recv(int sockfd, struct sockaddr_in temp_addr){
    socklen_t addr_len = sizeof(struct sockaddr);
    char buffer[sizeof(Packet)];

    while (TRUE) {
        // Receive packet from client
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&temp_addr, &addr_len);
        if (bytes_received == -1) {
            perror("Receive error");
            continue;
        }

        Packet *packet = (Packet *)buffer;
        
        if(packet-> flag == 1){
            printf("received SYN");
            rudp_send("ACK", sockfd, 2, temp_addr);
        }
        if(packet-> flag == 2){
            printf("received ACK");
        }
        if(packet-> flag == 0){
        // Check sequence number
            if (packet->seq_num == seq_num) {
                printf("Received packet with sequence number %d\n", seq_num);
                char seq_num_c = seq_num + '0';
                // Acknowledge received packet
                rudp_send(&seq_num_c, sockfd, 2, temp_addr);
                // Print received data
                printf("Data: %s\n", packet->data);

                // Update sequence number
                seq_num = (seq_num + 1);
            } else {
                printf("Received out-of-order packet. Discarding...\n");
                return 1;
            }
        }

    }
}


void rudp_close(int socket){
    close(socket);
    return;
}

