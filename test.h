#if !defined(TEST_H)
#define TEST_H
#define _POSIX_C_SOURCE 200112L  // per strtok_r
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "access.h"
#include "utils.h"

#define STARTING_SIZE 100
#define INC_SIZE 55
#define CONTENT "HelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloend!\n"

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
    char* data = (char*)malloc(sizeof(char) * STARTING_SIZE);
    int res, i = 0;
    long dataSize = 100;
    for (int i = 0; i < 20; i++) {
        if (i == 0) {
            dataSize = 101;
        } else if (i == 19) {
            dataSize = 100001;
        } else {
            dataSize = (i * INC_SIZE * STARTING_SIZE) + 1;
        }
        long current_size = ((i + 1) / 2 * 10000) + 1;
        if (i == 0) current_size = STARTING_SIZE + 1;
        data = (char*)realloc(data, (sizeof(char) * current_size));

        sprintf(nameOfData, "%d", i);
        int cx = 0;
        while (current_size - cx > 0) {
            cx += snprintf(data + cx, current_size - cx, "%s", CONTENT);
        }
        CHECK(res, os_store(nameOfData, data, strlen(data)), "Error STORE");

        OP_UPDATE_COUNTER(res, failed, success, total, "Test1 KO\n", "Test1 OK\n");
    }

    free(data);
}

void test2() {
    char* nameOfData = "test2";
    char* contentRetrieved;
    int res;
    CHECK(res, os_store(nameOfData, CONTENT, strlen(CONTENT)), "Error STORE");
    if (customError != NULL) fprintf(stdout, "\n CUSTOMERROR (Test 2 after store): [%s] \n", customError);
    CHECK(contentRetrieved, (char*)os_retrieve(nameOfData), "Error Retrieve");
    if (customError != NULL) fprintf(stdout, "\n CUSTOMERROR (test 2 after retrive): [%s] \n", customError);
    OP_UPDATE_COUNTER(equal(contentRetrieved, CONTENT), failed, success, total, "Test2 KO\n", "Test2 OK\n");
    free(contentRetrieved);
}

void test3() {
    char* nameOfData = "test3";
    int res;
    CHECK(res, os_store(nameOfData, CONTENT, strlen(CONTENT)), "Error STORE");
    if (customError != NULL) fprintf(stdout, "\n CUSTOMERROR Test3 after store: [%s] \n", customError);
    CHECK(res, os_delete(nameOfData), "Error DELETE");
    if (customError != NULL) fprintf(stdout, "\n CUSTOMERROR Test3 after delete: [%s] \n", customError);
    OP_UPDATE_COUNTER(res, failed, success, total, "Test3 KO\n", "Test3 OK\n");
}
#endif