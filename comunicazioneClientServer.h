/*
 *  Created on: 27 dic 2021
*/
#ifndef COMUNICAZIONECLIENTSERVER_H_
#define COMUNICAZIONECLIENTSERVER_H_

#include "comunicazioneClientServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>


ssize_t writen(int fdDaElaborare, void *v_ptr, size_t N);
ssize_t readn(int fdDaElaborare, void *v_ptr, size_t N);
int riceviDati(int fdDaElaborare, void *dest, size_t *sizePtr);
int inviaDati(int fdDaElaborare, const void *data, size_t size);


#endif /* COMUNICAZIONECLIENTSERVER_H_ */
