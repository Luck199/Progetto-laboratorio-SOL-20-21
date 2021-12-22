#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <math.h>

#include "utility.h"
#include "worker.h"
#include "gestioneFile.h"


#define NOOP ;

#define CLOSE(FD, S) \
  errno = 0;         \
  int c = close(FD); \
  if (c == -1)       \
  {                  \
    perror(S);       \
  }
#define HANDLE_WRN(A, S, OK, NE, IZ, IE, D) \
  errno = 0;                                \
  int r = A;                                \
  if (r == S)                               \
  {                                         \
    OK                                      \
  }                                         \
  else if (r == 0)                          \
  {                                         \
    IZ                                      \
  }                                         \
  else if (r == -1)                         \
  {                                         \
    IE                                      \
  }                                         \
  else if (r < S)                           \
  {                                         \
    NE                                      \
  }                                         \
  else                                      \
  {                                         \
    D                                       \
  }

#define HANDLE_WRNS(A, S, OK, KO) \
  HANDLE_WRN(A, S, OK, KO, KO, KO, KO)




void inizializza_worker()
{
	workers = calloc(thread_workers,sizeof(struct struttura_workers));
	int i;
	for(i=0; i<thread_workers; i++)
	{
	 	workers[i].id_worker = i;
	}
	////printf("SERVER-> inizializzati workers!\n");
}

void* vitaWorker(void*  idWorker)
{
	int indiceWorker=(intptr_t) idWorker;
	int cicli=0;
	int operazioneEseguita=0,chiudiConnessione=0,primaVolta=0;
	int stopWorker=0,numLetture=0;
	char bufferRicezione[200];
	//char bufferBackup[200];
	char operazioneRichiesta[50];
	////printf("SERVER-> indice Worker:%d\n",indiceWorker);
	workers[indiceWorker].threadId=pthread_self();
	char stringaToLog[100];
	strncpy(stringaToLog,"CIAO! SONO UN THREAD WORKER!",100);
	scriviSuLog(stringaToLog, 0);
	int fdDaElaborare=0;
	while (stopWorker == 0)
	{

		sleep(2);//temporaneo, fino a che non metto una condizione di attesa che un client abbia richiesto qualcosa
		if(chiudiConnessione == 1 || primaVolta==0)
		{
			primaVolta=1;
			fdDaElaborare=dequeueCodaFileDescriptor(codaFileDescriptor);

			chiudiConnessione=0;
		}

		////printf("ciclo: %d, fd: %d\n",cicli,fdDaElaborare);
		if(fdDaElaborare == -1)
		{
			////printf("NUMERO CICLO: %d->->->->-> sono in test -1\n",cicli);
			stopWorker=1;
		}
		else
		{
			////printf("NUMERO CICLO: %d->->->->-> sono in test else\n",cicli);
			////printf("WORKER -> HO LETTO DALLA CODA IL FILE DESCRIPTOR: %d\n", fdDaElaborare);

			int readReturnValue=read(fdDaElaborare,bufferRicezione,sizeof(bufferRicezione));
			if(readReturnValue == 0)
			{
				////printf("finiti i dati da leggere su questo fd!\n");
				chiudiConnessione=1;
			}
			if ((readReturnValue == -1) && (errno == EAGAIN))
			{
				perror("SERVER-> Errore: il fileDescriptor si riferisce a un file diverso da un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK), e la lettura si bloccherebbe. \n");
				chiudiConnessione=1;
			}

			if ((readReturnValue == -1) && (errno == EWOULDBLOCK))
			{
				perror("SERVER-> Errore: Il descrittore di file fd si riferisce a un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK) e la lettura si bloccherebbe.\n");
				chiudiConnessione=1;
			}
			if ((readReturnValue == -1) && (errno == EIO))
			{
				perror("SERVER-> Errore input/output");
				chiudiConnessione=1;
			}
			if ((readReturnValue == -1) && (errno == EBADF))
			{
				//perror("SERVER-> Errore: fileDescriptor non valido o non è  aperto in lettura.\n");
				chiudiConnessione=1;
			}
			if ((readReturnValue == -1) && (errno == EFAULT))
			{
				perror("SERVER-> Errore: bufferRicezione è fuori dal tuo spazio degli indirizzi accessibile");
				chiudiConnessione=1;
			}
			if ((readReturnValue == -1) && (errno == EINTR))
			{
				perror("SERVER-> Errore: La chiamata è stata interrotta da un segnale prima che i dati fossero letti\n");
				chiudiConnessione=1;
			}
			if ((readReturnValue == -1) && (errno == EINVAL))
			{
				perror("SERVER-> Errore: fd è attaccato ad un oggetto che non è adatto per la lettura; oppure il file è stato aperto con il flag O_DIRECT, e l'indirizzo specificato in bufferRicezione, il valore specificato in sizeof(bufferiRicezione) o l'offset del file non è adeguatamente allineato.\n");
				chiudiConnessione=1;
				break;
			}
			if ((readReturnValue == -1) && (errno == EISDIR))
			{
				perror("SERVER-> Errore: il file descriptor si riferiscxe ad una directory\n");
				chiudiConnessione=1;
				break;
			}




			if(chiudiConnessione != 1)
			{
				const char s[2] = ";";
				char *token;
				/* get the first token */
				token = strtok(bufferRicezione, s);
				/* walk through other tokens */
				if( token != NULL )
				{
					if(numLetture == 0)
					{
						strncpy(operazioneRichiesta,token,30);
						numLetture++;
					}
					//			if(numLetture == 1)
					//			{
					//
					//			}
					//			else
					//			{
					//				numLetture--;
					//			}
					//
					//			token = strtok(NULL,s);
				}
				if(strcmp(bufferRicezione,"OPEN_FILE")==0)
				{
					//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
					//i controlli sui parametri saranno effettuati da tale funzone
					//int aggiungiFileReturnValue=0;
					int openFileReturnValue=0;
					char *path;
					char *flag_array;
					int flag;
					//printf("Arrivata operazione OPEN_FILE\n");

					path=strtok(NULL, s);
					flag_array=strtok(NULL, s);
					flag=atoi(flag_array);
					printf("\n\n il worker fa la open file sul file : %s, utilizzando il flag %d \n\n",path,flag);


//					aggiungiFileReturnValue=aggiungiFile(token);
//					if(aggiungiFileReturnValue != 1)
//					{
//						write(fdDaElaborare,"OPEN_FILE: riscontrato errore\n",31);
//						strncpy(stringaToLog,"OPEN_FILE: riscontrato errore",30);
//						scriviSuLog(stringaToLog,0);
//					}

					openFileReturnValue=openFileServer(path,flag);
					if(openFileReturnValue != 1)
					{
						write(fdDaElaborare,"OPEN_FILE: riscontrato errore\n",31);
						strncpy(stringaToLog,"OPEN_FILE: riscontrato errore",30);
						scriviSuLog(stringaToLog,0);
					}

					openFileReturnValue=openFileServer(path,flag);
					if(openFileReturnValue != 1)
					{
						write(fdDaElaborare,"OPEN_FILE: riscontrato errore\n",31);
						strncpy(stringaToLog,"OPEN_FILE: riscontrato errore",30);
						scriviSuLog(stringaToLog,0);
					}

					else
					{
						write(fdDaElaborare,"OPEN_FILE eseguita correttamente!\n",35);
						strncpy(stringaToLog,"OPEN_FILE: eseguita operazione",31);
						scriviSuLog(stringaToLog,0);
					}
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);

					operazioneEseguita=1;
				}
				if(strcmp(bufferRicezione,"READ_FILE")==0)
				{
					////printf("Arrivata operazione READ_FILE\n");
					write(fdDaElaborare,"READ_FILE eseguita correttamente!\n",35);
					strncpy(stringaToLog,"READ_FILE: eseguita operazione",31);
					scriviSuLog(stringaToLog,0);
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);
					operazioneEseguita=1;
				}
				if(strcmp(bufferRicezione,"READ_N_FILE")==0)
				{
					////printf("Arrivata operazione READ_N_FILE\n");
					write(fdDaElaborare,"READ_N_FILE eseguita correttamente!\n",37);
					strncpy(stringaToLog,"READ_N_FILE: eseguita operazione",33);
					scriviSuLog(stringaToLog,0);
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);

					operazioneEseguita=1;
				}
				if(strcmp(bufferRicezione,"WRITE_FILE")==0)
				{
					////printf("Arrivata operazione WRITE_FILE\n");
					write(fdDaElaborare,"WRITE_FILE eseguita correttamente!\n",36);
					strncpy(stringaToLog,"WRITE_FILE: eseguita operazione",32);
					scriviSuLog(stringaToLog,0);
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);

					operazioneEseguita=1;
				}
				if(strcmp(bufferRicezione,"APPEND_TO_FILE")==0)
				{
					////printf("Arrivata operazione APPEND_TO_FILE\n");
					write(fdDaElaborare,"APPEND_TO_FILE eseguita correttamente!\n",40);
					strncpy(stringaToLog,"APPEND_TO_FILE: eseguita operazione",36);
					scriviSuLog(stringaToLog,0);
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);

					operazioneEseguita=1;
				}
				if(strcmp(bufferRicezione,"LOCK_FILE")==0)
				{
					//printf("Arrivata operazione LOCK_FILE\n");
					write(fdDaElaborare,"LOCK_FILE eseguita correttamente!\n",35);
					strncpy(stringaToLog,"LOCK_FILE: eseguita operazione",31);
					scriviSuLog(stringaToLog,0);
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);

					operazioneEseguita=1;
				}
				if(strcmp(bufferRicezione,"UNLOCK_FILE")==0)
				{
					//printf("Arrivata operazione UNLOCK_FILE\n");
					write(fdDaElaborare,"UNLOCK_FILE eseguita correttamente!\n",37);
					strncpy(stringaToLog,"UNLOCK_FILE: eseguita operazione",33);
					scriviSuLog(stringaToLog,0);
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);

					operazioneEseguita=1;
				}
				if(strcmp(bufferRicezione,"CLOSE_FILE")==0)
				{
					//printf("Arrivata operazione CLOSE_FILE\n");
					write(fdDaElaborare,"CLOSE_FILE eseguita correttamente!\n",36);
					strncpy(stringaToLog,"CLOSE_FILE: eseguita operazione",32);
					scriviSuLog(stringaToLog,0);
					operazioneEseguita=1;
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);
				}
				if(strcmp(bufferRicezione,"REMOVE_FILE")==0)
				{
					//printf("Arrivata operazione REMOVE_FILE\n");
					write(fdDaElaborare,"REMOVE_FILE eseguita correttamente!\n",37);
					strncpy(stringaToLog,"REMOVE_FILE: eseguita operazione",33);
					scriviSuLog(stringaToLog,0);
					strncpy(stringaToLog,"Richiesta Servita dal thread",32);
					scriviSuLog(stringaToLog,1,indiceWorker);

					operazioneEseguita=1;
				}
				////printf("\n\n\n\n\n\n\nGestita richiesta!\n");

			}

			if (operazioneEseguita == 1 && chiudiConnessione != 1)
			{
//				// sent the fd back
//				HANDLE_WRNS(writen(pipeGestioneWorkers[1], &fdDaElaborare, sizeof(fdDaElaborare)), sizeof(fdDaElaborare), NOOP,
//						{
//								perror("failed communication with main thread");
//						});
//				operazioneEseguita=0;
			}

			if(chiudiConnessione == 1)
			{
				int minusOne = -1;
				accediPipeWorker();
				HANDLE_WRNS(writen(pipeGestioneWorkers[1], &minusOne, sizeof(minusOne)), sizeof(minusOne), NOOP,
						{
								perror("failed communication with main thread");
						});
				decrementaNumClient();
				close(fdDaElaborare);
				strncpy(stringaToLog,"CLOSE: eseguita operazione",27);
				scriviSuLog(stringaToLog, 0);
				strncpy(stringaToLog,"Un client si è disconnesso, adesso il totale ammonta a",150);
				scriviSuLog(stringaToLog,1,clientConnessi);
				operazioneEseguita=0;
				lasciaPipeWorker();
			}
			cicli++;
		}
	}












	////printf("SERVER-> finito worker\n");
	pthread_exit(NULL);
}










