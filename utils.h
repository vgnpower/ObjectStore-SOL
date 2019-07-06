#if !defined(UTILS_H)
#define UTILS_H
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define EOPEN "Error open file"
#define MAXNAMELEN 32

#define CHECK_EQ(X, val, str) \
    if ((X) == val) {         \
        perror(#str);         \
    }

#define CHECK_NOTEQ(X, val, str) \
    if ((X) != val) {            \
        perror(str);             \
        str = NULL;              \
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

/**
 * @brief Wrapper for malloc function, handles the possible errors
 *
 * @param size
 * @return char*
 */
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

/**
 * @brief variable that will store various internal error
 *
 */
char *customError;

/**
 * @brief Compare 2 strings
 *
 * @param str
 * @param cmpstr
 * @return int 1 or 0. 1 if equal else 0
 */
int equal(char *str, char *cmpstr);

/**
 * @brief Compare only the charater of cmpstr with the matching of str
 *
 * @param str
 * @param cmpstr
 * @return int
 */
int equalN(char *str, char *cmpstr);

/**
 * @brief Get the Dir Path object
 *
 * @param username
 * @param baseDir
 * @return char*
 */
char *getDirPath(char *username, char *baseDir);

/**
 * @brief Get the File Path object
 *
 * @param fileName
 * @param username
 * @param baseDir
 * @return char*
 */
char *getFilePath(char *fileName, char *username, char *baseDir);

/**
 * @brief Get the File Data of specified path
 *
 * @param fPath
 * @return char*
 */
char *getFileData(char *fPath);

/**
 * @brief check if the last char is a dot
 *
 * @param dir
 * @return int
 */
int isDot(const char dir[]);

/**
 * @brief Get the File Size of specified file
 *
 * @param file
 * @return long
 */
long getFileSize(char *file);

/**
 * @brief Count all objects stored in the server data folder
 *
 * @param dirName
 */
void countObjects(char *dirName);

/**
 * @brief clear internal struct for counting purpose
 *
 */
void clearObjectStruct();

/**
 * @brief Create the request layout for the server
 *
 * @param messageLength
 * @param format
 * @param ...
 * @return char*
 */
char *createRequest(long messageLength, char *format, ...);

/**
 * @brief print current date and 1 other argument
 *
 * @param username
 * @param message
 */
void printDateAndMore(char *username, char *message);

#endif
