#define _POSIX_C_SOURCE 200112L
#include <ctype.h>
#include <locale.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "test.h"

void sigManager() {
    struct sigaction pipeHandler;
    memset(&pipeHandler, 0, sizeof(pipeHandler));
    pipeHandler.sa_handler = SIG_IGN;
    int notused;
    SYSCALL(notused, sigaction(SIGPIPE, &pipeHandler, NULL), "sigaction");
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s username \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc > 1 && strlen(argv[1]) > MAXNAMELEN) {
        fprintf(stderr, "Username can be max %d char\n", MAXNAMELEN);
        exit(EXIT_FAILURE);
    }

    // sigManager();
    int res = 0;
    char* username = argv[1];

    // TODO timer che tests X volte
    CHECK_EXIT(res, os_connect(username), "Connection error (os_connect)");

    if (argc == 3) {
        long testToRun = strtol(argv[2], NULL, 10);
        fprintf(stdout, "\n Test n.%ld [%s] INIT\n\n", testToRun, username);
        fprintf(stdout, "CONNECTED\n\n");
        if (testToRun == 1) test1();
        if (testToRun == 2) test2();
        if (testToRun == 3) test3();

        // fprintf(stdout, "\nResults:\n-> total: %d\n-> success: %d\n-> failed: %d\n\n", total, success, failed);
        fprintf(stdout, "\n Test n.%ld [%s] DISCONNESSO: \n", testToRun, username);
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
