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

        int bytes_received = recvfrom(sockfd, &tempPacket, sizeof(int), 0, (struct sockaddr *)&recv_addr, &addr_len);
        if (bytes_received != -1 && tempPacket.seq_num == seq_num) {
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
    char buffer[MAX_DATA_SIZE];

    while (TRUE) {
        // Receive packet from client
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_addr, &addr_len);
        if (bytes_received == -1) {
            perror("Receive error");
            continue;
        }

        Packet *packet = (Packet *)buffer;
        
        if(packet->flag == 1){
            printf("received SYN");
            rudp_send("ACK", sockfd, 2, recv_addr);
        }
        if(packet->flag == 2){
            printf("received ACK");
            return 0;
        }
        if(packet-> flag == 0){
            if (packet->seq_num == seq_num) {
                printf("Received packet with sequence number %d\n", seq_num);
                char seq_num_c = seq_num + '0';
                rudp_send(seq_num_c, sockfd, 2, recv_addr);// Acknowledge received packet
                printf("Data: %s\n", packet->data);// Print received data
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




int performHandshake(int sockfd_send, struct sockaddr_in *serverAddr,int sockfd_recv, struct sockaddr_in *clientAddr) {
    struct timeval timeout;
    timeout.tv_sec = 6;
    timeout.tv_usec = 0;
    setsockopt(sockfd_send, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    setsockopt(sockfd_recv, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    Packet handshakePacket_send; //sender's packet
    handshakePacket_send.flag = 1; //SYN flag

    Packet handshakePacket_recv; //receiver's packet
    socklen_t clientAddrLen = sizeof(*clientAddr);
    
    /*****************************************/
    //sender Sending SYN packet to the receiver
    if (sendto(sockfd_send, &handshakePacket_send, ntohs(handshakePacket_send.length), 0, (const struct sockaddr *)serverAddr, sizeof(*serverAddr)) < 0) {
        perror("sendto failed\n");
        return -1; // Error sending handshake packet
    }
    printf("SYN packet sent.\n");


    /*******************************************/
    //receiver receiving SYN and sending back ACK
    if ((recvfrom(sockfd_recv, &handshakePacket_recv, sizeof(handshakePacket_recv), 0, (struct sockaddr *)clientAddr, &clientAddrLen)) < 0) {
        perror("recvfrom failed\n");
        return -1; // Error receiving handshake packet
    }

    if(handshakePacket_recv.flag == 1){
        printf("SYN received. sending ACK\n");
        handshakePacket_recv.flag = 2; //ACK flag

        if(sendto(sockfd_recv, &handshakePacket_recv, ntohs(handshakePacket_recv.length), 0, (const struct sockaddr *)clientAddr, sizeof(*clientAddr)) < 0){
            perror("sendto failed\n");
            return -1;
        }
    }
    else{
        printf("Hasn't received SYN. aborting\n");
        return 1;
    }


    /**************************************************/
    // sender waits for acknowledgment from the receiver
    if (recvfrom(sockfd_send, &handshakePacket_send, sizeof(handshakePacket_send), 0, (struct sockaddr *)serverAddr, sizeof(*serverAddr))) {
        printf("Acknowledgment for handshake not received. Handshake failed.\n");
        return 1; // Handshake failed
    }

    if(handshakePacket_send.flag == 2){
        printf("ACK received\n");
    }
    else{
        printf("no ACK received. aborting\n");
    }


    printf("Handshake successful.\n");
    return 0; // Handshake successful
}
