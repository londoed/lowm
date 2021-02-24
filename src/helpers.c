/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/helpers.h }
 * This software is distributed under the GNU General Public License Version 2.
 * See the file LICENSE for details.
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "lowm.h"

void
warn(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

__attribute__((noreturn))
void
err(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

char *
read_string(const char *file_path, size_t *tlen)
{
    if (file_path == NULL)
        return NULL;

    int fd = open(file_path, O_RDONLY);

    if (fd == -1) {
        perror("Read file: open");

        return NULL;
    }

    char buf[BUFSIZ], *context = NULL;
    size_t len = sizeof(buf);

    if ((content = calloc(len, sizeof(char))) == NULL) {
        perror("Read file: calloc");
        goto end;
    }

    int nb;
    *tlen = 0;

    for (;;) {
        nb = read(fd, buf, sizeof(buf));

        if (nb < 0) {
            perror("Restore tree: read");
            free(content);
            content = NULL;
            goto end;
        } else if (nb == 0) {
            break;
        } else {
            *tlen += b;

            if (*tlen > len) {
                len *= 2;
                char *rcontent = realloc(content, len *sizeof(char));

                if (rcontent == NULL) {
                    perror("Read file: realloc");
                    free(content);
                    content = NULL;
                    goto end;
                } else {
                    content = rcontent;
                }
            }

            strncpy(content + (*tlen - nb), buf, nb);
        }
    }

end:
    close(fd);

    return content;
}

char *
copy_string(char *str, size_t len)
{
    char *cpy = calloc(1, ((len + 1) * sizeof(char)));

    if (cpy == NULL) {
        perror("Copy string: calloc");

        return NULL;
    }

    strncpy(cpy, str, len);
    cpy[len] = '\0';

    return cpy;
}

char *
mktempfifo(const char *template)
{
    int tempfd;
    char *runtime_dir = getenv(RUNTIME_DIR_ENV);

    if (runtime_dir == NULL)
        runtime_dir = "/tmp";

    char *fifo_path = malloc(strlen(runtime_dir) + 1 + strlen(template) + 1);

    if (fifo_path == NULL)
        return NULL;

    sprintf(fifo_path, "%s/%s", runtime_dir, template);

    if ((tempfd = mkstemp(fifo_path)) == -1) {
        free(fifo_path);

        return NULL;
    }

    close(tempfd);
    unlink(fifo_path);

    if (mkfifo(fifo_path, 0666) == -1) {
        free(fifo_path);

        return NULL;
    }

    return fifo_path;
}

int
asprintf(char **buf, const char *fmt, ...)
{
    int size = 0;
    va_list args;

    va_start(args, fmt);
    size = vasprintf(buf, fmt, args);
    va_end(args);

    return size;
}

int
vasprintf(char **buf, const char *fmt, va_list args)
{
    va_list tmp;

    va_copy(tmp, args);
    int size = vsnprintf(NULL, 0, fmt, tmp);
    va_end(tmp);

    if (size < 0)
        return -1;

    *buf = malloc(size + 1);

    if (*buf == NULL)
        return -1;

    size = vsprintf(*buf, fmt, args);

    return size;
}

bool
is_hex_color(const char *color)
{
    if (color[0] != '#' || strlen(color) != 7)
        return false;

    int i;

    for (i = 1; i < 7; i++) {
        if (!isxdigit(color[i]))
            return false;
    }

    return true;
}
