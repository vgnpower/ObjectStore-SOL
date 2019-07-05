#include "access.h"

static struct sockaddr_un serv_addr;
static int sockfd;

char buffer[BUFFER_SIZE];

int os_connect(char* username) {
    SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    int notused;
    SYSCALL_RETURN(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect", close(sockfd));
    // Create req
    int lenMex = sizeof(char) * (strlen(username) + strlen("REGISTER") + 4);
    char* message = createRequest(lenMex, "%s %s \n", "REGISTER", username);

    // Send request
    SYSCALL(notused, write(sockfd, message, lenMex), "write (os_connect)");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read (os_connect)");

    if (equalN(buffer, "OK")) return 1;
    if (equalN(buffer, "KO")) {
        char* savePtr;
        strtok_r(buffer, " ", &savePtr);
        char* errMsg = strtok_r(NULL, "\n", &savePtr);
        customError = errMsg;
        close(sockfd);
        return 0;
    }
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
    SYSCALL(notused, write(sockfd, message, messageLength), "write (store)");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read (os_store)");

    if (equalN(buffer, "KO")) {
        char* savePtr;
        strtok_r(buffer, " ", &savePtr);
        char* errMsg = strtok_r(NULL, "\n", &savePtr);
        customError = errMsg;
        fprintf(stderr, "%s\n", customError);
        return 0;
    }

    return (equalN(buffer, "OK")) ? 1 : 0;
}

void* os_retrieve(char* fileName) {
    long messageLength = sizeof(char) * (strlen("RETRIEVE") + strlen(fileName) + 4);
    char* message = createRequest(messageLength, "%s %s \n", "RETRIEVE", fileName);

    // Send request
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength * sizeof(char)), "write (os_retrive)");
    free(message);

    // Waiting response
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read (os_retrive)");
    char* savePtr;
    char* command = strtok_r(buffer, " ", &savePtr);

    // Error handling
    if (equal(command, "KO")) {
        char* errMsg = strtok_r(NULL, "\n", &savePtr);
        customError = errMsg;
        return NULL;
    }

    // Data to return
    if (equal(command, "DATA")) {
        char* dataLength = strtok_r(NULL, " ", &savePtr);
        savePtr += 2;  // skip te next 2 char (the \n and space)
        char* fileData = savePtr;
        long lengthFirstRead = strlen(fileData);
        long fileLength = strtol(dataLength, NULL, 10);
        long packetsLeft = (long)ceil((double)(fileLength - lengthFirstRead) / BUFFER_SIZE);
        char* data = MALLOC((fileLength + 1));
        int pointerLastWrite = snprintf(data, fileLength + 1, "%s", fileData);

        while (packetsLeft > 0) {
            memset(buffer, '\0', BUFFER_SIZE);

            int result;
            SYSCALL(result, read(sockfd, buffer, BUFFER_SIZE), "error on reading (os_retrive");
            if (notused == -1) {
                free(data);
                return NULL;
            }
            // TODO add controllo result
            pointerLastWrite += snprintf(data + pointerLastWrite, fileLength - pointerLastWrite, "%s", fileData);
            packetsLeft--;
        }

        return data;
    }

    return NULL;
}

int os_delete(char* fileName) {
    long messageLength = sizeof(char) * (strlen("DELETE") + strlen(fileName) + 4);
    char* message = createRequest(messageLength, "%s %s \n", "DELETE", fileName);

    // send request
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength * sizeof(char)), "write");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    char* savePtr;
    char* command = strtok_r(buffer, " ", &savePtr);

    if (equal(command, "OK")) return 1;
    if (equal(command, "KO")) {
        char* errMsg = strtok_r(NULL, "\n", &savePtr);
        customError = errMsg;
    }

    return 0;
}

int os_disconnect() {
    long messageLength = sizeof(char) * (strlen("LEAVE") + 3);
    char* message = createRequest(messageLength, "%s \n", "LEAVE");

    // send request
    int notused;
    SYSCALL(notused, write(sockfd, message, strlen(message) * sizeof(char)), "write");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    if (equalN(buffer, "OK")) {
        close(sockfd);
        return 1;
    }
    if (equal(buffer, "KO")) {
        char* savePtr;
        strtok_r(buffer, " ", &savePtr);
        char* errMsg = strtok_r(NULL, "\n", &savePtr);
        customError = errMsg;
    }

    close(sockfd);
    return 0;
}
