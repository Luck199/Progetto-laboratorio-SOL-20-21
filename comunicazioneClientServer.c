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






//Funzione write con un numero di bytes massimo uguale ad N
ssize_t writen(int fdDaElaborare, void *v_ptr, size_t N)
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
	valoreDiRitorno = N - nleft; //valoreDiRitorno  è >= 0
	return valoreDiRitorno;


}
//Funzione read con un numero di bytes massimo uguale ad N
ssize_t readn(int fdDaElaborare, void *v_ptr, size_t n)
{
	char *ptr = v_ptr;
	size_t nleft;
	ssize_t nread;
//	int valoreDiRitorno;

	nleft = n;
	while (nleft > 0)
	{
		if ((nread = read(fdDaElaborare, ptr, nleft)) < 0)
		{
			if (nleft == n)
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
		else if (nread == 0)
		{
			//EOF
			break;
		}
		nleft -= nread;
		ptr += nread;
	}
  	return (n-nleft);//ritorno un valore maggiore o uguale a zero

}

int riceviDati(int fdDaElaborare, void *dest, size_t *sizePtr)
{
	size_t size = 0;

	//Leggo dal socket la grandezza del file che riceverò
	int a=readn(fdDaElaborare, &size, sizeof(size));
	if(a<0)
	{
		printf("Errore funzione leggiNBytes\n");
		return -1;
	}
	*sizePtr=size;
	void *dati = dest;
	char **puntatoreAusiliario = dest;

	//alloco lo spazio in cui leggerò il vero e proprio dato
	*puntatoreAusiliario = malloc(sizeof(**puntatoreAusiliario) * size);
  
	dati = *puntatoreAusiliario;
	int b=readn(fdDaElaborare, dati, size);
	if(b<0)
	{
		printf("Errore funzione leggiNBytes\n");
		return -1;
	}	

	return b;

}



int inviaDati(int fdDaElaborare, const void *data, size_t size)
{
	int writeNBytesReturnValue=writen(fdDaElaborare, (void *)data, size);
	return writeNBytesReturnValue;
}
