#if !defined(UTILS_H)
#define UTILS_H
#define _POSIX_C_SOURCE 200112L
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define EOPEN "Errore apertura file"
#define MAXNAMELEN 32

#define CHECK_EQ(X, val, str) \
    if ((X) == val) {         \
        perror(#str);         \
    }

#define SYSCALL(r, c, e) \
    if ((r = c) == -1) { \
        perror(e);       \
    }

#define SYSCALL_QUIT(r, c, e) \
    if ((r = c) == -1) {      \
        perror(e);            \
        exit(EXIT_FAILURE);   \
    }

#define SYSCALL_BREAK(c, e) \
    if ((c) < 1) {          \
        perror(e);          \
        break;              \
    }
#define SYSCALL_RETURN(c, e, secondOp) \
    if ((c) != 0) {                    \
        perror(e);                     \
        secondOp;                      \
        return 0;                      \
    }

#define CHECK(r, c, e)              \
    if ((r = c) == 0) {             \
        fprintf(stderr, "%s\n", e); \
    }

#define CHECK_NULL(r, c, e)         \
    if ((r = c) == 0) {             \
        fprintf(stderr, "%s\n", e); \
    }

#define CHECK_EXIT(r, c, e)         \
    if ((r = c) == 0) {             \
        fprintf(stderr, "%s\n", e); \
        exit(EXIT_FAILURE);         \
    }

#define TH_IFNULL_QUIT(X, err)         \
    if ((X) == NULL) {                 \
        pthread_exit("Thread closed"); \
    }

#define TH_ERR_CLOSE(r, fd, err) \
    if ((r) != 0) {              \
        perror(err);             \
        close(fd);               \
        return;                  \
    }

#define TH_CLOSE_ATTR(r, fd, attr, err) \
    if ((r) != 0) {                     \
        perror(err);                    \
        pthread_attr_destroy(attr);     \
        close(fd);                      \
        return;                         \
    }

#define IFNULL_EXIT(X, err) \
    if ((X) == NULL) {      \
        perror(err);        \
        exit(EXIT_FAILURE); \
    }

#define IFNULL_RETURN(x, err) \
    if ((x) == NULL) {        \
        perror(err);          \
        return;               \
    }

#define FREE_ALL(...)                                        \
    void *pta[] = {__VA_ARGS__};                             \
    for (int i = 0; i < sizeof(pta) / sizeof(void *); i++) { \
        free(pta[i]);                                        \
    }

// https://stackoverflow.com/questions/20991229/how-to-freeing-pointers-using-macro-in-c

static inline char *MALLOC(long size) {
    char *result = (char *)malloc(sizeof(char) * size);
    if (result == NULL) {
        perror("error on malloc");
        exit(EXIT_FAILURE);
    }
    return result;
}

typedef struct objectStore {
    int n_items;
    long total_size;
} t_objectStore;

t_objectStore objStore;
char *customError;
int equal(char *str, char *cmpstr);
int equalN(char *str, char *cmpstr);
char *getDirPath(char *username, char *baseDir);
char *getFilePath(char *fileName, char *username, char *baseDir);
char *getFileData(char *fPath);
int isDot(const char dir[]);
long getFileSize(char *file);
void countObjects(char *dirName);
void clearObjectStruct();
char *createRequest(long messageLength, char *format, ...);
void printDateAndMore(char *username, char *message);

static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    while (left > 0) {
        if ((r = read((int)fd, bufptr, left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;  // gestione chiusura socket
        left -= r;
        bufptr += r;
    }
    return size;
}

static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    while (left > 0) {
        if ((r = write((int)fd, bufptr, left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;
        left -= r;
        bufptr += r;
    }
    return 1;
}

#endif /* UTIL_H */
