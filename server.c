#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "my_signal.h"
#include "my_socket.h"
#include "logUtil.h"

int debug = 0;
volatile sig_atomic_t has_usr1 = 0;

void sig_usr1()
{
    has_usr1 = 1;
    return;
}

int set_so_cork(int sockfd, int value)
{
    int on_off = value;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CORK , &on_off, sizeof(on_off)) < 0) {
        warn("setsockopt cork");
        return -1;
    }

    return 0;
}

int child_proc_accumulate(int connfd, int use_cork, int use_nodelay)
{
    fprintfwt(stderr, "use_cork: %d, use_nodelay: %d\n", use_cork, use_nodelay);

    if (use_nodelay) {
        if (set_so_nodelay(connfd) < 0) {
            errx(1, "set_so_nodelay");
        }
    }

    for (int n_loop = 0; n_loop < 5; ++n_loop) {
        if (use_cork) {
            /* prepare for accumulation */
            if (set_so_cork(connfd, 1) < 0) {
                errx(1, "set_so_cork(, 1)");
            }
        }

        for (int i = 0; i < 10; ++i) {
            unsigned char buf[100];
            int n = write(connfd, buf, sizeof(buf));
            if (n < 0) {
                err(1, "write");
            }
        }

        if (use_cork) {
            /* flush */
            if (set_so_cork(connfd, 0) < 0) {
                errx(1, "set_so_cork(, 0)");
            }
        }
        sleep(1);
    }

    return 0;
}

int child_proc(int connfd, int use_cork, int use_nodelay)
{
    fprintfwt(stderr, "use_cork: %d, use_nodelay: %d\n", use_cork, use_nodelay);

    if (use_nodelay) {
        if (set_so_nodelay(connfd) < 0) {
            errx(1, "set_so_nodelay");
        }
    }

    for (int n_loop = 0; n_loop < 5; ++n_loop) {
        for (int i = 0; i < 10; ++i) {
            unsigned char buf[100];
            if (use_cork) {
                if (set_so_cork(connfd, 1) < 0) {
                    errx(1, "set_so_cork(, 1)");
                }
            }
            int n = write(connfd, buf, sizeof(buf));
            if (n < 0) {
                err(1, "write");
            }
            if (use_cork) {
                if (set_so_cork(connfd, 0) < 0) {
                    errx(1, "set_so_cork(, 0)");
                }
            }
        }
        sleep(1);
    }

    return 0;
}

void sig_chld(int signo)
{
    pid_t pid;
    int   stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        ;
    }
    return;
}

int usage(void)
{
    char *msg = "Usage: server [-p port] [-k] [-D] [-A]\n"
"-p port: port number (1234)\n"
"-k:      use TCP_CORK\n"
"-D:      use TCP_NODELAY\n"
"-A:      Accumulate each write() bytes\n";

    fprintf(stderr, "%s", msg);

    return 0;
}

int main(int argc, char *argv[])
{
    int port = 1234;
    pid_t pid;
    struct sockaddr_in remote;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int listenfd;

    int use_cork       = 0;
    int use_nodelay    = 0;
    int use_accumulate = 0;

    int c;
    while ( (c = getopt(argc, argv, "AdDhkp:")) != -1) {
        switch (c) {
            case 'A':
                use_accumulate = 1;
                break;
            case 'd':
                debug += 1;
                break;
            case 'D':
                use_nodelay = 1;
                break;
            case 'h':
                usage();
                exit(0);
            case 'k':
                use_cork = 1;
                break;
            case 'p':
                port = strtol(optarg, NULL, 0);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    my_signal(SIGCHLD, sig_chld);
    my_signal(SIGPIPE, SIG_IGN);

    listenfd = tcp_listen(port);
    if (listenfd < 0) {
        errx(1, "tcp_listen");
    }

    for ( ; ; ) {
        int connfd = accept(listenfd, (struct sockaddr *)&remote, &addr_len);
        if (connfd < 0) {
            err(1, "accept");
        }
        
        pid = fork();
        if (pid == 0) { // child process
            if (close(listenfd) < 0) {
                err(1, "close listenfd");
            }
            if (use_accumulate) {
                if (child_proc_accumulate(connfd, use_cork, use_nodelay) < 0) {
                    errx(1, "child_proc");
                }
            }
            else {
                if (child_proc(connfd, use_cork, use_nodelay) < 0) {
                    errx(1, "child_proc");
                }
            }
            fprintfwt(stderr, "main: child_proc() returned\n");
            exit(0);
        }
        else { // parent process
            if (close(connfd) < 0) {
                err(1, "close for accept socket of parent");
            }
        }
    }
        
    return 0;
}
