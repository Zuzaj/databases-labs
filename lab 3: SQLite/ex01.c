#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

#define MAX_NAME_LENGTH 20
#define MAX_DESC_LENGTH 90
#define NUM_RECORDS 1000000

struct Rec {
    int id;
    char name[MAX_NAME_LENGTH];
    char desc[MAX_DESC_LENGTH];
};

void generateData(struct Rec *records, int num_records) {
    for (int i = 0; i < num_records; ++i) {
        records[i].id = i;
        sprintf(records[i].name, "Name%d", i);
        sprintf(records[i].desc, "Description%d", i);
    }
}

struct Rec *searchById(struct Rec *records, int num_records, int id) {
    for (int i = 0; i < num_records; ++i) {
        if (records[i].id == id) {
            return &records[i];
        }
    }
    return NULL; 
}

long memoryUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss; // Max resident set size (in kilobytes)
}

int main() {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    struct Rec *records = malloc(NUM_RECORDS * sizeof(struct Rec));
    if (records == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    generateData(records, NUM_RECORDS);
    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken to generate records: %f seconds\n", cpu_time_used);

    // Time to search for a record by id
    start = clock();
    struct Rec *result = searchById(records, NUM_RECORDS, 999999);
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    if (result != NULL) {
        printf("Record found: id=%d, name=%s, desc=%s\n", result->id, result->name, result->desc);
    } else {
        printf("Record not found\n");
    }
    printf("Time taken to search for a record: %f seconds\n", cpu_time_used);

    long memory_usage = memoryUsage();
    printf("Memory usage: %ld KB\n", memory_usage);

    free(records);

    return 0;
}
