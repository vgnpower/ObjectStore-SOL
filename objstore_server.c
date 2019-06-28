#define _POSIX_C_SOURCE 200112L
//#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "connection.h"
#include "utils.h"
#define TMPDIR ".tmp"
#define DATADIR "data"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct client {
    char *username;
    long fd;
    struct client *next;
} t_client;

t_client *connectedClient;

int n_client = 0;
int n_items = 0;
long total_size = 0;

static volatile sig_atomic_t sigInt = 0;
static volatile sig_atomic_t sigUsr1 = 0;

static void sighandler(int sig) {
    switch (sig) {
        case SIGINT: {
            sigInt = 1;
        } break;
        case SIGUSR1: {
            sigUsr1 = 1;
        } break;
    }
}

void sigManager() {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighandler;

    int notused;
    SYSCALL(notused, sigaction(SIGUSR1, &sa, NULL), "sigaction");
    SYSCALL(notused, sigaction(SIGINT, &sa, NULL), "sigaction");
}

void freeList() {
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK
    t_client *curr = connectedClient;
    t_client *prev = NULL;

    if (curr == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    while (curr->next != NULL) {
        prev = curr;
        curr = curr->next;
        FREE_ALL(prev->username, prev);
    }

    FREE_ALL(curr->username, curr);
    pthread_mutex_unlock(&mutex);  // Rilascio della LOCK
}

int Connected(char *name) {
    t_client *curr = connectedClient;

    while (curr != NULL) {
        if (equal(name, curr->username)) return 1;
        curr = curr->next;
    }

    return 0;
}

int sendErrorMessage(t_client *client, char *error) {
    int eMsgLen = strlen("KO") + strlen(error) + 3;
    char *message = createRequest(eMsgLen, "%s %s \n", "KO", error);
    int result;
    SYSCALL(result, write(client->fd, message, eMsgLen * sizeof(char)), "errore invio");
    free(message);

    return (result != -1) ? 1 : 0;
}

int sendSucessMessage(t_client *client) {
    int result;
    SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "errore invio");
    return (result != -1) ? 1 : 0;
}

t_client *initClient(long fd) {
    t_client *client = (t_client *)malloc(sizeof(t_client));
    IFNULL_EXIT(client, "Error on malloc t_client");
    client->next = NULL;
    client->username = NULL;
    client->fd = fd;

    return client;
}

t_client *addClient(t_client *client, char *username) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    fprintf(stderr, "[%d-%d-%d %d:%d:%d] %s has been connected!\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
            tm.tm_min, tm.tm_sec, username);
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK

    if (connectedClient == NULL) {
        connectedClient = client;
        connectedClient->username = MALLOC(strlen(username) + 1);
        strcpy(connectedClient->username, username);
        n_client++;
        pthread_mutex_unlock(&mutex);
        return connectedClient;
    }

    if (Connected(username)) {
        fprintf(stderr, "already connected");
        pthread_mutex_unlock(&mutex);
        // TODO exit handler
        return client;
    }

    t_client *curr = connectedClient;

    while (curr->next != NULL) curr = curr->next;

    client->username = MALLOC(strlen(username) + 1);
    strcpy(client->username, username);
    curr->next = client;

    n_client++;
    pthread_mutex_unlock(&mutex);  // Rilascio della LOCK

    return client;
}

void removeClient(t_client *client) {
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK

    t_client *curr = connectedClient;
    t_client *prev = NULL;

    if (client == NULL || curr == NULL /*|| n_client == 0*/) {
        // FREE_ALL(client->username, client);
        pthread_mutex_unlock(&mutex);
        return;
    }

    while (curr->next != NULL && client != curr) {
        prev = curr;
        curr = curr->next;
    }

    if (prev == NULL) {  // se è il primo della lista
        connectedClient = curr->next;
    } else {
        prev->next = curr->next;
    }

    n_client--;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    fprintf(stderr, "[%d-%d-%d %d:%d:%d] %s has been disconnected!\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
            tm.tm_min, tm.tm_sec, curr->username);
    FREE_ALL(curr->username, curr);
    pthread_mutex_unlock(&mutex);
}

t_client *reqRegister(char *buf, t_client *client, char *savePtr) {
    char *user = strtok_r(NULL, " ", &savePtr);
    if (strlen(user) > 254) {
        sendErrorMessage(client, "Username too long. It should be less than 255");
        return NULL;
    }

    client = addClient(client, user);

    if (client->username == NULL) {
        sendErrorMessage(client, "Username already taken");
        return NULL;
    }

    char *dirPath = getDirPath(client->username, DATADIR);

    if (mkdir(dirPath, 0777) == -1 && errno != EEXIST) {
        sendErrorMessage(client, "Path name too big");
        free(dirPath);
        return NULL;
    }

    free(dirPath);
    sendSucessMessage(client);

    return client;
}

t_client *reqStore(char *buf, t_client *client, char *savePtr) {
    char *fileName = strtok_r(NULL, " ", &savePtr);
    char *fileLen = strtok_r(NULL, " \n", &savePtr);
    strtok_r(NULL, " ", &savePtr);
    char *tmpFileToWrite = getFilePath(fileName, "", TMPDIR);
    char *fileToWrite = getFilePath(fileName, client->username, DATADIR);
    char *fileData = savePtr;
    long lengthHeader = strlen("STORE") + strlen(fileName) + strlen(fileLen) + 5;
    long lengthFirstRead = strnlen(fileData, BUFFER_SIZE - lengthHeader);
    long fileLength = strtol(fileLen, NULL, 10);

    FILE *fp;
    CHECK_EQ(fp = fopen(tmpFileToWrite, "w"), NULL, EOPEN);
    if (fp == NULL) {
        sendErrorMessage(client, EOPEN);
        FREE_ALL(tmpFileToWrite, fileToWrite);
        return client;
    }

    long packetsLeft = (long)ceil((double)(fileLength - lengthFirstRead) / BUFFER_SIZE);
    fwrite(fileData, sizeof(char), lengthFirstRead, fp);

    while (packetsLeft > 0) {
        memset(buf, '\0', BUFFER_SIZE);
        SYSCALL_BREAK(read(client->fd, buf, BUFFER_SIZE), "error on read");
        fwrite(buf, sizeof(char), (packetsLeft > 1) ? BUFFER_SIZE : ((fileLength - lengthFirstRead) % BUFFER_SIZE), fp);
        packetsLeft--;
    }
    fclose(fp);

    int result = -1;
    if (packetsLeft <= 0) SYSCALL(result, rename(tmpFileToWrite, fileToWrite), "error on renaming");

    if (result == 0)
        sendSucessMessage(client);
    else
        sendErrorMessage(client, "Error on write or reading store file");

    FREE_ALL(tmpFileToWrite, fileToWrite);
    return client;
}

t_client *reqRetrive(char *buf, t_client *client, char *savePtr) {
    char *fileName = strtok_r(NULL, " ", &savePtr);
    char *fileToRetrive = getFilePath(fileName, client->username, DATADIR);
    char *data = getFileData(fileToRetrive);

    if (data == NULL) {
        sendErrorMessage(client, EOPEN);
        return client;
    }

    long dataLength = strlen(data);
    int nCharDtLngth = log10(dataLength) + 1;          // Numero di char che servono per scrivere lenData
    char *dataLenAsString = MALLOC(nCharDtLngth + 1);  // stringa per contenere lenData
    snprintf(dataLenAsString, nCharDtLngth + 1, "%ld", dataLength);

    long responseLength = strlen("DATA") + strlen(dataLenAsString) + strlen(data) + 4 + 1;
    char *response = MALLOC(responseLength);
    snprintf(response, responseLength, "DATA %s \n %s", dataLenAsString, data);
    int result = 0;
    SYSCALL(result, write(client->fd, response, responseLength * sizeof(char)), "error on send");
    if (result == -1) sendErrorMessage(client, "error on send");
    FREE_ALL(dataLenAsString, data, response, fileToRetrive);

    return client;
}

t_client *reqDelete(char *buf, t_client *client, char *savePtr) {
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

t_client *manageRequest(char *buf, t_client *client) {
    char *savePtr;
    // fprintf(stderr, "buffer[%s]\n", buf);
    char *comand = strtok_r(buf, " ", &savePtr);
    int result;
    if (client->username == NULL && equal(comand, "REGISTER")) return reqRegister(buf, client, savePtr);

    if (equal(comand, "STORE")) return reqStore(buf, client, savePtr);
    if (equal(comand, "RETRIEVE")) return reqRetrive(buf, client, savePtr);
    if (equal(comand, "DELETE")) return reqDelete(buf, client, savePtr);
    if (equal(comand, "LEAVE")) {
        SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "errore invio");
        removeClient(client);
        return NULL;
    }

    sendErrorMessage(client, "Error undefined. Maybe wrong request? Try again.");

    return NULL;
}

void *threadClient(void *arg) {
    long connfd = (long)arg;
    t_client *client = initClient(connfd);
    char *buffer = MALLOC(BUFFER_SIZE);

    do {
        memset(buffer, '\0', BUFFER_SIZE);
        SYSCALL_BREAK(read(connfd, buffer, BUFFER_SIZE), "errore lettura thread F");
        client = manageRequest(buffer, client);
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

    TH_ERR_CLOSE(pthread_attr_init(&thattr), connfd, "pthread_attr_init FALLITA\n");
    TH_CLOSE_ATTR(pthread_attr_setdetachstate(&thattr, PTHREAD_CREATE_DETACHED), connfd, &thattr,
                  "pthread_attr_setdetachstate FALLITA\n");
    TH_CLOSE_ATTR(pthread_create(&thid, &thattr, threadClient, (void *)connfd), connfd, &thattr, "pthread_create FALLITA\n");
}

int main(int argc, char *argv[]) {
    unlink(SOCKNAME);

    if (mkdir(DATADIR, 0777) == -1 && errno != EEXIST) exit(1);
    if (mkdir(TMPDIR, 0777) == -1 && errno != EEXIST) exit(1);

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

    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1 && errno == EINTR) {
        }

        if (sigInt == 1) break;

        if (sigUsr1 == 0)
            spawnThread(connfd);
        else {
            clearObjectStruct();
            countObjects("data");
            fprintf(stdout,
                    "\n/------------Stats------------\\\nConnected cients: %d\nTotal objects: %d\nTotal size of objects: %ld "
                    "MB\n\\-----------------------------/\n",
                    n_client, objStore.n_items, ((objStore.total_size / 1024) / 1024));
            sigUsr1 = 0;
        }
    }

    freeList();
    unlink(SOCKNAME);

    return 0;
}
