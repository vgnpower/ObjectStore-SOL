#include "access.h"

static struct sockaddr_un serv_addr;
static int sockfd;

char buffer[BUFFER_SIZE];

/*
Create a connection between server with SOCKNAME
Sets globally connfd and serv_addr
 */
int os_connect(char* username) {
    SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    int notused;
    SYSCALL_RETURN(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect", close(sockfd));
    // Create req
    int lenMex = sizeof(char) * (strlen(username) + strlen("REGISTER") + 3);
    char* message = createRequest(lenMex, "%s %s \n", "REGISTER", username);

    // Send request
    SYSCALL(notused, write(sockfd, message, lenMex), "write");
    free(message);

    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    if (equalN(buffer, "OK")) return 1;
    close(sockfd);

    return 0;
}

int os_store(char* fileName, void* block, size_t dtLength) {
    long dataLength = (long)dtLength;
    int nCharDtLngth = log10(dataLength) + 1;
    char* dataLenAsString = MALLOC((nCharDtLngth + 1));
    sprintf(dataLenAsString, "%ld", dataLength);
    long messageLength = sizeof(char) * (strlen("STORE") + dataLength + strlen(fileName) + strlen(dataLenAsString) + 6);
    char* message = createRequest(messageLength, "%s %s %s \n %s", "STORE", fileName, dataLenAsString, (char*)block);
    free(dataLenAsString);

    // Send Req
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength), "write");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    return (equalN((char*)buffer, "OK")) ? 1 : 0;
}

void* os_retrieve(char* fileName) {
    // Creazione header
    long messageLength = sizeof(char) * (strlen("RETRIEVE") + strlen(fileName) + 3);
    char* message = createRequest(messageLength, "%s %s \n", "RETRIEVE", fileName);

    // Send request
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength * sizeof(char)), "write");
    free(message);

    // Waiting response
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    char* savePtr;
    char* command = strtok_r(buffer, " ", &savePtr);

    // Error handling
    if (equal(command, "KO")) {
        char* errMsg = strtok_r(NULL, "\n", &savePtr);
        fprintf(stdout, "%s\n", errMsg);
        return NULL;
    }

    // Data to return
    if (equal(command, "DATA")) {
        char* dataLength = strtok_r(NULL, " ", &savePtr);
        char* fileData = strtok_r(NULL, " \n", &savePtr);
        long lengthFirstRead = strlen(fileData);
        long fileLength = strtol(dataLength, NULL, 10);
        long packetsLeft = (long)ceil((double)(fileLength - lengthFirstRead) / BUFFER_SIZE);
        char* data = MALLOC((fileLength + 1));  // Alloco il dato da ritornare
        int pointerLastWrite = snprintf(data, fileLength + 1, "%s", fileData);

        while (packetsLeft > 0) {
            memset(buffer, '\0', BUFFER_SIZE);  // Azzero il buffer

            int result;
            SYSCALL(result, read(sockfd, buffer, BUFFER_SIZE), "errore lettura");
            pointerLastWrite += snprintf(data + pointerLastWrite, fileLength - pointerLastWrite, "%s", fileData);
            packetsLeft--;
        }

        return data;
    }

    return NULL;
}

int os_delete(char* fileName) {
    long messageLength = sizeof(char) * (strlen("DELETE") + strlen(fileName) + 3);
    char* message = createRequest(messageLength, "%s %s \n", "DELETE", fileName);

    // send request
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength * sizeof(char)), "write");
    free(message);

    // Aspetto la risposta
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    char* savePtr;
    char* command = strtok_r(buffer, " ", &savePtr);
    // Gestisco risposta di errore
    if (equal(command, "OK")) return 1;

    if (equal(command, "KO")) {
        char* errMsg = strtok_r(NULL, "\n", &savePtr);
        fprintf(stdout, "%s\n", errMsg);
        return 0;
    }

    return 0;
}

int os_disconnect() {
    long messageLength = sizeof(char) * (strlen("LEAVE") + 3);
    char* message = createRequest(messageLength, "%s \n", "LEAVE");

    // invio request
    int notused;
    SYSCALL(notused, write(sockfd, message, strlen(message) * sizeof(char)), "write");
    free(message);

    // Aspetto la risposta
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");
    if (equalN(buffer, "OK")) {
        close(sockfd);
        return 1;
    }
    close(sockfd);  // Chiudo il socket
    return 0;
}
