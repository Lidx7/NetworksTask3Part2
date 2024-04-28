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
    char buffer[1024];
    socklen_t addr_len = sizeof(serverAddress);
    int send_socket = rudp_socket(serverAddress, port, ip);

    if(senderHandshake(send_socket, &serverAddress) != 0){
        printf("handshake error. aborting\n");
        exit(1);
    }
    else{
        printf("handshake successful\n");
    }

    //sending the file and repeating as long as the user wants
    char again;
    do {
        rudp_send(rand_file, send_socket, 0, serverAddress);
        send(send_socket, "\exit", strlen("\exit") + 1, 0);
        printf("Do you want to send the file again? type y for yes, any other character for no\n");
        scanf(" %c", &again);
    } while (again == 'y' || again == 'Y');

    //closing the socket and freeing the memory
    rudp_close(send_socket);
    free(rand_file);

    return 0;
}