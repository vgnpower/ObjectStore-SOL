#if !defined(CONNECTION_H)
#define CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>

#define SOCKNAME "./objstore.sock"
#define MAXBACKLOG 32
#define BUFFER_SIZE 2048

#endif
