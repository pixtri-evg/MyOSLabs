#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include "common.h"

static int sum_line(const char *line, float *result) {
    float sum = 0;
    const char *p = line;

    while (1) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\0') break;

        char *end;
        const float x = strtof(p, &end);
        if (end == p) return -1;
        sum += x;
        p = end;
    }
    *result = sum;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Использование: ./child <shm> <sem_data> <sem_free>\n");
        return EXIT_FAILURE;
    }

    int out = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    int fd = shm_open(argv[2], O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    shared_t *sh = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
    close(fd);

    sem_t *sem_data = sem_open(argv[3], 0);
    if (sem_data == SEM_FAILED) {
        perror("sem_open data");
        return EXIT_FAILURE;
    }
    sem_t *sem_free = sem_open(argv[4], 0);
    if (sem_free == SEM_FAILED) {
        perror("sem_open free");
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;
    while (1) {
        sem_wait(sem_data);
        if (sh->done) break;

        float sum;
        if (sum_line(sh->data, &sum) == -1)
            fprintf(stderr, "Некорректная строка: %s\n", sh->data);

        else if (dprintf(out, "%g\n", sum) < 0) {
            perror("write");
            ret = EXIT_FAILURE;
        }
        sem_post(sem_free);
    }

    munmap(sh, sizeof(shared_t));
    sem_close(sem_data);
    sem_close(sem_free);
    if (close(out) == -1) {
        perror("close");
        ret = EXIT_FAILURE;
    }
    return ret;
}