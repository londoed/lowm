/**
 * Copyright (C) 2021, Eric Londo <londoed@comast.net>, { src/lopc.c }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/un.h>
#include <unistd.h>

#include "helpers.h"
#include "common.h"

int
main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_un sock_address;
    char msg[BUF_SIZE], rsp[BUF_SIZE];

    if (argc < 2)
        lowm_err("[!] ERROR: lowm: No arguments given\n");

    sock_address.sun_family = AF_UNIX;
    char *sp;

    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        lowm_err("[!] ERROR: lowm: Failed to create UNIX socket\n");

    sp = getenv(SOCKET_ENV_VAR);

    if (sp != NULL) {
        snprintf(sock_address.sun_path, sizeof(sock_address.sun_path), "%s", sp);
    } else {
        char *host = NULL;
        int dn = 0, sn = 0;

        if (xcb_parse_display(NULL, &host, &dn, &sn) != 0)
            snprintf(sock_address.sun_path, sizeof(sock_address.sun_path),
                SOCKET_PATH_TPL, host, dn, sn);

        free(host);
    }

    if (connect(sock_fd, (struct sockaddr *)&sock_address, sizeof(sock_address)) == -1)
        lowm_err("[!] ERROR: lowm: Failed to connect to UNIX socket\n");

    argc--;
    argv++;
    int msg_len = 0, offset;

    for (offset = 0, rem = sizeof(msg), n = 0; argc > 0 && rem > 0; offset +=n, rem -= n, argc--, argv++) {
        n = snprintf(msg + offset, rem, "%s%c", *argv, 0);
        msg_len += n;
    }

    if (send(sock_fd, msg, msg_len, 0) == -1)
        lowm_err("[!] ERROR: lowm: Failed to send the data\n");

    int ret = EXIT_SUCCESS, nb;

    struct pollfd fds[] = {
        { sock_fd, POLLIN, 0 },
        { STDOUT_FILENO, POLLHUP, 0 },
    };

    while (poll(fds, 2, -1) > 0) {
        if (fds[0].revents & POLLIN) {
            if ((nb = recv(sock_fd, rsp, sizeof(rsp) - 1, 0)) > 0) {
                rsp[nb] = '\0';

                if (rsp[0] == FAILURE_MESSAGE[0]) {
                    ret = EXIT_FAILURE;
                    fprintf(stderr, "%s", rsp + 1);
                    fflush(stderr);
                } else {
                    fprintf(stdout, "%s", rsp);
                    fflush(stdout);
                }
            } else {
                break;
            }
        }

        if (fds[1].revents & (POLLERR | POLLHUP))
            break;
    }

    close(sock_fd);

    return ret;
}
