#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <ftw.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include "connection.h"
#include "icl_hash.h"
#include "utils.h"

#define TMPDIR "/tmp/objStoreTmpFiles"
#define DATADIR "data"
#define NBUCKETS 512

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct client {
    char *username;
    long fd;
} client_t;

icl_hash_t *userTables;

int n_client = 0;
int n_items = 0;
long total_size = 0;

static volatile sig_atomic_t sigInt = 0;

static void stopServer(int sig) {
    sigInt = 1;
    fprintf(stderr, "\n/!\\ THE SERVER IS SHUTTING DOWN /!\\\n");
}

void printStats() {
    clearObjectStruct();
    countObjects("data");
    fprintf(stdout,
            "\n/------------Stats------------\\\n Connected cients: %d\n Total objects: %d\n Total size of objects: %ld "
            "MB\n\\-----------------------------/\n",
            n_client, objStore.n_items, ((objStore.total_size / 1024) / 1024));
}

void sigManager() {
    struct sigaction exitHandler;
    struct sigaction statsHandler;
    struct sigaction pipeHandler;

    // Reset struct content
    memset(&exitHandler, 0, sizeof(exitHandler));
    memset(&statsHandler, 0, sizeof(statsHandler));
    memset(&pipeHandler, 0, sizeof(pipeHandler));

    exitHandler.sa_handler = stopServer;
    pipeHandler.sa_handler = SIG_IGN;
    statsHandler.sa_handler = printStats;

    int notused;
    SYSCALL(notused, sigaction(SIGUSR1, &statsHandler, NULL), "sigaction");
    SYSCALL(notused, sigaction(SIGINT, &exitHandler, NULL), "sigaction");
    SYSCALL(notused, sigaction(SIGPIPE, &pipeHandler, NULL), "sigaction");
}

client_t *initClient(long fd) {
    client_t *client = (client_t *)malloc(sizeof(client_t));
    IFNULL_EXIT(client, "Error on malloc client_t");
    client->username = NULL;
    client->fd = fd;

    return client;
}

client_t *addClient(client_t *client, char *username) {
    printDateAndMore(username, "connected");
    pthread_mutex_lock(&mutex);

    if (icl_hash_find(userTables, username) != NULL) {
        pthread_mutex_unlock(&mutex);
        fprintf(stderr, "'%s': already connected!\n", username);
        return client;
    }

    client->username = MALLOC(strlen(username) + 1);
    strcpy(client->username, username);
    icl_entry_t *ins_ret = icl_hash_insert(userTables, client->username, client);

    if (ins_ret != NULL) {
        n_client++;
    } else {
        FREE_ALL(client->username);
        client->username = NULL;
    }

    pthread_mutex_unlock(&mutex);

    return client;
}

void clean(void *item) {
    client_t *tmp = (client_t *)item;
    free(tmp->username);
    free(tmp);
}

void removeClient(client_t *client) {
    pthread_mutex_lock(&mutex);

    if (client == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    if (client->username == NULL) {
        free(client);
        pthread_mutex_unlock(&mutex);
        return;
    }
    printDateAndMore(client->username, "disconnected");

    int success = icl_hash_delete(userTables, client->username, free, free);
    if (success == 0) n_client--;

    pthread_mutex_unlock(&mutex);
}

void freeHT() {
    pthread_mutex_lock(&mutex);

    icl_hash_destroy(userTables, NULL, clean);

    pthread_mutex_unlock(&mutex);
}

int sendErrorMessage(client_t *client, char *error) {
    int eMsgLen = strlen("KO") + strlen(error) + 3;
    char *message = createRequest(eMsgLen, "%s %s \n", "KO", error);
    int result;
    SYSCALL(result, write(client->fd, message, eMsgLen * sizeof(char)), "error sending response message");
    free(message);

    return (result != -1) ? 1 : 0;
}

int sendSucessMessage(client_t *client) {
    int result;
    SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "error sending response message");
    return (result != -1) ? 1 : 0;
}

client_t *reqRegister(char *buf, client_t *client, char *savePtr) {
    char *user = strtok_r(NULL, " ", &savePtr);
    if (strlen(user) > 254) {
        sendErrorMessage(client, "Username too long. It should be less than 255");
        return NULL;
    }

    client = addClient(client, user);

    if (client->username == NULL) {
        if (client->fd >= 0) sendErrorMessage(client, "Username already taken");
        return NULL;
    }

    char *dirPath = getDirPath(client->username, DATADIR);

    if (mkdir(dirPath, 0777) == -1 && errno != EEXIST) {
        sendErrorMessage(client, "Path name too big");
        free(dirPath);
        return client;
    }

    free(dirPath);
    sendSucessMessage(client);

    return client;
}

client_t *reqStore(char *buf, client_t *client, char *savePtr) {
    char *fileName = strtok_r(NULL, " ", &savePtr);
    char *fileLen = strtok_r(NULL, " ", &savePtr);
    savePtr += 2;  // skip te next 2 char (the \n and space)
    char *fileData = savePtr;
    char *tmpFileToWrite = getFilePath(client->username, "", TMPDIR);
    char *fileToWrite = getFilePath(fileName, client->username, DATADIR);
    long fileLength = strtol(fileLen, NULL, 10);
    long lengthHeader = strlen("STORE") + strlen(fileName) + strlen(fileLen) + 5;
    long lengthFirstRead = strnlen(fileData, BUFFER_SIZE - lengthHeader);

    FILE *fp;

    // Open tmp file to write data
    if ((fp = fopen(tmpFileToWrite, "w")) == NULL) {
        fclose(fp);
        sendErrorMessage(client, "error on fopen");
        FREE_ALL(tmpFileToWrite, fileToWrite);
        return client;
    }

    int result = 0;
    fwrite(fileData, sizeof(char), lengthFirstRead, fp);
    while (fileLength - lengthFirstRead > 0) {
        memset(buf, '\0', BUFFER_SIZE);
        SYSCALL(result, read(client->fd, buf, BUFFER_SIZE), "error on read");
        long byteRead = (long)fwrite(buf, sizeof(char), strnlen(buf, BUFFER_SIZE), fp);
        fileLength -= byteRead;
    }
    fclose(fp);

    if (result != -1) {
        SYSCALL(result, rename(tmpFileToWrite, fileToWrite), "error on renaming");
        sendSucessMessage(client);
    } else {
        sendErrorMessage(client, "Error on write or reading store file");
    }

    FREE_ALL(tmpFileToWrite, fileToWrite);

    return client;
}

client_t *reqRetrive(char *buf, client_t *client, char *savePtr) {
    // parse header
    char *fileName = strtok_r(NULL, " ", &savePtr);
    char *fileToRetrive = getFilePath(fileName, client->username, DATADIR);
    char *data = getFileData(fileToRetrive);

    if (data == NULL) {
        sendErrorMessage(client, EOPEN);
        return client;
    }

    // handle the init & creation of dataLength part
    long dataLength = strlen(data);
    int nCharDtLngth = log10(dataLength) + 1;
    char *dataLenAsString = MALLOC(nCharDtLngth + 1);
    snprintf(dataLenAsString, nCharDtLngth + 1, "%ld", dataLength);

    long responseLength = strlen("DATA") + strlen(dataLenAsString) + strlen(data) + 5;
    char *response = MALLOC(responseLength);
    snprintf(response, responseLength, "DATA %s \n %s", dataLenAsString, data);

    int result = 0;
    SYSCALL(result, write(client->fd, response, responseLength * sizeof(char)), "error sending response message");
    if (result == -1) sendErrorMessage(client, "error sending error response message");
    FREE_ALL(dataLenAsString, data, response, fileToRetrive);

    return client;
}

client_t *reqDelete(char *buf, client_t *client, char *savePtr) {
    char *fileName = strtok_r(NULL, " ", &savePtr);
    char *fileToDelete = getFilePath(fileName, client->username, DATADIR);

    // delete file
    if (remove(fileToDelete) == 0)
        sendSucessMessage(client);
    else
        sendErrorMessage(client, EOPEN);

    free(fileToDelete);
    return client;
}

client_t *manageRequest(char *buf, client_t *client) {
    char *savePtr;
    char *comand = strtok_r(buf, " ", &savePtr);

    if (client->username == NULL && equal(comand, "REGISTER")) return reqRegister(buf, client, savePtr);
    if (equal(comand, "STORE")) return reqStore(buf, client, savePtr);
    if (equal(comand, "RETRIEVE")) return reqRetrive(buf, client, savePtr);
    if (equal(comand, "DELETE")) return reqDelete(buf, client, savePtr);
    if (equal(comand, "LEAVE")) {
        sendSucessMessage(client);
        removeClient(client);
        return NULL;
    }

    sendErrorMessage(client, "Error undefined. Maybe wrong request? Try again.");

    return NULL;
}

void *threadClient(void *arg) {
    long connfd = (long)arg;
    client_t *client = initClient(connfd);
    char *buffer = MALLOC(BUFFER_SIZE);

    do {
        memset(buffer, '\0', BUFFER_SIZE);
        if (sigInt == 1) break;
        SYSCALL_BREAK(read(client->fd, buffer, BUFFER_SIZE), "Error reading (thClient)");
        client = manageRequest(buffer, client);
        if (client != NULL && client->fd < 1) break;
    } while (client != NULL);

    free(buffer);
    removeClient(client);
    close(connfd);
    pthread_exit("Thread closed");

    return NULL;
}

void spawnThread(long connfd) {
    pthread_attr_t thattr;
    pthread_t thid;

    TH_ERR_CLOSE(pthread_attr_init(&thattr), connfd, "pthread_attr_init FAILED\n");
    TH_CLOSE_ATTR(pthread_attr_setdetachstate(&thattr, PTHREAD_CREATE_DETACHED), connfd, &thattr,
                  "pthread_attr_setdetachstate FAILED\n");
    TH_CLOSE_ATTR(pthread_create(&thid, &thattr, threadClient, (void *)connfd), connfd, &thattr, "pthread_create FAILED\n");
}

int main(int argc, char *argv[]) {
    unlink(SOCKNAME);

    if (mkdir(DATADIR, 0777) == -1 && errno != EEXIST) exit(1);
    if (mkdir(TMPDIR, 0777) == -1 && errno != EEXIST) exit(1);

    userTables = NULL;
    userTables = icl_hash_create(NBUCKETS, NULL, NULL);
    if (!userTables) {
        fprintf(stderr, "Error creating HashTable\n");
        return 0;
    }

    sigManager();

    int listenfd;
    SYSCALL_QUIT(listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    int notused;
    SYSCALL_QUIT(notused, bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)), "bind");
    SYSCALL_QUIT(notused, listen(listenfd, MAXBACKLOG), "listen");
    int connfd = -1;

    while (sigInt != 1) {
        if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1 && errno == EINTR) {
        }
        spawnThread(connfd);
    }
    freeHT();
    unlink(SOCKNAME);

    return 0;
}
