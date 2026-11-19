#ifndef COMMON_H
#define COMMON_H

#define BUF_SIZE 4096

typedef struct {
    int done;
    char data[BUF_SIZE];
} shared_t;

#endif
