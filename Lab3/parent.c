#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"

int main(void) {
    char *line = NULL;
    size_t cap = 0;
    ssize_t len = 0;

    printf("Введите имя файла: \n");
    if (getline(&line, &cap, stdin) == -1) {
        fprintf(stderr, "Имя файла не введено\n");
        free(line);
        return EXIT_FAILURE;
    }
    line[strcspn(line, "\n")] = '\0';
    if (line[0] == '\0') {
        fprintf(stderr, "Имя файла пустое\n");
        free(line);
        return EXIT_FAILURE;
    }
    char *file_name = strdup(line);
    if (file_name == NULL) {
        perror("strdup");
        free(line);
        return EXIT_FAILURE;
    }

    char shm_name[64], sd_name[64], sf_name[64];
    snprintf(shm_name, sizeof(shm_name), "/lab3_shm_%d", getpid());
    snprintf(sd_name, sizeof(sd_name), "/lab3_sd_%d", getpid());
    snprintf(sf_name, sizeof(sf_name), "/lab3_sf_%d", getpid());

    int fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    if (ftruncate(fd, sizeof(shared_t)) == -1) {
        perror("ftruncate");
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    shared_t *sh = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) {
        perror("mmap");
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }
    close(fd);
    sh->done = 0;

    sem_t *sem_data = sem_open(sd_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem_data == SEM_FAILED) {
        perror("sem_open data");
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }
    sem_t *sem_free = sem_open(sf_name, O_CREAT | O_EXCL, 0600, 1);
    if (sem_free == SEM_FAILED) {
        perror("sem_open free");
        sem_unlink(sd_name);
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        sem_unlink(sd_name);
        sem_unlink(sf_name);
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }
    if (pid == 0) {
        execl("./child", "child", file_name, shm_name, sd_name, sf_name, (char*)NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    while ((len = getline(&line, &cap, stdin)) != -1) {
        if (len >= BUF_SIZE) {
            fprintf(stderr, "Строка слишком длинная\n");
            continue;
        }
        sem_wait(sem_free);
        strcpy(sh->data, line);
        sem_post(sem_data);
    }

    sem_wait(sem_free);
    sh->done = 1;
    sem_post(sem_data);

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
    }
    else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        fprintf(stderr, "Child завершился с ошибкой, код %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        fprintf(stderr, "Child убит сигналом %d\n", WTERMSIG(status));


    munmap(sh, sizeof(shared_t));
    sem_close(sem_data);
    sem_close(sem_free);
    shm_unlink(shm_name);
    sem_unlink(sd_name);
    sem_unlink(sf_name);
    return EXIT_SUCCESS;
}