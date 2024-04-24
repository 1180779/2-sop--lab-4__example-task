#include "l4-common.c"
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
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

    char *key = argv[2];
    uint64_t key_valid = strtoul(key, NULL, 10);

    printf("port = %d\n", port);
    printf("key_valid = %lu\n", key_valid);

    printf("Binding the socket...\n");
    int fd_server = bind_tcp_socket(port, 1);

    for(int k = 0; k < 5; ++k)
    {
        printf("\nWaiting for client...\n");
        int fd_client = add_new_client(fd_server);
        printf("Accepted a client!\n");

        char buff[BUFF_SIZE + 1] = { 0 };
        int read = bulk_read(fd_client, buff, BUFF_SIZE);
        printf("received = %d bytes\n\n", read);

        printf("name = %s\nkey = %s\n", buff, buff + MESSAGE_OFFSET);
        uint64_t key_user = strtoul(buff + MESSAGE_OFFSET, NULL, 10);
        if(key_valid == key_user)
        {
            printf("correct key! sending the message back.\n");
            bulk_write(fd_client, buff, BUFF_SIZE);
        }
        else
        {
            printf("invalid key!\n");
            close(fd_client);
            close(fd_server);
            return EXIT_FAILURE;
        }
        printf("Disconnecting...\n");
        close(fd_client); 
    }
    close(fd_server);

    return EXIT_SUCCESS;
}
