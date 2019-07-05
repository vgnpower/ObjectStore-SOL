#if !defined(TEST_H)
#define TEST_H
#define _POSIX_C_SOURCE 200112L  // per strtok_r
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "access.h"
#include "utils.h"

#define STARTING_SIZE 100
#define INC_SIZE 5000
#define CONTENT "ciao\n"

#define OP_UPDATE_COUNTER(res, failed, success, total, msgSucc, msgFail) \
    total++;                                                             \
    if (res != 1) {                                                      \
        failed++;                                                        \
        fprintf(stdout, msgSucc);                                        \
    } else {                                                             \
        success++;                                                       \
        fprintf(stdout, msgFail);                                        \
    }

int total = 0;
int success = 0;
int failed = 0;

void test1() {
    /*     char nameOfData[3];
        int res;
        long dataSize = 0;
        for (int i = 0; i < 20; i++) {
            dataSize = (STARTING_SIZE * i * INC_SIZE) + 1;
            if (i == 0) dataSize = STARTING_SIZE + 1;
            if (i == 19) dataSize = 100001;

            char* data = MALLOC(dataSize);

            sprintf(nameOfData, "%d", i);
            int cx = 0;
            while (dataSize - cx > 0) {
                cx += snprintf(data + cx, dataSize - cx, "%s", "hello");
            }
            CHECK(res, os_store(nameOfData, data, strlen(data)), "Error STORE");
            free(data);
            data = NULL;
            OP_UPDATE_COUNTER(res, failed, success, total, "Test1 KO\n", "Test1 OK\n");
        } */
    char nameOfData[3];
    char* data;  // = MALLOC(STARTING_SIZE);
    int res, i = 0;
    long dataSize = 0;

    for (i = 0; i < 20; i++) {
        dataSize = (i == 0) ? STARTING_SIZE : (i + 1) * INC_SIZE;

        char* data = MALLOC(dataSize + 1);
        // data = realloc(data, (sizeof(char) * (dataSize + 1)));
        sprintf(nameOfData, "%d", i);
        int pos = 0;
        while (dataSize - pos > 0) pos += sprintf(data + pos, "%s", CONTENT);

        CHECK(res, os_store(nameOfData, data, strlen(data)), "Error STORE");
        OP_UPDATE_COUNTER(res, failed, success, total, "Test1 KO\n", "Test1 OK\n");
        free(data);
    }
}

void test2() {
    char* nameOfData = "test2";
    char* contentRetrieved;
    int res;
    CHECK(res, os_store(nameOfData, CONTENT, strlen(CONTENT)), "Error STORE");
    CHECK(contentRetrieved, (char*)os_retrieve(nameOfData), "Error Retrieve");
    if (customError != NULL) fprintf(stdout, "CUSTOMERROR (test 2): [%s] \n", customError);
    OP_UPDATE_COUNTER(equal(contentRetrieved, CONTENT), failed, success, total, "Test2 KO\n", "Test2 OK\n");
    free(contentRetrieved);
}

void test3() {
    char* nameOfData = "test3";
    int res;
    CHECK(res, os_store(nameOfData, CONTENT, strlen(CONTENT)), "Error STORE");
    CHECK(res, os_delete(nameOfData), "Error DELETE");
    if (customError != NULL) fprintf(stdout, "CUSTOMERROR Test3: [%s] \n", customError);
    OP_UPDATE_COUNTER(res, failed, success, total, "Test3 KO\n", "Test3 OK\n");
}
#endif