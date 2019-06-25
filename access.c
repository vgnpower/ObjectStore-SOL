#include "access.h"

static struct sockaddr_un serv_addr;
static int sockfd;

char buffer[BUFFER_SIZE];

/*
Create a connection between server with SOCKNAME
Sets globally connfd and serv_addr
 */
int os_connect(char* username) {
    // Connessione socket
    SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    int notused;
    SYSCALL(notused, connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect");
    if (notused != 0) return 0;

    // Creazione Header
    int lenMex = sizeof(char) * (strlen(username) + strlen("REGISTER") + 3);  // 3 is for space and \n
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
    char* type = "STORE";

    // Creazione stringa che contiene la lunghezza del dato(size_t -> string)
    long dataLength = (long)dtLength;
    int nCharDtLngth = log10(dataLength) + 1;            // Numero di char che servono per scrivere dataLength
    char* dataLenAsString = MALLOC((nCharDtLngth + 1));  // Stringa per contenere dataLength
    sprintf(dataLenAsString, "%ld", dataLength);

    // Creazione header
    long messageLength = sizeof(char) * (strlen(type) + dataLength + strlen(fileName) + strlen(dataLenAsString) + 5 +
                                         1);  // lunghezza messaggio (4 spazi + \n + terminazione)
    char* message = MALLOC(messageLength);    // messaggio da inviare

    snprintf(message, messageLength, "%s %s %s \n %s", type, fileName, dataLenAsString,
             (char*)block);  // creo la stringa  da inviare
    free(dataLenAsString);

    // Invio request
    int notused;
    SYSCALL(notused, write(sockfd, message, messageLength), "write");
    free(message);
    SYSCALL(notused, read(sockfd, buffer, BUFFER_SIZE * sizeof(char)), "read");

    if (equalN((char*)buffer, "OK")) return 1;

    return 0;
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
        // Prendo le informazioni dall'Header
        char* dataLength = strtok_r(NULL, " ", &savePtr);  // Stringa con la lunghezza del dato
        char* fileData = strtok_r(NULL, " \n", &savePtr);  // Prima parte del dato letta presente nel buffer
        long lengthFirstRead = strlen(fileData);           // Lunghezza della prima parte letta

        long fileLength = strtol(dataLength, NULL, 10);  // Trasformo la stringa della lunghezza in intero

        long nReadLeft = (long)ceil((double)(fileLength - lengthFirstRead) /
                                    BUFFER_SIZE);  // Calcolo quante altre read dovrò fare per leggere l'intero dato

        char* data = MALLOC((fileLength + 1));  // Alloco il dato da ritornare

        // fileLength+1 o scoppia tutto
        int pointerLastWrite = snprintf(data, fileLength + 1, "%s",
                                        fileData);  // uso pointerLastWrite per sapere il punto in cui sono arrivato a scrivere

        // Leggo la parte restante
        while (nReadLeft > 0) {
            memset(buffer, '\0', BUFFER_SIZE);  // Azzero il buffer

            int result;
            SYSCALL(result, read(sockfd, buffer, BUFFER_SIZE), "errore lettura");
            pointerLastWrite += snprintf(data + pointerLastWrite, fileLength - pointerLastWrite, "%s",
                                         fileData);  // Metto in append su data usando pointerLastWrite

            nReadLeft--;
        }

        return data;
    }

    return NULL;
}

int os_delete(char* fileName) {
    // Creo l'header
    long messageLength = sizeof(char) * (strlen("DELETE") + strlen(fileName) + 3);
    char* message = createRequest(messageLength, "%s %s \n", "DELETE", fileName);

    // invio request
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
    char* type = "LEAVE";
    long messageLength = sizeof(char) * (strlen(type) + 3);
    char* message = MALLOC(messageLength);
    snprintf(message, messageLength, "%s \n", type);

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
