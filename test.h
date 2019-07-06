#if !defined(TEST_H)
#define TEST_H
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib_client.h"
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
    char nameOfData[5];
    int res;

    for (int i = 0; i < 20; i++) {
        long dataSize = (i == 0) ? STARTING_SIZE : (i + 1) * INC_SIZE;

        char* data = MALLOC(dataSize + 1);
        sprintf(nameOfData, "%d", i);
        int size = 0;
        while (dataSize - size > 0) size += sprintf(data + size, "%s", CONTENT);

        CHECK(res, os_store(nameOfData, data, strlen(data)), "Error STORE");
        CHECK_NOTEQ(customError, NULL, customError);
        OP_UPDATE_COUNTER(res, failed, success, total, "Test1 KO\n", "Test1 OK\n");
        free(data);
    }
}

void test2() {
    char* nameOfData = "t2";
    char* contentRetrieved;
    int res;
    CHECK(res, os_store(nameOfData, CONTENT, strlen(CONTENT)), "Error STORE");
    CHECK(contentRetrieved, (char*)os_retrieve(nameOfData), "Error Retrieve");
    CHECK_NOTEQ(customError, NULL, customError);
    OP_UPDATE_COUNTER(equal(contentRetrieved, CONTENT), failed, success, total, "Test2 KO\n", "Test2 OK\n");
    free(contentRetrieved);
}

void test3() {
    char* nameOfData = "t3";
    int res;
    CHECK(res, os_store(nameOfData, CONTENT, strlen(CONTENT)), "Error STORE");
    CHECK(res, os_delete(nameOfData), "Error DELETE");
    CHECK_NOTEQ(customError, NULL, customError);
    OP_UPDATE_COUNTER(res, failed, success, total, "Test3 KO\n", "Test3 OK\n");
}
#endif