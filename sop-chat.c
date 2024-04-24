#include "l4-common.c"
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

#define BACKLOG_SIZE 10
#define MAX_CLIENT_COUNT 4
#define MAX_EVENTS 10

#define NAME_OFFSET 0
#define NAME_SIZE 64
#define MESSAGE_OFFSET NAME_SIZE
#define MESSAGE_SIZE 448
#define BUFF_SIZE (NAME_SIZE + MESSAGE_SIZE)

void usage(char *program_name) {
    fprintf(stderr, "USAGE: %s port key\n", program_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    char *program_name = argv[0];
    if (argc != 3) {
        usage(program_name);
    }

    uint16_t port = atoi(argv[1]);
    if (port == 0){
        usage(argv[0]);
    }

    // char *key = argv[2];
    // char *address = argv[1];


    int fd_server = bind_tcp_socket(port, 1);
    int fd_client = add_new_client(fd_server);
    close(fd_server);

    // char buff[BUFF_SIZE];
    // while(bulk_read(fd_client, buff, BUFF_SIZE - 1) > 0)
        // printf("%s", buff);
    close(fd_client); 


    return EXIT_SUCCESS;
}
