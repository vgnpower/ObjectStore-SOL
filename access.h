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

/**
 * @brief Allow the connection throw the socket defined in connection.h
 *
 * @param name username of the client
 * @return int
 */
int os_connect(char *name);

/**
 * @brief Send to server a block of that will be saved to disk
 *
 * @param name (name of file)
 * @param block (content of file to save)
 * @param len (length of data)
 * @return int
 */
int os_store(char *name, void *block, size_t len);

/**
 * @brief Retrive from server (if exists) the content of specified filename
 *
 * @param name
 * @return void*
 */
void *os_retrieve(char *name);

/**
 * @brief Delete from server the specified filename
 *
 * @param name
 * @return int
 */
int os_delete(char *name);

/**
 * @brief Disconnect from the server socket
 *
 * @return int
 */
int os_disconnect();

#endif /* LIBOBJSTORE_ACCESS_H_ */