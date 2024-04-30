#ifndef RUDP_API_H
#define RUDP_API_H

#include <netinet/in.h>

#define MAX_DATA_SIZE 16215
#define TRUE 1
#define FALSE 0
#define RETRIES 5

typedef struct _Packet{
    int seq_num;
    uint16_t checksum;
    uint16_t length;
    unsigned short flag;
    char data[MAX_DATA_SIZE];
} Packet;

unsigned short int calculate_checksum(void *data, unsigned int bytes);
char *util_generate_random_data(int size);
void rudp_close(int socket);
void initPacket(Packet *packet, int seq_num, __u_short flags, char* data);
int rudp_socket();
void rudp_send(const char *data, int sockfd, __u_short flag, struct sockaddr_in* recv_addr, int data_length, int seq_num);
int rudp_recv(int sockfd, struct sockaddr_in* recv_addr);
int senderHandshake(int sockfd, struct sockaddr_in *serverAddr);
int receiverHandshake(int sockfd, struct sockaddr_in *clientAddr);

#endif /* RUDP_API_H */
