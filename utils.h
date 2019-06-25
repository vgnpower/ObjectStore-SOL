#if !defined(UTILS_H)
#define UTILS_H
#define _POSIX_C_SOURCE 200112L
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#define EOPEN "Errore apertura file"

#define MAXNAMELEN 32

#define CHECK_EQ(X, val, str) \
    if ((X) == val) {         \
        perror(#str);         \
        myErrno = str;        \
    }

#define SYSCALL(r, c, e)                \
    if ((r = c) == -1) {                \
        perror(e);                      \
    }

#define SYSCALL_QUIT(r, c, e)          \
    if ((r = c) == -1) {               \
        perror(e);                     \
        exit(EXIT_FAILURE);            \
    }

#define SYSCALL_BREAK(c, e)          \
    if ((c) < 1) {               \
        perror(e);                     \
        break;                          \
    }

#define CHECK(r, c, e)              \
    if ((r = c) == 0) {              \
        fprintf(stderr, "%s\n", e); \
    }

#define CHECK_EXIT(r, c, e)         \
    if ((r = c) == 0) {             \
        fprintf(stderr, "%s\n", e); \
        exit(EXIT_FAILURE);         \
    }

#define TH_IFNULL_QUIT(X, err)\
        if((X) == NULL){      \
            pthread_exit("Thread closed");\
        }

 #define TH_ERR_CLOSE(r, fd, err)\
        if((r) != 0){      \
            perror(err);\
            close(fd);  \
            return;     \
        } 

#define TH_CLOSE_ATTR(r, fd, attr, err)\
    if((r) != 0){      \
        perror(err);\
        pthread_attr_destroy(attr);\
        close(fd);  \
        return;     \
    }             

#define IFNULL_EXIT(X, err)     \
        if((X) == NULL){        \
            perror(err);        \
            exit(EXIT_FAILURE); \
        }  

#define IFNULL_RETURN(x, err)   \
    if((x) == NULL){            \
        perror(err);            \
        return;                 \
    }

#define ALLOC(t,n) (t *) malloc((n)*sizeof(t))

#define FREE_ALL(...)                                   \
    void *pta[] = {__VA_ARGS__};                        \
    for(int i = 0; i < sizeof(pta) / sizeof(void*); i++)\
    {                                                   \
        free(pta[i]);                                   \
    }

//https://stackoverflow.com/questions/20991229/how-to-freeing-pointers-using-macro-in-c

static inline char* MALLOC(long size) {
    char* result = (char *)malloc(sizeof(char) * size);
    if(result == NULL){
        perror("error on malloc");
        exit(EXIT_FAILURE);
    }
    return result;
}

static char *myErrno;

typedef struct objectStore {
    int n_items;
    long total_size;
} t_objectStore;

t_objectStore objStore;

int equal(char *str, char *cmpstr);
int equalN(char *str, char *cmpstr);
char *getDirPath(char *username);
char *getFilePath(char *fileName, char *username);
char *getFileData(char *fPath);
int isDot(const char dir[]);
long getFileSize(char *file);
void countObjects(char *dirName);
void clearObjectStruct();
char *createRequest(long messageLength, char *format, ...);
#endif /* UTIL_H */
