#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

static int demo_hold = 0;

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int partition(int arr[], int low, int high) {
    int pivot = arr[low + (high - low) / 2];
    int i = low;
    int j = high;

    while (1) {
        while (arr[i] < pivot) i++;
        while (arr[j] > pivot) j--;

        if (i >= j) {
            return j;
        }
        swap(&arr[i], &arr[j]);
        i++;
        j--;
    }
}

void quickSort(int arr[], int low, int high) {
    if (low >= high) return;

    int p = partition(arr, low, high);
    quickSort(arr, low, p);
    quickSort(arr, p + 1, high);
}

typedef struct {
    int *arr;
    int low;
    int high;
    int threads;
} SortArgs;

void parallel_quickSort(int arr[], int low, int high, int threads);

void *parallel_worker(void *arg) {
    SortArgs *args = (SortArgs *)arg;
    parallel_quickSort(args->arr, args->low, args->high, args->threads);
    if (demo_hold > 0 && args->threads <= 1)
        sleep((unsigned)demo_hold);
    return NULL;
}

void parallel_quickSort(int arr[], int low, int high, int threads) {
    if (low >= high) return;

    if (threads <= 1) {
        quickSort(arr, low, high);
        return;
    }

    int p = partition(arr, low, high);
    int left_work = (low < p);
    int right_work = (high > p + 1);

    if (!left_work) {
        parallel_quickSort(arr, p + 1, high, threads);
        return;
    }
    if (!right_work) {
        parallel_quickSort(arr, low, p, threads);
        return;
    }

    int left_threads = threads / 2;
    int right_threads = threads - left_threads;

    SortArgs args;
    args.arr = arr;
    args.low = low;
    args.high = p;
    args.threads = left_threads;

    pthread_t thread;
    int rc = pthread_create(&thread, NULL, parallel_worker, &args);
    if (rc == 0) {
        parallel_quickSort(arr, p + 1, high, right_threads);
        pthread_join(thread, NULL);
    }
    else {
        quickSort(arr, low, p);
        quickSort(arr, p + 1, high);
    }
}

int is_sorted(int arr[], int size) {
    for (int i = 1; i < size; i++)
        if (arr[i - 1] > arr[i]) return 0;
    return 1;
}

double get_time_ms(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * 1000.0 + time.tv_nsec / 1e6;
}

void fill_array(int arr[], int size) {
    for (int i = 0; i < size; i++)
        arr[i] = rand() % 10000;
}

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        printf("Usage: %s <array size> <max_threads> [hold_sec]\n", argv[0]);
        return 1;
    }

    int size = atoi(argv[1]);
    int max_threads = atoi(argv[2]);
    if (argc == 4) demo_hold = atoi(argv[3]);
    if (size <= 0 || max_threads <= 0 || demo_hold < 0) {
        printf("Error: arguments must be positive\n");
        return 1;
    }

    long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count < 1) {
        printf("Cannot determine CPU count\n");
        return 1;
    }
    printf("Logical CPU count: %ld\n", cpu_count);
    printf("Array size: %d\n", size);
    printf("Max threads: %d\n", max_threads);

    int *original = malloc(size * sizeof(int));
    int *sequential = malloc(size * sizeof(int));
    int *parallel = malloc(size * sizeof(int));

    if (original == NULL || sequential == NULL || parallel == NULL) {
        printf("Error allocating memory\n");
        free(original);
        free(sequential);
        free(parallel);
        return 1;
    }

    srand(42);
    fill_array(original, size);
    memcpy(sequential, original, size * sizeof(int));
    memcpy(parallel, original, size * sizeof(int));

    double start = get_time_ms();
    quickSort(sequential, 0, size - 1);
    double end = get_time_ms();
    double sequential_time = end - start;
    printf("Sequential time: %.3f ms\n", sequential_time);

    start = get_time_ms();
    parallel_quickSort(parallel, 0, size - 1, max_threads);
    end = get_time_ms();
    double parallel_time = end - start;
    printf("Parallel time: %.3f ms\n", parallel_time);

    printf("Sequential sorted: %s\n", is_sorted(sequential, size) ? "true" : "false");
    printf("Parallel sorted: %s\n", is_sorted(parallel, size) ? "true" : "false");

    double speedup = sequential_time / parallel_time;
    double efficiency = speedup / max_threads;

    printf("Speedup: %.3f\n", speedup);
    printf("Efficiency: %.2f%%\n", efficiency * 100);

    free(original);
    free(sequential);
    free(parallel);
    return 0;
}