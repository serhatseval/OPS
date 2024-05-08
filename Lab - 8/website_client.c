#include "common.h"
#include <stdint.h>

#define SERVER_IP "127.0.0.1"

int main(int argc, char** argv) {

    if(argc < 2) {
        ERR("Usage: <port>");
    }
    int client_fd;
    int16_t sum;


    // Create socket and connect to server
    client_fd = connect_tcp_socket(SERVER_IP, argv[1]);

    char MESSAGE[20];
    sprintf(MESSAGE, "%d", getpid());
    // Send data
    if (bulk_write(client_fd, MESSAGE, strlen(MESSAGE)) < 0) {
        ERR("bulk_write");
    }

    
    
    // Close connection
    if (close(client_fd) < 0) {
        ERR("close");
    }

    return 0;
}