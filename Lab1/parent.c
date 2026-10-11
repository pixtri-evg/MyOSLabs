#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    char* line = NULL;
    size_t cap = 0;
    ssize_t len;

    printf("Введите имя файла: ");
    fflush(stdout);

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

    char *filename = strdup(line);
    if (filename == NULL) {
        perror("strdup");
        free(line);
        return EXIT_FAILURE;
    }

    int pipe1[2];
    if (pipe(pipe1) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        if (close(pipe1[1]) == -1) {
            perror("close");
            _exit(EXIT_FAILURE);
        }

        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        if (close(pipe1[0]) == -1) {
            perror("close");
            _exit(EXIT_FAILURE);
        }
        execl("./child", "child", filename, (char*)NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    if (close(pipe1[0]) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    while ((len = getline(&line, &cap, stdin)) != -1)
        if (write(pipe1[1], line, len) == -1) {
            perror("write");
            return EXIT_FAILURE;
        }

    if (close(pipe1[1]) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status) != 0 && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Child завершился с ошибкой %d\n", WEXITSTATUS(status));
    }

    printf("Имя файла было: %s\n", filename);

    free(filename);
    free(line);
    return EXIT_SUCCESS;
    
}