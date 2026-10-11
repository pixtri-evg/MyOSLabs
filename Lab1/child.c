#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

static int sum_line(const char *line, float *result) {
    float sum = 0;
    const char *p = line;

    while (1) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\0') break;

        char *end;
        float x = strtof(p, &end);
        if (end == p) return -1;
        sum += x;
        p = end;
    }
    *result = sum;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: ./child <имя файла>\n");
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t cap = 0;

    while (getline(&line, &cap, stdin) != -1) {
        float sum;
        if (sum_line(line, &sum) == -1) {
            fprintf(stderr, "Некорректная строка: %s", line);
            continue;
        }

        if (dprintf(fd, "%g\n", sum) < 0) {
            perror("write");
            return EXIT_FAILURE;
        }
    }
    free(line);

    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}