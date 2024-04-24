#include "l4-common.c"
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define BACKLOG_SIZE 10
#define MAX_CLIENT_COUNT 4
#define MAX_EVENTS 10

#define NAME_OFFSET 0
#define NAME_SIZE 64
#define MESSAGE_OFFSET NAME_SIZE
#define MESSAGE_SIZE 448
#define BUFF_SIZE (NAME_SIZE + MESSAGE_SIZE)

//#define DEBUG

typedef struct {
    int accepted;
    int fd;
    char name[NAME_SIZE];
} user_data_t;

void user_data_init(user_data_t *data, int fd) {
    data->fd = fd;
    data->accepted = 0;
}

void user_data_set(user_data_t *data, char *name) {
    strncpy(data->name, name, NAME_SIZE);
}

typedef struct {
    user_data_t* data_all[MAX_CLIENT_COUNT];
    int taken[MAX_CLIENT_COUNT];
    int takenc;
} data_all_t;

void data_all_init(data_all_t *data) {
    data->takenc = 0;
    memset(data->taken, 0, sizeof(int) * MAX_CLIENT_COUNT);
    memset(data->data_all, 0, sizeof(user_data_t*) * MAX_CLIENT_COUNT);
}

int data_all_takefree(data_all_t *data, user_data_t *user_data) {
    if(data->takenc == MAX_CLIENT_COUNT)
        return 0;
    for(int i = 0; i < MAX_CLIENT_COUNT; ++i) {
        if(!data->taken[i])
        {
            data->data_all[i] = user_data;
            data->taken[i] = 1;
            ++data->takenc;
            return 1;
        }
    }
    return 0;
}

int data_all_free(data_all_t *data, user_data_t *user_data) {
    for(int i = 0; i < MAX_CLIENT_COUNT; ++i) {
        if(data->data_all[i] == user_data)
        {
            data->data_all[i] = NULL;
            data->taken[i] = 0;
            --data->takenc;
            return 1;
        }
    }
    return 0;
}



void accept_user(int fd_epoll, int *max_users, int fd_server) {
    int fd_client = accept(fd_server, NULL, NULL);
#ifdef DEBUG
    printf("\nAccepting a new client...\n");
#endif
    if(fd_client == -1)
        ERR("accept");
    if(*max_users >= MAX_CLIENT_COUNT)
    {
        printf("Too many clients (%d). Dropping connection...\n", *max_users);
        close(fd_client);
        return;
    }
#ifdef DEBUG
    printf("New user accepted!\n");
#endif
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd_client;
    user_data_t *data = malloc(sizeof(user_data_t));
    user_data_init(data, fd_client);
    ev.data.ptr = data;
    if(epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_client, &ev) == -1)
        ERR("epoll_ctl");
    ++(*max_users);
}

void remove_user(int fd_epoll, int *max_users, user_data_t *data, data_all_t *data_all) {
    printf("Removing client...\n");
    if(epoll_ctl(fd_epoll, EPOLL_CTL_DEL, data->fd, NULL))
        ERR("epoll_ctl");
    if(!data_all_free(data_all, data))
        ERR("data_all_free");
    close(data->fd);
    free(data);
    --(*max_users);
}

void authenticate_user(int fd_epoll, uint64_t key_valid, char *buff, user_data_t *data, data_all_t *data_all) {
    char *buff_name = buff + NAME_OFFSET;
    char *buff_message = buff + MESSAGE_OFFSET;
#ifdef DEBUG
    printf("\nHanding client (authentication)...\n");
#endif
    // handle first message (accept or decline)
    printf("User: %s\nprovided key: %s\n", buff_name, buff_message);
    uint64_t key_user = strtoul(buff_message, NULL, 10);
    if(key_user != key_valid)
    {
        printf("Client not authenticated! (Wrong key)\n");
        // remove user and close connection
        if(epoll_ctl(fd_epoll, EPOLL_CTL_DEL, data->fd, NULL))
            ERR("epoll_ctl");
        close(data->fd);
        free(data);
    }
    else
    {
        printf("Client authenticated!\n");
        data->accepted = 1;
        user_data_set(data, buff_name);
        if(!data_all_takefree(data_all, data))
            ERR("data_all_takefree");
        // send message back
        bulk_write(data->fd, buff, BUFF_SIZE);
    }
}

void forward_message(data_all_t *data, user_data_t *user_data, char *buff) {
#ifdef DEBUG
    printf("Forwarding message (%d active users)...\n", data->takenc);
#endif
    for(int i = 0; i < MAX_CLIENT_COUNT; ++i)
        if(data->taken[i] && data->data_all[i]->fd != user_data->fd)
            bulk_write(data->data_all[i]->fd, buff, BUFF_SIZE);
}

void handle_user(char *buff, user_data_t *data, data_all_t *data_all) {
    char *buff_name = buff + NAME_OFFSET;
    char *buff_message = buff + MESSAGE_OFFSET;
#ifdef DEBUG
    printf("\nHanding client (receiving message)...\n");
#endif
    printf("name: %s\nmessage: %s\n\n", buff_name, buff_message);
    forward_message(data_all, data, buff);
}


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
    int max_clients = 0;

    printf("port = %d\n", port);
    printf("key_valid = %lu\n", key_valid);

#ifdef DEBUG
    printf("Binding the socket...\n");
#endif
    int fd_server = bind_tcp_socket(port, 1);

#ifdef DEBUG
    printf("Creating epoll...\n");
#endif
    int fd_epoll = epoll_create1(0);
    if(fd_epoll == -1)
        ERR("epoll_create1");
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = fd_server;
    if(epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_server, &ev) == -1)
        ERR("epoll_ctl");
    
    data_all_t data_all;
    data_all_init(&data_all);
#ifdef DEBUG
    printf("Entering the while loop...\n");
#endif
    int nfds;
    char buff[BUFF_SIZE + 1] = { 0 };
    while(1)
    {
#ifdef DEBUG
        printf("Running epoll_wait...\n");
#endif
        nfds = epoll_wait(fd_epoll, events, MAX_EVENTS, -1);
        if(nfds == -1)
            ERR("epoll_wait");
        for(int i = 0; i < nfds; ++i)
        {
            if(events[i].data.fd == fd_server)
            {
                accept_user(fd_epoll, &max_clients, fd_server);
            }
            else
            {
                user_data_t *data = events[i].data.ptr;
                if(bulk_read(data->fd, buff, BUFF_SIZE) <= 0)
                {
                    remove_user(fd_epoll, &max_clients, data, &data_all);
                }
                else if(!data->accepted)
                {
                    authenticate_user(fd_epoll, key_valid, buff, data, &data_all);
                }
                else
                {
                    handle_user(buff, data, &data_all);
                }
            }
        }
    }
    close(fd_server);
    close(fd_epoll);
    return EXIT_SUCCESS;
}
