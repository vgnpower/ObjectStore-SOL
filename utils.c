#include "utils.h"

int equal(char *s1, char *s2) {
    if (s1 == NULL || s2 == NULL) return 0;
    return strcmp(s1, s2) != 0 ? 0 : 1;
}

int equalN(char *s1, char *s2) {
    if (s1 == NULL || s2 == NULL) return 0;

    return (strncmp(s1, s2, strlen(s2)) == 0) ? 1 : 0;
}

char *getDirPath(char *username) {
    int lenPath = sizeof(char) * (strlen("data/") + strlen(username) + 1);
    char *path = (char *)malloc(lenPath);
    snprintf(path, lenPath, "data/%s", username);
    return path;
}

char *getFilePath(char *fileName, char *username) {
    char *dir = getDirPath(username);
    int lenPath = sizeof(char) * (strlen(dir) + strlen(fileName) + 2);
    char *path = (char *)malloc(lenPath);

    snprintf(path, lenPath, "%s/%s", dir, fileName);

    free(dir);
    return path;
}

long getFileSize(char *file) {
    if (file == NULL) return -1;
    struct stat st;
    stat(file, &st);
    return st.st_size;
}

char *getFileData(char *fPath) {
    FILE *fpr;
    CHECK_EQ(fpr = fopen(fPath, "r"), NULL, EOPEN);

    if (fpr == NULL) {
        free(fPath);
        return NULL;
    }

    long file_size = getFileSize(fPath);
    char *data = MALLOC(file_size + 1);

    if (fread(data, sizeof(char), file_size, fpr) != file_size) {
        fclose(fpr);
        FREE_ALL(data, fPath);
        return NULL;
    }

    fclose(fpr);
    data[file_size] = '\0';
    return data;
}

int isDot(const char dir[]) {
    int l = strlen(dir);

    if ((l > 0 && dir[l - 1] == '.')) return 1;
    return 0;
}

void countObjects(char *dirName) {
    DIR *dir;
    IFNULL_RETURN((dir = opendir(dirName)), "Error on opening dir");

    struct dirent *file;

    while ((file = readdir(dir)) != NULL) {
        struct stat statbuf;
        char filename[512];
        strncpy(filename, dirName, strlen(dirName) + 1);
        strncat(filename, "/", 2);
        strncat(filename, file->d_name, strlen(file->d_name) + 1);

        if (isDot(filename)) continue;  // TODO a che serve?

        int result;
        SYSCALL(result, stat(filename, &statbuf), "unable to open stat");
        if (result == -1) return;

        // recursive print if file = directory
        if (S_ISDIR(statbuf.st_mode))
            countObjects(filename);
        else {
            objStore.n_items++;
            objStore.total_size += statbuf.st_size;
        }
    }

    closedir(dir);
}

void clearObjectStruct() {
    if (objStore.n_items > 0) {
        objStore.n_items = 0;
        objStore.total_size = 0;
    }
}

char *createRequest(long messageLength, char *format, ...) {
    char *formatAlloc = MALLOC(strlen(format) + 1);
    strcpy(formatAlloc, format);

    va_list argptr;

    char *save;
    char *token = strtok_r(formatAlloc, "%s", &save);

    int nOfParam = (token != NULL) ? 1 : 0;
    char *current;
    while ((current = strtok_r(NULL, "%s", &save)) != NULL) nOfParam++;

    free(formatAlloc);

    if (nOfParam < 2) return NULL;

    char *message = MALLOC(messageLength);

    va_start(argptr, format);

    if (vsnprintf(message, messageLength, format, argptr) == -1) {
        va_end(argptr);
        return NULL;
    }

    va_end(argptr);

    return message;
}