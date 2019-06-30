#if !defined(TEST_H)
#define TEST_H
#define _POSIX_C_SOURCE 200112L  // per strtok_r
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "access.h"
#include "utils.h"

#define STARTING_SIZE 100  // start offset
#define INC_SIZE 1000      // modify this to reach 100000 bytes

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
    char nameOfData[3];
    char* content = "H llo";
    char* data = MALLOC(STARTING_SIZE);
    int res;

    for (int i = 0; i < 20; i++) {
        long dataSize = STARTING_SIZE + i * INC_SIZE;
        data = (char*)realloc(data, (sizeof(char) * dataSize));
        sprintf(nameOfData, "%d", i);

        while (dataSize - strlen(data) > 0) sprintf(data + strlen(data), "%s", content);

        CHECK(res, os_store(nameOfData, data, strlen(data)), "Error STORE");
        OP_UPDATE_COUNTER(res, failed, success, total, "Test1 KO\n", "Test1 OK\n");
    }

    free(data);
}

void test2() {
    char* nameOfData = "test2";
    char* contentToStore = "H llo";
    char* contentRetrieved;
    int res;
    CHECK(res, os_store(nameOfData, contentToStore, strlen(contentToStore)), "Error STORE");
    CHECK(contentRetrieved, (char*)os_retrieve(nameOfData), "Error Retrieve");

    OP_UPDATE_COUNTER(equal(contentRetrieved, contentToStore), failed, success, total, "Test2 KO\n", "Test2 OK\n");
    free(contentRetrieved);
}

void test3() {
    char* nameOfData = "test3";
    char* contentToStore = "H llo";
    int res;
    CHECK(res, os_store(nameOfData, contentToStore, strlen(contentToStore)), "Error STORE");
    CHECK(res, os_delete(nameOfData), "Error DELETE");

    OP_UPDATE_COUNTER(res, failed, success, total, "Test3 KO\n", "Test3 OK\n");
}
#endif