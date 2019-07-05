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

int main(int argc, char* argv[]) {
    if (argc == 1 || (argc > 1 && strlen(argv[1]) > MAXNAMELEN)) exit(EXIT_FAILURE);

    int res = 0;
    char* username = argv[1];
    int connAttempLeft = 5;

    CHECK(res, os_connect(username), "Connection error (os_connect)");

    while (res == 0 && connAttempLeft > 0) {
        connAttempLeft--;
        sleep(0.2);
        CHECK(res, os_connect(username), "Connection error (os_connect)");
    }

    if (argc == 3) {
        long testToRun = strtol(argv[2], NULL, 10);  // Convert number from char to long

        if (res == 0) {
            fprintf(stdout, "Test n.%ld [%s] FAILED \n\n", testToRun, username);
            return 0;
        }

        fprintf(stdout, "Test n.%ld [%s] INIT\n", testToRun, username);
        fprintf(stdout, "CONNECTED\n");

        if (testToRun == 1) test1();
        if (testToRun == 2) test2();
        if (testToRun == 3) test3();

        CHECK(res, os_disconnect(), "Error LEAVE");

        if (res == 0)
            fprintf(stdout, "Error LEAVE: %s\n", username);
        else
            fprintf(stdout, "Test n.%ld, [%s] DISCONNECTED: \n", testToRun, username);

        fprintf(stdout, "\n\n");
        exit(EXIT_SUCCESS);
    }

    return 0;
}
