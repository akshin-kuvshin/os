// author: Danila "akshin_" Axyonov
// Linux specific!

// Compiler: gcc
// Output: main.out

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <iso646.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <signal.h>

#define SERVER_DOMAIN AF_INET
#define ERROR -1
#define PORT 5267
#define BACKLOG 13
#define EMPTY -1
#define BUFFER_SIZE 4096

int result;
volatile sig_atomic_t was_sighup = 0,
                      was_sigterm = 0;

void restore_old_mask(int, void* arg);
void close_server(int, void* arg);
void configure_server(int server_fd);
void setup_signal_handler(int sig_num, const char* const sig_name, void(* sig_handler)(int));
void sighup_handler(int);
void sigterm_handler(int);

int main() {
    sigset_t block_mask;
    sigset_t* old_mask = malloc(sizeof(sigset_t));
    assert(old_mask and "couldn't allocate memory for old_mask");
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGHUP);
    sigaddset(&block_mask, SIGTERM);
    result = sigprocmask(SIG_BLOCK, &block_mask, old_mask);
    if (result == ERROR) {
        perror("couldn't block SIGHUP and SIGTERM in main thread");
        exit(EXIT_FAILURE);
    }
    result = on_exit(restore_old_mask, old_mask);
    if (result == ERROR) {
        perror("couldn't register restore_old_mask to on_exit");
        restore_old_mask(EXIT_FAILURE, old_mask);
        exit(EXIT_FAILURE);
    }


    setup_signal_handler(SIGHUP, "SIGHUP", sighup_handler);
    setup_signal_handler(SIGTERM, "SIGTERM", sigterm_handler);


    int server_fd = socket(SERVER_DOMAIN, SOCK_STREAM, 0);
    if (server_fd == ERROR) {
        perror("couldn't create socket");
        exit(EXIT_FAILURE);
    }
    result = on_exit(close_server, (void*)(intptr_t)server_fd);
    if (result == ERROR) {
        perror("couldn't register close_server to on_exit");
        close_server(EXIT_FAILURE, (void*)(intptr_t)server_fd);
        exit(EXIT_FAILURE);
    }

    configure_server(server_fd);





    printf("[server initiated]: port = %d | pid = %d\n", PORT, getpid());

    int client_fd = EMPTY;
    bool finish = false;
    while (not finish) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;
        if (client_fd != EMPTY) {
            FD_SET(client_fd, &read_fds);
            if (max_fd < client_fd)
                max_fd = client_fd;
        }

        int fd_ready = pselect(
            max_fd + 1,
            &read_fds,
            NULL,
            NULL,
            NULL,
            old_mask
        );

        // fd_ready == 0 => timeout exceeded - impossible, because we provided __timeout = NULL
        if (fd_ready == ERROR) {
            if (errno == EINTR) {
                if (was_sighup) {
                    printf("# SIGHUP signal was received\n");
                    was_sighup = 0;
                }
                if (was_sigterm) {
                    printf("# SIGTERM signal was received - finishing main loop...\n");
                    finish = true;
                    was_sigterm = 0;
                }
            } else {
                perror("unexpected error occured");
                exit(EXIT_FAILURE);
            }
        } else if (fd_ready > 0) {
            if (client_fd != EMPTY and FD_ISSET(client_fd, &read_fds)) {
                char buffer[BUFFER_SIZE];
                ssize_t n = read(client_fd, buffer, BUFFER_SIZE);
                if (n <= 0) {
                    if (n == ERROR)
                        fprintf(stderr, "client connection (fd = %d) closed due to error: %s\n", client_fd, strerror(errno));
                    else // n == 0
                        printf("client connection (fd = %d) closed\n", client_fd);
                    close(client_fd);
                    client_fd = EMPTY;
                } else // n > 0
                    printf("%ld bytes were received through client connection (fd = %d)\n", n, client_fd);
            }
            if (FD_ISSET(server_fd, &read_fds)) {
                int new_client_fd = accept(server_fd, NULL, NULL);
                if (client_fd == EMPTY) {
                    printf("new client connection accepted (fd = %d)\n", new_client_fd);
                    client_fd = new_client_fd;
                } else { // client_fd != EMPTY
                    printf("new client connection accepted (fd = %d) [closed, because another client connection exists (fd = %d)]\n", new_client_fd, client_fd);
                    close(new_client_fd);
                }
            }
        }
    }





    if (client_fd != EMPTY)
        close(client_fd);
    printf("done.\n");
    exit(EXIT_SUCCESS);
}

void restore_old_mask(int, void* arg) {
    sigset_t* old_mask = arg;
    result = sigprocmask(SIG_SETMASK, old_mask, NULL);
    if (result == ERROR)
        perror("couldn't unblock SIGHUP and SIGTERM in main thread");
    free(old_mask);
}

void close_server(int, void* arg) {
    int server_fd = (int)(intptr_t)arg;
    close(server_fd);
}

void configure_server(int server_fd) {
    int opt = 1;
    result = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (result == ERROR) {
        perror("couldn't enable local address and port reusage");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = SERVER_DOMAIN;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    result = bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    if (result == ERROR) {
        perror("couldn't bind socket to address");
        exit(EXIT_FAILURE);
    }

    result = listen(server_fd, BACKLOG);
    if (result == ERROR) {
        perror("couldn't turn socket into listen mode");
        exit(EXIT_FAILURE);
    }
}

void setup_signal_handler(int sig_num, const char* const sig_name, void(* sig_handler)(int)) {
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;
    result = sigaction(sig_num, &sig_action, NULL);
    if (result == ERROR) {
        fprintf(stderr, "couldn't set up %s's handler: %s\n", sig_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void sighup_handler(int) {
    was_sighup = 1;
}

void sigterm_handler(int) {
    was_sigterm = 1;
}
