#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>

#define MAX_DATA_SIZE 16215
#define TRUE 1
#define FALSE 0
#define RETRIES 20

typedef struct _Packet{
    int seq_num;
    uint16_t checksum;
    uint16_t length;
    unsigned short flag;
    char data[MAX_DATA_SIZE];
} Packet;

struct timeval start_time, end_time;
int total_bytes_received = 0;

void printStats(){
    double total_time = (double)(end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec + start_time.tv_usec)/ 1e6;
    printf("Total bytes received: %d\n", total_bytes_received);
    printf("Total time taken: %.8f seconds\n", total_time);
    printf("Average throughput: %f bytes/second\n", (double)total_bytes_received / total_time);
}

unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;
    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
    // Add left-over byte, if any
    if (bytes > 0){
        total_sum += *((unsigned char *)data_pointer);
    }
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16){
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    }
    return (~((unsigned short int)total_sum));
}

char *util_generate_random_data(int size) {
    char *rand_file_ret = NULL;
    if (size == 0)
        return NULL;
    rand_file_ret = (char *)calloc(size, sizeof(char));
    if (rand_file_ret == NULL)
        return NULL;
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(rand_file_ret + i) = ((unsigned int)rand() % 256);
    return rand_file_ret;
}


void rudp_close(int socket){
    close(socket);
    return;
}

void initPacket(Packet *packet, int seq_num, __u_short flags, char* data){
    packet->seq_num = seq_num;
    packet->flag = flags;
    packet->checksum = 0;
    packet->length = strlen(data);
    memcpy(packet->data, data, packet->length);
}


int rudp_socket() {
    int sockfd;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    return sockfd;
    
}


void rudp_send(const char *data, int sockfd, unsigned short flag, struct sockaddr_in* recv_addr, int data_length, int seq_num) {
    // Create a packet for this chunk of data
    
    

    //int seq_num = 0; // Initialize sequence number

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));

    socklen_t recv_addr_len = sizeof(*recv_addr);        
    int num_packets = (data_length + MAX_DATA_SIZE - 1) / MAX_DATA_SIZE; // Calculate the number of packets needed


    for (int i = 0; i < num_packets; i++) {
        Packet packet;
        packet.flag = flag;
        int remaining_data = data_length - (i) * MAX_DATA_SIZE;
        int chunk_size = remaining_data > MAX_DATA_SIZE ? MAX_DATA_SIZE : remaining_data;

        packet.seq_num = seq_num;
        packet.length = chunk_size;
        memcpy(packet.data, data + i * MAX_DATA_SIZE, chunk_size);
        packet.checksum = calculate_checksum(packet.data, packet.length);

        int c = 0;
        int retries = RETRIES;
        while (c<retries){
            printf("sending packet with seq_num %d\n", seq_num);
            if(sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)recv_addr, sizeof(*recv_addr)) < 0){
                perror("sendto failed");
                exit(1);
            }
            printf("packet sent\n");
            if(recvfrom(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)recv_addr, &recv_addr_len) < 0){
                perror("ack not received");
                c++;
            }
            else{
                printf("ack received\n");
                break;
            }

        }
        seq_num++;
        
    }
    return;
}


int rudp_recv(int sockfd, struct sockaddr_in* recv_addr){
    socklen_t addr_len = sizeof(struct sockaddr);
    Packet *buffer = (Packet *)malloc(sizeof(Packet));
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        exit(1); // Exit with error
    }
    int seq_num = 0;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));


    while (TRUE) {
        // Receive packet from client
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(Packet) + buffer->length, 0, (struct sockaddr *) recv_addr, &addr_len);
        total_bytes_received += bytes_received;
        if(bytes_received < 0){
            printf("recvfrom failed (rudp_recv)\n"); 
        }
        
        if(buffer->flag == 1){
            printf("received SYN");
            rudp_send("ACK", sockfd, 2, recv_addr, 0, seq_num);
        }
        if(buffer->flag == 2){
            printf("received ACK");
            free(buffer);
            return 0;
        }
        if(buffer->flag == 3){
            printf("received FIN. closing...\n");
            rudp_send(NULL, sockfd, 3, recv_addr, 0, seq_num);
            gettimeofday(&end_time, NULL);
            printStats();
            rudp_close(sockfd);
            exit(0);
        }
        if(buffer->flag == 4){
            seq_num = 0;
        }
        if(buffer->flag == 0 && buffer->checksum == calculate_checksum(buffer->data, buffer->length)){
            if ((buffer->seq_num)%10 == seq_num%10) {
                printf("Received packet with sequence number %d\n", seq_num);

                Packet ack_packet;
                ack_packet.flag = 2;
                sendto(sockfd, &ack_packet, sizeof(Packet), 0, (struct sockaddr *)recv_addr, addr_len);

                seq_num = (seq_num + 1)%10;// Update sequence number
            } else {
                printf("Received out-of-order packet. Discarding...\n");
                return 1;
            }
        }

    }
    free(buffer);
    return 0;
}






int senderHandshake(int sockfd, struct sockaddr_in *serverAddr) {
    // struct timeval timeout;
    // timeout.tv_sec = 0;
    // timeout.tv_usec = 500000;
    // setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));

    Packet handshake_send; //sender's packet
    handshake_send.flag = 1; //SYN flag
    socklen_t serverAddrLen = sizeof(*serverAddr);
    Packet handshake_ack;
    
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
    if ((recvfrom(sockfd, &handshake_ack, sizeof(handshake_ack), 0, (struct sockaddr *)serverAddr, &serverAddrLen)) < 0) {
        perror("recvfrom failed");
        return 1; // Handshake failed
    }

    if(handshake_ack.flag == 2){
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
    Packet handshake_ack;


    /*******************************************/
    //receiver receiving SYN and sending back ACK
        if ((recvfrom(sockfd, &handshake_recv, sizeof(handshake_recv), 0, (struct sockaddr *)clientAddr, &clientAddrLen)) < 0) {
            perror("recvfrom failed\n");
            return -1; // Error receiving handshake packet
        }

        if(handshake_recv.flag == 1){
            printf("SYN received. sending ACK\n");
            handshake_ack.flag = 2; //ACK flag

            if(sendto(sockfd, &handshake_ack, sizeof(handshake_ack), 0, (const struct sockaddr *)clientAddr, sizeof(*clientAddr)) < 0){
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



















        // while (retries){
        //     Packet tempPacket;
        //     // Set timeout for acknowledgment
        //     struct timeval tv;
        //     tv.tv_sec = 0;
        //     tv.tv_usec = 500000; // 0.5 second timeout

        //     setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        //     int bytes_received = recvfrom(sockfd, &tempPacket, sizeof(tempPacket), 0, NULL, &addr_len);
        //     if(bytes_received== -1) printf("ronaldoooooo");
        //     if (bytes_received >= 0 /*&& tempPacket.seq_num == packet.seq_num*/) {
        //         printf("Packet with sequence number %d acknowledged.\n", packet.seq_num);
        //         packet.seq_num++;
        //         retries = RETRIES;
        //         break;
            
        //     } else {
        //         printf("Timeout or incorrect acknowledgment. Retransmitting packet...\n");
        //         // Resend packet
        //         sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)recv_addr, addr_len);
        //         retries--;
        //     }
        
    
        // }