#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
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

void createTable(sqlite3 *db, struct Rec *records, int num_records) {
    char *errmsg;
    char sql[200];

    sprintf(sql, "CREATE TABLE IF NOT EXISTS Records (id INTEGER PRIMARY KEY, name TEXT, desc TEXT);");
    sqlite3_exec(db, sql, NULL, 0, &errmsg);

    for (int i = 0; i < num_records; ++i) {
        sprintf(sql, "INSERT INTO Records (id, name, desc) VALUES (%d, 'Name%d', 'Description%d');", records[i].id, records[i].id, records[i].id);
        sqlite3_exec(db, sql, NULL, 0, &errmsg);
    }
}


struct Rec *searchById(sqlite3 *db, int id) {
    sqlite3_stmt *stmt;
    char sql[100];
    struct Rec *result = malloc(sizeof(struct Rec));

    sprintf(sql, "SELECT * FROM Records WHERE id = %d;", id);
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result->id = sqlite3_column_int(stmt, 0);
        strcpy(result->name, (char *)sqlite3_column_text(stmt, 1));
        strcpy(result->desc, (char *)sqlite3_column_text(stmt, 2));
    } else {
        free(result);
        result = NULL; // Record not found
    }

    sqlite3_finalize(stmt);
    return result;
}


sqlite3 *openDatabase(const char *file) {
    sqlite3 *db;
    int rc = sqlite3_open(file, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    return db;
}

void closeDatabase(sqlite3 *db) {
    sqlite3_close(db);
}

long getMemoryUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss; 
}

int main() {
    clock_t start, end;
    double cpu_time_used;

    const char *database = "data.db";

    struct Rec *records = malloc(NUM_RECORDS * sizeof(struct Rec));
    if (records == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

 
    generateData(records, NUM_RECORDS);

    // Parameters combinations
    const char *storage_methods[] = {"MEMORY", "file"};
    const char *journal_modes[] = {"DELETE", "WAL"};
    int use_transactions[] = {0, 1};
    int use_indexes[] = {0, 1};

    for (int i = 0; i < 2; ++i) { // Storage
        for (int j = 0; j < 2; ++j) { // Journal
            for (int k = 0; k < 2; ++k) { // Transactions
                for (int l = 0; l < 2; ++l) { // Indexes
  
                    sqlite3 *db = openDatabase(database);
                    if (i == 0) {
                        printf("Data storage method: Memory\n");
                    } else {
                        printf("Data storage method: File\n");
                    }

                    char *errmsg;
                    char journal_mode_sql[50];
                    sprintf(journal_mode_sql, "PRAGMA journal_mode=%s;", journal_modes[j]);
                    sqlite3_exec(db, journal_mode_sql, NULL, 0, &errmsg);
                    printf("Journaling mode: %s\n", journal_modes[j]);
                    if (use_transactions[k]) {
                        sqlite3_exec(db, "BEGIN;", NULL, 0, &errmsg);
                        printf("Transaction: BEGIN\n");
                    }

                    start = clock();
                    createTable(db, records, NUM_RECORDS);
                    end = clock();
                    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                    printf("Time taken to generate records: %f seconds\n", cpu_time_used);

                    if (use_indexes[l]) {
                        sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_id ON Records(id);", NULL, 0, &errmsg);
                        printf("Use of index: Yes\n");
                    } else {
                        printf("Use of index: No\n");
                    }

                    if (use_transactions[k]) {
                        sqlite3_exec(db, "COMMIT;", NULL, 0, &errmsg);
                        printf("Transaction: COMMIT\n");
                    }

                    start = clock();
                    struct Rec *result = searchById(db, 999999);
                    end = clock();
                    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                    if (result != NULL) {
                        printf("Record found: id=%d, name=%s, desc=%s\n", result->id, result->name, result->desc);
                        free(result);
                    } else {
                        printf("Record not found\n");
                    }
                    printf("Time taken to search for a record: %f seconds\n", cpu_time_used);

                    long memory_usage = getMemoryUsage();
                    printf("Memory usage: %ld KB\n", memory_usage);

                    closeDatabase(db);
                    printf("\n");
                }
            }
        }
    }

    free(records);

    return 0;
}

