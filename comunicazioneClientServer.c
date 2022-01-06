/*
 * comunicazioneClientServer.c
 */
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



ssize_t writeNBytes(int fdDaElaborare, void *v_ptr, size_t N)
{
	char *pointer = v_ptr;
	int valoreDiRitorno;
	size_t nleft;
	ssize_t bytesScritti;

	nleft = N;
	while (nleft > 0)
	{
		if ((bytesScritti = write(fdDaElaborare, pointer, nleft)) < 0)
		{
			if (nleft == N)
			{
				//è stato riscontrato un errore, ritorno -1
				return -1;
			}
			else
			{
				//Errore, ritorno quello che sono riuscito a leggere fino ad ora
				break;

			}
		}
		else if (bytesScritti == 0)
		{
			break;
		}
		nleft -= bytesScritti;
		pointer += bytesScritti;
	}
	valoreDiRitorno = N - nleft; //valoreDiRitorno >= 0
	return valoreDiRitorno;
}

//Funzione read con un numero di bytes massimo uguale ad n
ssize_t readNBytes(int fdDaElaborare, void *v_ptr, size_t N)
{
	char *pointer = v_ptr;
	size_t nleft;
	ssize_t bytesLetti;
	int valoreDiRitorno;

	nleft = N;
	while (nleft > 0)
	{
		if ((bytesLetti = read(fdDaElaborare, pointer, nleft)) < 0)
		{
			if (nleft == N)
			{
				//è stato riscontrato un errore, ritorno -1
				return -1;
			}
			else
			{
				//Errore, ritorno quello che sono riuscito a leggere fino ad ora
				break;
			}
		}
		else if (bytesLetti == 0)
		{
			//EOF
			break;
		}
		nleft -= bytesLetti;
		pointer += bytesLetti;
	}
	valoreDiRitorno = N - nleft; //valoreDiRitorno >= 0
  	return valoreDiRitorno;
}

int riceviDati(int fd, void *dest, size_t *sizePtr)
{
  size_t size = 0;

  // control flow flags
//  int sizeRead = 0;
//  int error = 0;
//  int done = 0;

  // read the size
  int a=readNBytes(fd, &size, sizeof(size));
  if(a<0)
  {
	  printf("boh\n");
	  return -1;
  }
  *sizePtr=size;
  //printf("sizeletta:%ld\n",size);
  //if (sizeRead)
  {
    // default behaviour: write to dest
    void *writeTo = dest;

   // if (alloc)

      // in this situation dest is considered as the address
      // of a pointer that we have to set to the read data
    	char **destPtr = dest;

      // malloc enough space
      *destPtr = malloc(sizeof(**destPtr) * size);

      // we have to write into the allocated space
      writeTo = *destPtr;



    // read the data if writeTo is not NULL
    //if (writeTo)

    	int b=readNBytes(fd, writeTo, size);
    	if(b<0)
    	{
    		printf("boh2\n");
    		return -1;
    	}

    	return b;

//    else
//    {
//      error = 1;
//    }
  }

//  if (done)
//  {
//    // return the size as well
//    sizePtr ? *sizePtr = size : 0;
//    return 0;
//  }

//  if (error)
//  {
//    perror("getData has failed");
//    return -1;
//  }
//  return -1;
}


int sendData(int fd, const void *data, size_t size)
{
	int a=writeNBytes(fd, (void *)data, size);
	return a;
}
