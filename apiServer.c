#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>        /* ind AF_UNIX */
#include <errno.h>
#include <assert.h>

#include "apiServer.h"
#include "utility.h"
#define N 100
#define UNIX_PATH_MAX 108
#define CHECK_FD    \
  if (fd_socket < 0)   \
  {                 \
    errno = EBADFD; \
    return -1;      \
  }

#define CHECK_FD_SK(S)                                  \
  CHECK_FD;                                             \
  if (!S || strncmp(socketname, S, UNIX_PATH_MAX) != 0) \
  {                                                     \
    errno = EINVAL;                                     \
    return -1;                                          \
  }

#define AINO(code, message, action) \
  if (code == -1)                   \
  {                                 \
    perror(message);                \
    action                          \
  }




int fd_socket = -1;


//Api per interagire con il file server


int openConnection(const char *sockname, int msec, const struct timespec abstime)
{

	//creo il socket e gestisco gli eventuali errori
	fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd_socket == -1 && errno == EINTR)
	{
		perror("SERVER-> un segnale ha interrotto la funzione socket\n");
		return -1;
	}
	else if (fd_socket == -1)
	{
		perror("SERVER-> la funzione socket ha risocntrato un errore\n");
		return -1;
	}

	//setto la strttura sockaddr_un neccessaria per effettuare la connect
	struct sockaddr_un sa;
	strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	//setto la struttura per poter riprovare la connessione in caso di fallimento,
	//per un tempo max di msec millisecondi
	struct timespec request;
	struct timespec remaining;
	request.tv_sec=msec / 1000;
	request.tv_nsec=(msec % 1000) * 1000000L;

	fd_socket=socket(AF_UNIX, SOCK_STREAM, 0);
	while (connect(fd_socket,(struct sockaddr*)&sa, sizeof(sa)) == -1 )
	{
		if (errno == ENOENT)
		{
			time_t current = time(NULL);

			if (current >= abstime.tv_sec)
			{
				//è finito il tempo
				errno = ETIME;
				return -1;
			}

			if(nanosleep(&request,&remaining)<0)
			{
				printf("errore nanosleep\n");
			}
		}
		else
		{
			return -1;
		}
	}
	return 0;
}


int closeConnection(const char* sockname)
{
	int closeReturnValue = close(fd_socket);
	if ((closeReturnValue == -1) && (errno == EBADF))
	{
		perror("CLIENT-> Errore: file descritor aperto non valido\n");
	}

	if ((closeReturnValue == -1) && (errno == EINTR))
	{
		perror("CLIENT-> Errore: close interrotta da un segnale\n");
	}
	if ((closeReturnValue == -1) && (errno == EIO))
	{
		perror("CLIENT-> Errore input/output");
	}
	if ((closeReturnValue == -1) && (errno == ENOSPC) && (errno ==EDQUOT))
	{
		perror("CLIENT-> Errore: scrittura in spazio di archiviazione non disponibile!\n");
	}
	fd_socket = -1;
	printf("CLIENT-> La connessione è stata chiusa correttamente\n");
	return closeReturnValue;
}

int openFile(const char* pathname, int flags)
{
	write(fd_socket,pathname,28);
	return 1;
}
