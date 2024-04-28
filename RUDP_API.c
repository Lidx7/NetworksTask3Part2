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

typedef struct _Packet{
    int seq_num;
    uint16_t checksum;
    uint16_t length;
    unsigned short flag;
    char data[MAX_DATA_SIZE];
} Packet;


/*TODO: disolve these globals into function-specific variables*/
int seq_num = 0;
//could possibly add a packet loss send retries counter here

void initPacket(Packet *packet, int seq_num, __u_short flags, char* data){
    *packet->data = *data;
    packet->seq_num = seq_num;
    packet->flag = flags;
    packet->checksum = 0;
    packet->length = sizeof(Packet);

}

int rudp_socket(struct sockaddr_in addr, int port, char* ip) {
    int sockfd;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    return sockfd;
    
}


void rudp_send(const char *data, int sockfd, __u_short flag, struct sockaddr_in recv_addr) {;
    socklen_t addr_len = sizeof(struct sockaddr);
    Packet packet;
    packet.seq_num = seq_num;
    packet.flag = flag;
    strncpy(packet.data, data, MAX_DATA_SIZE);
    packet.length = sizeof(packet);
    Packet tempPacket; 

    // Send packet
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recv_addr, addr_len);
    
    // Wait for acknowledgment
    while (TRUE) {
        // Set timeout for acknowledgment
        struct timeval tv;
        tv.tv_sec = 6; // 1 second timeout
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        int bytes_received = recvfrom(sockfd, &tempPacket, sizeof(tempPacket), 0, (struct sockaddr *)&recv_addr, &addr_len);
        printf("%d", bytes_received);/////////////////////////////////////////////////////////
        if (bytes_received != -1 /*&& tempPacket.seq_num == seq_num*/) {
            printf("Packet with sequence number %d acknowledged.\n", seq_num);
            break;
        } else {
            printf("Timeout. Retransmitting packet...\n");
            sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recv_addr, addr_len);
            
        }
    }
}


int rudp_recv(int sockfd, struct sockaddr_in recv_addr){
    socklen_t addr_len = sizeof(struct sockaddr);
    //char buffer[MAX_DATA_SIZE];
    Packet *buffer;


    while (TRUE) {
        // Receive packet from client
        int bytes_received = recvfrom(sockfd, &buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_addr, &addr_len);
        if (bytes_received == -1) {
            perror("Receive error");
            continue;
        }

        
        if(buffer->flag == 1){
            printf("received SYN");
            rudp_send("ACK", sockfd, 2, recv_addr);
        }
        if(buffer->flag == 2){
            printf("received ACK");
            return 0;
        }
        if(buffer-> flag == 0){
            if (buffer->seq_num == seq_num) {
                printf("Received packet with sequence number %d\n", seq_num);
                char seq_num_c = seq_num + '0';
                rudp_send(&seq_num_c, sockfd, 2, recv_addr);// Acknowledge received packet
                printf("Data: %s\n", buffer->data);// Print received data
                seq_num = (seq_num + 1);// Update sequence number
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




int senderHandshake(int sockfd, struct sockaddr_in *serverAddr) {
    struct timeval timeout;
    timeout.tv_sec = 6;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));

    Packet handshake_send; //sender's packet
    initPacket(&handshake_send, 0, 1, "SYN");
    handshake_send.flag = 1; //SYN flag
    socklen_t serverAddrLen = sizeof(*serverAddr);
    
    /*****************************************/
    //sender Sending SYN packet to the receiver
    if(sendto(sockfd, &handshake_send, (sizeof(handshake_send)), 0, (const struct sockaddr *)serverAddr, sizeof(*serverAddr)) < 0) {
        perror("sendto failed");
        return -1; // Error sending handshake packet
    }
    else{
        printf("SYN packet sent.\n");
    }

    /**************************************************/
    // sender waits for acknowledgment from the receiver
    if ((recvfrom(sockfd, &handshake_send, sizeof(handshake_send), 0, (struct sockaddr *)serverAddr, &serverAddrLen)) < 0) {
        perror("recvfrom failed");
        return 1; // Handshake failed
    }

    if(handshake_send.flag == 2){
        printf("ACK received\n");
    }
    else{
        printf("no ACK received. aborting\n");
    }

    return 0; // Handshake successful
}


int receiverHandshake(int sockfd, struct sockaddr_in *clientAddr){
    Packet handshake_recv; //receiver's packet
    socklen_t clientAddrLen = sizeof(*clientAddr);

    /*******************************************/
    //receiver receiving SYN and sending back ACK
    if ((recvfrom(sockfd, &handshake_recv, sizeof(handshake_recv), 0, (struct sockaddr *)clientAddr, &clientAddrLen)) < 0) {
        perror("recvfrom failed\n");
        return -1; // Error receiving handshake packet
    }

    if(handshake_recv.flag == 1){
        printf("SYN received. sending ACK\n");
        handshake_recv.flag = 2; //ACK flag

        if(sendto(sockfd, &handshake_recv, ntohs(handshake_recv.length), 0, (const struct sockaddr *)clientAddr, sizeof(*clientAddr)) < 0){
            perror("sendto failed\n");
            return -1;
        }
    }
    else{
        printf("Hasn't received SYN. aborting\n");
        return 1;
    }

    return 0; //success
}
