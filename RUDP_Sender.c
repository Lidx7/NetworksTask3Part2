#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "RandomFileGen.c"
#include "RUDP_API.c"


#define TRUE 1
#define FALSE 0
#define MIN_SIZE 2097152
#define FILE_SIZE 2000000 


int main(int argc, char* argv[]) {
    // correct argument input check
    if (argc < 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }
    char* ip = argv[1];
    int port = atoi(argv[2]);

    //enter desiered size of the random file
    int file_size;
    do{
        printf("Enter the size of the file to be sent (minimum size is 2MB): ");
        scanf("%d", &file_size);
    } while(file_size < MIN_SIZE);
    char* rand_file = util_generate_random_data(file_size);

    //creating socket
    struct sockaddr_in serverAddress;
    int sender_socket = rudp_socket(serverAddress, port, ip);
    char buffer[1024];
    socklen_t addr_len = sizeof(serverAddress);

    // Send SYN
    printf("Sending handshake request to server...\n");
    rudp_send("SYN", sender_socket, 1, serverAddress);
    //sendto(sender_socket, "SYN", sizeof("SYN"), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    printf("Handshake request sent. Waiting for acknowledgment...\n");

    // Receive ACK
    recvfrom(sender_socket, buffer, sizeof("ACK"), 0, (struct sockaddr *)&serverAddress, &addr_len);
    printf("Handshake acknowledgment received from server: %s\n", buffer);

    printf("Handshake complete. Sending data...\n");


// // Wait for acknowledgment
//     while (TRUE) {
//         // Set timeout for acknowledgment
//         struct timeval tv;
//         tv.tv_sec = 6; // 1 second timeout
//         tv.tv_usec = 0;
//         int ex_seq_num = 0;
//         setsockopt(sender_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

//         int bytes_received = recvfrom(sender_socket, &buffer, sizeof(int), 0, (struct sockaddr *)&serverAddress, &addr_len);
//         Packet *packet = (Packet *)buffer;

//         if (bytes_received != -1 && ((packet)->seq_num == ex_seq_num)) {
//             printf("Packet with sequence number %d acknowledged.\n", ex_seq_num);
//             ex_seq_num++;
//             break;
//         } else {
//             printf("Timeout. Retransmitting packet...\n");
//             sendto(sender_socket, &packet, sizeof(packet), 0, (struct sockaddr *)&serverAddress, addr_len);
            
//         }
//     }


    //sending the file and repeating as long as the user wants
    char again;
    do {
        rudp_send(rand_file, sender_socket, 0, serverAddress);
        send(sender_socket, "\exit", strlen("\exit") + 1, 0);
        printf("Do you want to send the file again? type y for yes, any other character for no\n");
        scanf(" %c", &again);
    } while (again == 'y' || again == 'Y');

    //closing the socket and freeing the memory
    rudp_close(sender_socket);
    free(rand_file);

    return 0;
}