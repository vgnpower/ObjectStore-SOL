#include "lib_client.h"

static struct sockaddr_un serv_addr;
static int sockfd;

char buffer[BUFFER_SIZE];

int os_connect(char* name) {
    // init the socket
    SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    int notused;  // connect to the socket
    SYSCALL_RETURN(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect", close(sockfd));
    // Create req
    int lenMex = sizeof(char) * (strlen(name) + strlen("REGISTER") + 4);
    char* message = createRequest(lenMex, "%s %s \n", "REGISTER", name);

    // Send request
    SYSCALL(notused, write(sockfd, message, lenMex), "write (os_connect)");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read (os_connect)");

    // read the response
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

int os_store(char* name, void* block, size_t len) {
    long dataLength = (long)len;
    int nCharDtLngth = log10(dataLength) + 1;  // used to prepare the string
    char* dataLenAsString = MALLOC((nCharDtLngth + 1));
    sprintf(dataLenAsString, "%ld", dataLength);  // creating the string with the size of block
    long messageLength = sizeof(char) * (strlen("STORE") + dataLength + strlen(name) + strlen(dataLenAsString) + 6);
    char* message = createRequest(messageLength, "%s %s %s \n %s", "STORE", name, dataLenAsString, (char*)block);
    free(dataLenAsString);

    // Send Req
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength), "write (store)");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read (os_store)");

    // read the response
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

void* os_retrieve(char* name) {
    long messageLength = sizeof(char) * (strlen("RETRIEVE") + strlen(name) + 4);
    char* message = createRequest(messageLength, "%s %s \n", "RETRIEVE", name);

    // Send request
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength * sizeof(char)), "write (os_retrive)");
    free(message);

    // Wait response
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read (os_retrive)");
    char* savePtr;
    char* command = strtok_r(buffer, " ", &savePtr);

    // read the error response
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
        char* data = MALLOC((fileLength + 1));

        int pointerLastWrite = snprintf(data, fileLength + 1, "%s", fileData);
        // loop until read all bytes
        while (fileLength - lengthFirstRead > 0) {
            memset(buffer, '\0', BUFFER_SIZE);
            SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE), "error on reading (os_retrive");
            if (notused == -1) {
                free(data);
                return NULL;
            }
            pointerLastWrite += snprintf(data + pointerLastWrite, fileLength - pointerLastWrite, "%s", fileData);
            lengthFirstRead = pointerLastWrite;  // used to exict the loop if all bytes are read
        }

        return data;
    }

    return NULL;
}

int os_delete(char* name) {
    long messageLength = sizeof(char) * (strlen("DELETE") + strlen(name) + 4);  // 3 space + 1 termination char
    char* message = createRequest(messageLength, "%s %s \n", "DELETE", name);

    // send request
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength * sizeof(char)), "write");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    // read response
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
    long messageLength = sizeof(char) * (strlen("LEAVE") + 3);  // 1 space 1 \n and 1 termination
    char* message = createRequest(messageLength, "%s \n", "LEAVE");

    // send request
    int notused;
    SYSCALL(notused, write(sockfd, message, strlen(message) * sizeof(char)), "write");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    // read response
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
