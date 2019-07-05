#if !defined(ACCESS_H_)
#define ACCESS_H_
#define _POSIX_C_SOURCE 200809L
#include <access.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "connection.h"
#include "utils.h"

int os_connect(char *name);
int os_store(char *name, void *block, size_t len);
void *os_retrieve(char *name);
int os_delete(char *name);
int os_disconnect();

#endif /* LIBOBJSTORE_ACCESS_H_ */