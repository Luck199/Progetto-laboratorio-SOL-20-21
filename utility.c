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
#include "coda.h"
#include "utility.h"
#include "gestioneFile.h"


#include <limits.h>

#include "apiServer.h"
#include "utility.h"
#include "gestioneFile.h"

#define DIM 100
#define UNIX_PATH_MAX 108

int numMaxconnessioniContemporanee=-1;
int clientTotali=0;
int thread_workers;
int threadInAttesa=0;
int segnale_globale=0;
int dim_memoria;
int broadcast=0;
char name_socket[100];
int num_max_file;
int ricevutoSIGHUP=0;
int ricevutoSIGINTORQUIT=0;
FILE *logFile;
int contatoreCodaFd=0;
struct codaInteri *codaFileDescriptor;
int segnaleChiusuraHup = 0;

int pipeGestioneWorkers[2];

int fdInCoda=0;
int clientConnessi=0;
pthread_mutex_t lockClientConnessi = PTHREAD_MUTEX_INITIALIZER;//lock per la gestione del numero di client connessi
pthread_mutex_t lockCodaComandi = PTHREAD_MUTEX_INITIALIZER;//lock per la scrittura sulla coda dei fileDescriptor
pthread_mutex_t lockPipeWorker = PTHREAD_MUTEX_INITIALIZER;//lock per la scrittura sulla pipe per la gestione dei workers
pthread_mutex_t lockScritturaLog = PTHREAD_MUTEX_INITIALIZER;//lock per la scrittura sul file di log

pthread_cond_t allClientExitCond = PTHREAD_COND_INITIALIZER;//condition variable per chiusura server con segnale SIGHUP

pthread_cond_t CVFileDescriptor = PTHREAD_COND_INITIALIZER;//condition VAriable
pthread_mutex_t lockSegnali=PTHREAD_MUTEX_INITIALIZER;
struct struttura_workers *workers;


struct info_file *array_file;

void letturaFile(char *config, char *nomeFilelog)
{
    char *parola, *riga = NULL, *value;
    FILE *f;
    size_t len = 0;
    ssize_t r;

    f = fopen(config, "r");
    if(f == NULL)
    {
        perror("Error fopen");
        exit(EXIT_FAILURE);
    }

    while((r=getline(&riga,&len,f)) != -1)
    {

      	parola = strtok(riga, " ");
        value = strtok(NULL, " ");

        if(strcmp(parola, "thread_workers") == 0 )
        {
        	thread_workers = (int) atol(value);
            continue;
        }
        if(strcmp(parola, "dim_memoria") == 0 )
        {
        	dim_memoria = (int) atol(value);
            continue;
        }
        if(strcmp(parola, "socket_file") == 0 )
        {
        	int n=strlen(value);
            char *fchar = strpbrk(value, "\n");
            strcpy(fchar, "");
        	strncpy(name_socket, value, n+1);
        	continue;
        }
        if(strcmp(parola, "num_max_file") == 0)
        {
        	num_max_file = (int) atol(value);
            continue;
        }

        if(strcmp(parola, "logfile") == 0)
        {
        	int n=strlen(value);
            char *fchar = strpbrk(value, "\n");
            strcpy(fchar, "");
        	strncpy(nomeFilelog, value, n);
        	continue;
        }

        ////printf("Stringa %s non riconosciuta\n", parola);
        exit(EXIT_FAILURE);
    }
    free(parola);

    fclose(f);
}

void decrementaNumClient()
{
	int err;
	if ( ( err=pthread_mutex_lock(&lockClientConnessi)) != 0 )
	{
		perror("lock numero clienti in supermercato");
		pthread_exit(&err);
	}
	clientConnessi--;
	printf("SERVER-> Sconnesso Client! Sono connessi %d client\n",clientConnessi);
	////printf("numero fd presenti in coda: %d, numero client connessi: %d\n",contatoreCodaFd,clientConnessi);

	//Verifico se non sono presenti client connessi
	//e se fosse arrivato segnale sighup; in caso positivo
	//invio una signal per svegliare il thread main che può terminare
	if ((clientConnessi == 0) && (segnaleChiusuraHup == 1))
	{
		//printf("fatta signal\n");
		pthread_cond_signal(&allClientExitCond);
	}
	pthread_mutex_unlock(&lockClientConnessi);
}

//Funzione che permette di incrementare il numero dei clienti attualmente all' interno del supermercato
//in mutua esclusione
void incrementaNumClient()
{
	int err;
	if ( ( err=pthread_mutex_lock(&lockClientConnessi)) != 0 )
	{
		perror("lock numero clienti in supermercato");
		pthread_exit(&err);
	}

	clientTotali++;
	clientConnessi++;
	if(numMaxconnessioniContemporanee<clientConnessi)
	{
		numMaxconnessioniContemporanee=clientConnessi;
	}
	printf("SERVER-> Sono connessi %d client\n",clientConnessi);
	////printf("numero fd presenti in coda: %d, numero client connessi: %d\n",contatoreCodaFd,clientConnessi);
	pthread_mutex_unlock(&lockClientConnessi);
}


//Funzione che permette di incrementare il numero dei clienti attualmente all' interno del supermercato
//in mutua esclusione
int getNumClient()
{
	int err, result;
	if ( ( err=pthread_mutex_lock(&lockClientConnessi)) != 0 )
	{
		pthread_exit(&err);
	}

	result = clientConnessi;
	pthread_mutex_unlock(&lockClientConnessi);
	return result;
}

//Funzione che riceve una stringa e dà la possibilità di scriverla sul file di log
//in mutua esclusione
void scriviSuLog(char * stringa, int count, ...)
{
	fflush(logFile);
	int errore;
	if ( ( errore=pthread_mutex_lock(&lockScritturaLog)) != 0 )
	{
		perror("lock scrittura log");
		pthread_exit(&errore);
	}
	va_list ap;
	va_start (ap, count); // inizializzo la lista
	int sum = 0;
//	for (int i=0; i<count; i++)
//	{
		sum += va_arg (ap, int); // il prossimo
//	}
	if(count == 0)
	{
		fprintf(logFile, "%s \n" ,stringa);
	}
	else
	{
		fprintf(logFile, "%s: %d \n" ,stringa, sum);
	}

	va_end (ap);
	pthread_mutex_unlock(&lockScritturaLog);
}


//Funzione write con un numero di bytes massimo uguale ad n

void accediCodaComandi()
{
	int err;
	if ( ( err=pthread_mutex_lock(&lockCodaComandi)) != 0 )
	{
		perror("lock coda comandi\n");
		pthread_exit(&err);
	}
	//printf("SERVER-> Assunto lock coda comandi\n");
}

//Funzione che permette di rilasciare il lock relativa alla struttura dati contenente le casse.
void lasciaCodaComandi()
{
	int err;
	if ( ( err=pthread_mutex_unlock(&lockCodaComandi)) != 0 )
	{
		perror("unlock coda comandi\n");
		pthread_exit(&err);
	}
	//printf("SERVER-> Rilasciato lock coda comandi\n");
}

void accediPipeWorker()
{
	int errore;
	if ( ( errore=pthread_mutex_lock(&lockPipeWorker)) != 0 )
	{
		perror("lock pipe worker\n");
		pthread_exit(&errore);
	}
	//printf("SERVER-> Assunto lock pipe worker\n");
}

void lasciaPipeWorker()
{
	int errore;
	if ( ( errore=pthread_mutex_unlock(&lockPipeWorker)) != 0 )
	{
		perror("lock pipe worker\n");
		pthread_exit(&errore);
	}
	//////printf("SERVER-> Rilasciato lock pipe worker\n");
}






void enqueueCodaFileDescriptor(struct codaInteri *codaFileDescriptor, int fileDescriptorPointer)
{
	accediCodaComandi();
	enqueue_Interi(codaFileDescriptor,fileDescriptorPointer);
	contatoreCodaFd++;
	//printf("contatoreCodaFd: %d\n",contatoreCodaFd);
	pthread_cond_signal(&CVFileDescriptor);
	//StampaLista_Interi(codaFileDescriptor);
	lasciaCodaComandi();
}


int dequeueCodaFileDescriptor(struct codaInteri *codaFileDescriptor, int *stop)
{
	int fdDaElaborare;
	accediCodaComandi();
	int errore=0;
	fdDaElaborare=dequeue_Interi(codaFileDescriptor, &errore);
	//printf("fd pescato:%d\n",fdDaElaborare);
	if(errore==1)
	{
		*stop=1;
		//perror("ERRORE\n");
		//printf("contatoreCodaFd: %d\n",contatoreCodaFd);
	}
	if(fdDaElaborare!=-1)
	{
		contatoreCodaFd--;
		//printf("contatoreCodaFd: %d\n",contatoreCodaFd);
	}

	//StampaLista_Interi(codaFileDescriptor);
	lasciaCodaComandi();

	return fdDaElaborare;
}


void accediSegnali()
{
	int err;
	if 	( ( err=pthread_mutex_lock(&lockSegnali)) != 0 )
	{
		perror("lock coda comandi\n");
		pthread_exit(&err);
	}
//	printf("SERVER-> Assunto lock segnali\n");
}


void lasciaSegnali()
{
	int err;
	if 	( ( err=pthread_mutex_unlock(&lockSegnali)) != 0 )
	{
		perror("lock coda comandi\n");
		pthread_exit(&err);
	}
//	printf("SERVER-> Lasciato lock segnali\n");
}

int getSegnale()
{
	int result=0;
	accediSegnali();
	result=segnale_globale;
	lasciaSegnali();
	return result;
}


