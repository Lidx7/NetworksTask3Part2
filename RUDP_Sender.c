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

// Helper function to send data to the receiver
int sendData(int socketfd, void* buffer, int len) {
    int sentd = send(socketfd, buffer, len, 0);
    if (sentd == -1) {
        perror("send");
        exit(1);
    }
    else if (!sentd) {
        printf("Receiver doesn't accept requests.\n");
    }
    else if (sentd < len) {
        printf("Data was only partly sent (%d/%d bytes).\n", sentd, len);
    }
    else {
        printf("Total bytes sent is %d.\n", sentd);
    }

    return sentd;
}

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

    int network_socket = rudp_socket(serverAddress, port, ip);

    printf("Sending handshake request to server...\n");

    // Send handshake request
    sendto(network_socket, "Hello", strlen("Hello"), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    printf("Handshake request sent. Waiting for acknowledgment...\n");

    // Receive handshake acknowledgment
    char buffer[1024];
    socklen_t addr_len = sizeof(serverAddress);
    recvfrom(network_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&serverAddress, &addr_len);
    printf("Handshake acknowledgment received from server: %s\n", buffer);

    printf("Handshake complete. Sending data...\n");

    // Send data
    sendto(network_socket, "Data from client", strlen("Data from client"), 0, (struct sockaddr *)&serverAddress, addr_len);

    printf("Data sent.\n");

    //sending the file and repeating as long as the user wants
    char again;
    do {
        rudp_send(rand_file, network_socket);
        send(network_socket, "\exit", strlen("\exit") + 1, 0);
        printf("Do you want to send the file again? type y for yes, any other character for no\n");
        scanf(" %c", &again);
    } while (again == 'y' || again == 'Y');

    //closing the socket and freeing the memory
    rudp_close(network_socket);
    free(rand_file);

    return 0;
}