#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

int main(int argc, char* argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s username \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc > 1 && strlen(argv[1]) > MAXNAMELEN) {
        fprintf(stderr, "Username can be max %d char\n", MAXNAMELEN);
        exit(EXIT_FAILURE);
    }

    int res = 0;
    char* username = argv[1];
    fprintf(stdout, "\nCLIENT: %s\n\n", username);
    CHECK_EXIT(res, os_connect(username), "Connection error");

    if (argc == 3) {
        fprintf(stdout, "CONNECTED\n\n");
        long testToRun = strtol(argv[2], NULL, 10);
        if (testToRun == 1) test1();
        if (testToRun == 2) test2();
        if (testToRun == 3) test3();

        fprintf(stdout, "\nResults:\n-> total: %d\n-> success: %d\n-> failed: %d\n\n", totalOp, successOp, failedOp);

        CHECK(res, os_disconnect(), "Error LEAVE");
        exit(EXIT_SUCCESS);
    }

    int choice = 0;

    while (1) {
        printf(
            "\nSelect operation:\n\
                1)STORE\n\
                2)RETRIEVE\n\
                3)DELETE\n\
                4)LEAVE\n");
        choice = 0;
        scanf("%d", &choice);
        char fileName[MAXNAMELEN + 1];
        char* data;

        switch (choice) {
            case 1:
                printf("Insert data name:");
                scanf("%s", fileName);
                printf("Insert data:\n");
                scanf("%*c%ms", &data);  // Dynamic alloc
                CHECK(res, os_store(fileName, data, strlen(data)), "Error STORE");
                if (res != 0) free(data);
                break;
            case 2:
                printf("Insert data name:");
                scanf("%s", fileName);
                CHECK(data, os_retrieve(fileName), "Error RETRIEVE");

                if (data != NULL) {
                    fprintf(stderr, "DATA: {%s}", data);
                    free(data);
                }
                break;
            case 3:
                printf("Insert data name:");
                scanf("%s", fileName);
                CHECK(res, os_delete(fileName), "Error DELETE");
                break;
            case 4:
                CHECK_EXIT(res, os_disconnect(), "Error LEAVE");
                exit(EXIT_SUCCESS);
                break;
            default:
                perror("Error\n");
                break;
        }
    }

    return 0;
}
