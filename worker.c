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
#include <stdarg.h>



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


char * dati;
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
	int stop=0;
	int primaVolta=0;
	int /*operazioneEseguita=0,*/chiudiConnessione=0;//,primaVolta=0;
//	int /*stopWorker=0*,*/numLetture=0;

//	char bufferRicezione[200]="";
	char *bufferRicezione=NULL;
	size_t daLeggere=0;
	workers[indiceWorker].threadId=pthread_self();
	char stringaToLog[100]="";
	strncpy(stringaToLog,"CIAO! SONO UN THREAD WORKER!",100);
	scriviSuLog(stringaToLog, 0);
	int fdDaElaborare=0;
	int stoLavorando=0;
	while ((getSegnale() !=2 || (getSegnale() != 1 && getNumClient()>0) ) /*|| stop != 0)*/)
	{
		accediCodaComandi();
		while((contatoreCodaFd == 0) && (broadcast==0))
		{
			if(stoLavorando==1)
			{
				//tramite questo if sto verificando se, nel caso stessi elborando delle richieste
				//di un  mio client ma fossi in condizione di mettermi in wait, non aspetto perchè continuo
				//l' elaborazione di tale richiesta
				break;
			}
//			printf("aspetto....\n");
			pthread_cond_wait(&(CVFileDescriptor), &(lockCodaComandi));
//			printf("svegliato!\n");
		}
		lasciaCodaComandi();
		if(getSegnale() == 2 || (getSegnale() == 1 && getNumClient()==0))
		{
			printf("TERMINO\n");
			stop=1;
			//sleep(2);
			break;
		}

		if(chiudiConnessione==1 || primaVolta==0)
		{
			primaVolta=1;
			fdDaElaborare=dequeueCodaFileDescriptor(codaFileDescriptor, &stop);

			stoLavorando=1;
			chiudiConnessione=0;
		}
		if(stop==1)
		{
			break;
		}
		else
		{
			//printf("fdDaElaborare:%d\n",fdDaElaborare);

			if(fdDaElaborare == -1)
			{
				////printf("NUMERO CICLO: %d->->->->-> sono in test -1\n",cicli);
				chiudiConnessione=1;
				//lasciaCodaComandi();
			}
			else
			{

				char * a=NULL;
				void *b=NULL;
				int readReturnValue=1;
				readReturnValue=riceviDati3(fdDaElaborare,&bufferRicezione,&daLeggere,0);
//						riceviDati(fdDaElaborare,&bufferRicezione,6);
//						printf("worker riceve: %s\n",bufferRicezione);
//						riceviDati(fdDaElaborare,&bufferRicezione,6);
				//int readReturnValue=read(fdDaElaborare,bufferRicezione,sizeof(bufferRicezione));

						printf("worker riceve: %s\n",bufferRicezione);




				if(readReturnValue == 0)
				{
					//printf("finiti i dati da leggere su questo fd!\n");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}
//				if(errno == EIO)
//				{
//					printf("errno: %d\n",errno);
//					chiudiConnessione=1;
////					lasciaCodaComandi();
////					break;
//				}

				else if ((readReturnValue == -1) && (errno == EAGAIN))
				{
					perror("SERVER-> Errore: il fileDescriptor si riferisce a un file diverso da un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK), e la lettura si bloccherebbe. \n");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}

				else if ((readReturnValue == -1) && (errno == EWOULDBLOCK))
				{
					perror("SERVER-> Errore: Il descrittore di file fd si riferisce a un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK) e la lettura si bloccherebbe.\n");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}
				else if ((readReturnValue == -1) && (errno == EIO))
				{
					perror("SERVER-> Errore input/output");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}
				else if ((readReturnValue == -1) && (errno == EBADF))
				{
					perror("SERVER-> Errore: fileDescriptor non valido o non è  aperto in lettura.\n");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}
				else if ((readReturnValue == -1) && (errno == EFAULT))
				{
					perror("SERVER-> Errore: bufferRicezione è fuori dal tuo spazio degli indirizzi accessibile");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}
				else if ((readReturnValue == -1) && (errno == EINTR))
				{
					perror("SERVER-> Errore: La chiamata è stata interrotta da un segnale prima che i dati fossero letti\n");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}
				else if ((readReturnValue == -1) && (errno == EINVAL))
				{
					perror("SERVER-> Errore: fd è attaccato ad un oggetto che non è adatto per la lettura; oppure il file è stato aperto con il flag O_DIRECT, e l'indirizzo specificato in bufferRicezione, il valore specificato in sizeof(bufferiRicezione) o l'offset del file non è adeguatamente allineato.\n");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}
				else if ((readReturnValue == -1) && (errno == EISDIR))
				{
					perror("SERVER-> Errore: il file descriptor si riferiscxe ad una directory\n");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}

				if(chiudiConnessione != 1)
				{
					const char puntoVirgola[2] = ";";
					char *token=NULL;
//					char* rest = bufferRicezione;
					char * rest;
					/* get the first token */
					token=strtok_r(bufferRicezione, puntoVirgola, &rest);
					/* walk through other tokens */
					if( token != NULL )
					{
						if(strcmp(token,"OPEN_FILE")==0)
						{
							//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
							//i controlli sui parametri saranno effettuati da tale funzone
							//int aggiungiFileReturnValue=0;
							int openFileServerReturnValue=0;
							int flag=0;
							printf("SERVER->Arrivata operazione OPEN_FILE\n");

							char * path=strtok_r(NULL, puntoVirgola, &rest);

							char *flag_array = strtok_r(NULL, puntoVirgola, &rest);

							flag=atoi(flag_array);

							printf("SERVER-> il worker fa la open file sul file : %s, utilizzando il flag %d\n",path,flag);


							accediStrutturaFile();
							openFileServerReturnValue=openFileServer(path,flag,fdDaElaborare);
							lasciaStrutturaFile();

							if(openFileServerReturnValue != 1)
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
							//operazioneEseguita=1;
						}
					}
					if(strcmp(bufferRicezione,"LOCK_FILE")==0)
					{
						//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
						//i controlli sui parametri saranno effettuati da tale funzone
						int lockFileServerReturnValue=0;

						printf("Arrivata operazione lock File\n");

						char * path=strtok_r(NULL, puntoVirgola, &rest);
					//	printf("il worker fa la lock file sul file : %s\n",path);

						accediStrutturaFile();
						lockFileServerReturnValue=lockFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						if(lockFileServerReturnValue != 1)
						{
							write(fdDaElaborare,"LOCK_FILE: riscontrato errore\n",31);
							strncpy(stringaToLog,"LOCK_FILE: riscontrato errore",31);
							scriviSuLog(stringaToLog,0);
						}
						else
						{
							write(fdDaElaborare,"LOCK_FILE eseguita correttamente!\n",35);
							strncpy(stringaToLog,"LOCK_FILE: eseguita operazione",31);
							scriviSuLog(stringaToLog,0);

						}
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						//operazioneEseguita=1;
					}
					if(strcmp(bufferRicezione,"UNLOCK_FILE")==0)
					{
						//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
						//i controlli sui parametri saranno effettuati da tale funzone
						int unlockFileServerReturnValue=0;

						printf("Arrivata operazione unlock File\n");

						char * path=strtok_r(NULL, puntoVirgola, &rest);
						printf("il worker fa la unlock file sul file : %s\n",path);

						accediStrutturaFile();
						unlockFileServerReturnValue=unlockFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						if(unlockFileServerReturnValue != 1)
						{
							write(fdDaElaborare,"UNLOCK_FILE: riscontrato errore\n",33);
							strncpy(stringaToLog,"UNLOCK_FILE: riscontrato errore",33);
							scriviSuLog(stringaToLog,0);
						}

						else
						{
							write(fdDaElaborare,"UNLOCK_FILE eseguita correttamente!\n",37);
							strncpy(stringaToLog,"UNLOCK_FILE: eseguita operazione",33);
							scriviSuLog(stringaToLog,0);
						}
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);

						//operazioneEseguita=1;

					}
					if(strcmp(bufferRicezione,"CLOSE_FILE")==0)
					{
						printf("Arrivata operazione CLOSE_FILE\n");
						int closeFileServerReturnValue=0;


						char * path=strtok_r(NULL, puntoVirgola, &rest);

						printf("il worker fa la close file sul file : %s \n\n",path);

						accediStrutturaFile();
						closeFileServerReturnValue=closeFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();
						if(closeFileServerReturnValue != 1)
						{
							write(fdDaElaborare,"CLOSE_FILE: riscontrato errore\n",32);
							strncpy(stringaToLog,"CLOSE_FILE: riscontrato errore",31);
							scriviSuLog(stringaToLog,0);
						}
						else
						{
							write(fdDaElaborare,"CLOSE_FILE eseguita correttamente!\n",36);
							strncpy(stringaToLog,"CLOSE_FILE: eseguita operazione",32);
							scriviSuLog(stringaToLog,0);
						}
						//operazioneEseguita=1;
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
					}
					if(strcmp(bufferRicezione,"REMOVE_FILE")==0)
					{
						int removeFileServerReturnValue=0;
						printf("Arrivata operazione REMOVE_FILE\n");
						char * path=strtok_r(NULL, puntoVirgola, &rest);

						accediStrutturaFile();
						removeFileServerReturnValue=removeFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						if(removeFileServerReturnValue != 1)
						{
							write(fdDaElaborare,"REMOVE_FILE: riscontrato errore\n",33);
							strncpy(stringaToLog,"REMOVE_FILE: riscontrato errore",32);
							scriviSuLog(stringaToLog,0);
						}
						else
						{
							write(fdDaElaborare,"REMOVE_FILE eseguita correttamente!\n",37);
							strncpy(stringaToLog,"REMOVE_FILE: eseguita operazione",33);
							scriviSuLog(stringaToLog,0);
						}
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						//operazioneEseguita=1;
					}
					if(strcmp(bufferRicezione,"APPEND_TO_FILE")==0)
					{
						printf("Arrivata operazione APPEND_TO_FILE\n\n\n");
						int appendToFileServerReturnValue=0;
						char * path=strtok_r(NULL, puntoVirgola, &rest);
						char * buffer=strtok_r(NULL, puntoVirgola, &rest);
						char * size_array=strtok_r(NULL, puntoVirgola, &rest);
						size_t size = atoi(size_array);
						char * dirname=strtok_r(NULL, puntoVirgola, &rest);
						printf("il worker esegue append file sul file %s scrivendoci:\n %s\n, il buffer è grande: %ld\n e la cartella dirname è uguale a %s\n",path,buffer,size,dirname);

						accediStrutturaFile();
						appendToFileServerReturnValue=appendToFileServer(path,buffer,size,dirname,fdDaElaborare);
						lasciaStrutturaFile();
						if(appendToFileServerReturnValue != 1)
						{
							write(fdDaElaborare,"APPEND_TO_FILE: riscontrato errore\n",36);
							strncpy(stringaToLog,"APPEND_TO_FILE: riscontrato errore",35);
							scriviSuLog(stringaToLog,0);
						}
						else
						{
							write(fdDaElaborare,"APPEND_TO_FILE eseguita correttamente!\n",36);
							strncpy(stringaToLog,"APPEND_TO_FILE: eseguita operazione",36);
							scriviSuLog(stringaToLog,0);
						}
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						//operazioneEseguita=1;
					}

					if(strcmp(bufferRicezione,"READ_FILE")==0)
					{
						printf("Arrivata operazione READ_FILE\n");
						int dimFile=0;
						char  stringa[200];
						char * buffer2=malloc(2000*sizeof(char));
						int readFileServerReturnValue=1;
						char * path=malloc(2000*sizeof(char));
						path=strtok_r(NULL, puntoVirgola, &rest);
//						printf("il worker esegue read file sul file %s \n",path);



						accediStrutturaFile();
						size_t size=0;
						buffer2=readFileServer(path,stringa,&size,fdDaElaborare);
						lasciaStrutturaFile();
						if(strcmp(buffer2,"errore")==0)
						{
							write(fdDaElaborare,"errore",7);
							sleep(1);
							write(fdDaElaborare,"errore",7);
						}
						else
						{
							char * invia=(char*)buffer2;
							write(fdDaElaborare,path,sizeof(path)+100);
							sleep(1);
							write(fdDaElaborare,buffer2,sizeof(buffer2)+100);

						}

						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);

						//operazioneEseguita=1;
					}
					if(strcmp(bufferRicezione,"READ_N_FILE")==0)
					{
						printf("Arrivata operazione READ_N_FILE\n");
						int N=0;
						char* dirname;
						char risposta[200]="";
						int read_N_FileServerReturnValue=-1;
						N=atoi(strtok_r(NULL, puntoVirgola, &rest));
						dirname=strtok_r(NULL, puntoVirgola, &rest);

						printf("WORKER-> N= %d, dirname= %s\n\n",N,dirname);
						accediStrutturaFile();
						read_N_FileServerReturnValue=readNFileServer(N,fdDaElaborare);
						lasciaStrutturaFile();

						char appoggio[10]="";

						if(read_N_FileServerReturnValue != -1)
						{
							strncat(risposta,"READ_N_FILE eseguita correttamente!;",37);
							sprintf(appoggio, "%d", read_N_FileServerReturnValue);
							strncat(risposta,appoggio,10);
							write(fdDaElaborare,risposta,37);
							strncpy(stringaToLog,"READ_N_FILE: eseguita operazione",33);
							scriviSuLog(stringaToLog,0);

						}
						else
						{
							strncat(risposta,"READ_N_FILE: riscontrato errore;",34);
							sprintf(appoggio, "%d", read_N_FileServerReturnValue);
							strncat(risposta,appoggio,10);


							write(fdDaElaborare,risposta,37);
							strncpy(stringaToLog,"READ_N_FILE: riscontrato errore",32);
							scriviSuLog(stringaToLog,0);
						}
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);

						//operazioneEseguita=1;
					}

					if(strcmp(bufferRicezione,"WRITE_FILE")==0)
					{
						printf("Arrivata operazione WRITE_FILE\n");
//
//
						int writeFileServerReturnValue=0;
						char * path="";
						path=strtok_r(NULL, puntoVirgola, &rest);
						printf("il worker esegue write file sul file %s \n",path);
						strncpy(stringaToLog,"Arrivata richiesta di writeFile",31);
						scriviSuLog(stringaToLog,1,indiceWorker);
						void *buf;
						size_t sizeFile=0;
						char *sizeArray="";
						sizeArray=strtok_r(NULL, puntoVirgola, &rest);
						sizeFile=atoi(sizeArray);
						//printf("Worker -> size:%ld\n\n\n",sizeFile);
//
						//write(fdDaElaborare,"ok",3);
						//riceviData
						void* buf2=strtok_r(NULL, puntoVirgola, &rest);
						printf("buf2:%s\n",buf2);
						char* dati;
						dati= malloc(sizeFile*sizeof(char));

						riceviDati3(fdDaElaborare, &dati, &sizeFile);
						printf("dati: %s\n\n\n\n\n\n\n\n",dati);
////
						accediStrutturaFile();
						writeFileServerReturnValue=writeFileServer(path,dati,sizeFile,fdDaElaborare);
						lasciaStrutturaFile();
						//free(dati);
						char  daInviare[200]="";
						if(writeFileServerReturnValue != 1)
						{
							strncpy(daInviare,"WRITE_FILE: riscontrato errore\n",32);
							strncpy(stringaToLog,"WRITE_FILE: riscontrato errore",33);
							scriviSuLog(stringaToLog,0);
						}
						else
						{
							strncpy(daInviare,"WRITE_FILE eseguita correttamente!",35);
							strncpy(stringaToLog,"WRITE_FILE: eseguita operazione",32);
							scriviSuLog(stringaToLog,0);
						}

////						strcat(daInviare,";");
////						strcat(daInviare,buffer2);
////						strcat(daInviare,";");
////						char appoggio[20];
////						sprintf(appoggio, "%ld",size);
////						strcat(daInviare,appoggio);
//						printf("il server risponde: %s\n",daInviare);
						write(fdDaElaborare,daInviare,sizeof(daInviare));
//
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
//

						//operazioneEseguita=1;
					}

				//operazioneEseguita=1;
			}

		}


//			if (//operazioneEseguita == 1 && chiudiConnessione != 1)
//			{
//				// sent the fd back
//				HANDLE_WRNS(writen(pipeGestioneWorkers[1], &fdDaElaborare, sizeof(fdDaElaborare)), sizeof(fdDaElaborare), NOOP,
//						{
//								perror("failed communication with main thread");
//						});
//				//operazioneEseguita=0;
//			}


			if(chiudiConnessione == 1)
			{
				int menoUno = -1;
				accediPipeWorker();
				HANDLE_WRNS(writen(pipeGestioneWorkers[1], &menoUno, sizeof(menoUno)), sizeof(menoUno), NOOP,
						{
								perror("failed communication with main thread");
						});
				if(fdDaElaborare!=-1)
				{
					close(fdDaElaborare);
					strncpy(stringaToLog,"CLOSE: eseguita operazione",27);
					scriviSuLog(stringaToLog, 0);
					strncpy(stringaToLog,"Un client si è disconnesso, adesso il totale ammonta a",150);
					scriviSuLog(stringaToLog,1,getNumClient());
					decrementaNumClient();
				}
				//operazioneEseguita=0;
				lasciaPipeWorker();
				stoLavorando=0;
			}
		}
	}
	pthread_exit(NULL);
}

void  riceviDati(int fdDaElaborare, void *dest, size_t size)
{

	//size_t size = 0;

	  // control flow flags
	  int sizeRead = 0;
	  int error = 0;
	  int done = 0;

	  // read the size
	//  readn(fdDaElaborare, &size, sizeof(size));

//	  if (sizeRead)
//	  {
	    // default behaviour: write to dest
	    void *writeTo = dest;

//	    if (alloc)
//	    {
	      // in this situation dest is considered as the address
	      // of a pointer that we have to set to the read data
	      char **destPtr = dest;

	      // malloc enough space
	      *destPtr = malloc(sizeof(**destPtr) * size);

	      // we have to write into the allocated space
	      writeTo = *destPtr;
//	    }

	    // read the data if writeTo is not NULL
	    //if (writeTo)
	    {
	      readn(fdDaElaborare, writeTo, size);
	      //printf("writeTo:%s\n",writeTo);
	    }
//	    else
//	    {
//	      error = 1;
//	    }
	//  }

//	  if (done)
//	  {
//	    // return the size as well
//	    sizePtr ? *sizePtr = size : 0;
//	    return 0;
//	  }
//
//	  if (error)
//	  {
//	    perror("getData has failed");
//	    return -1;
//	  }


//	}
}

//ssize_t readn(int fd, void *v_ptr, size_t n)
//{
//  char *ptr = v_ptr;
//  size_t nleft;
//  ssize_t nread;
//
//  nleft = n;
//  while (nleft > 0)
//  {
//    if ((nread = read(fd, ptr, nleft)) < 0)
//    {
//      if (nleft == n)
//        return -1; /* error, return -1 */
//      else
//        break; /* error, return amount read so far */
//    }
//    else if (nread == 0)
//    {
//      break; /* EOF */
//    }
//    nleft -= nread;
//    ptr += nread;
//  }
//  return (n - nleft); /* return >= 0 */
//}
//
///* Write "n" bytes to a descriptor */
//ssize_t writen(int fd, void *v_ptr, size_t n)
//{
//  char *ptr = v_ptr;
//  size_t nleft;
//  ssize_t nwritten;
//
//  nleft = n;
//  while (nleft > 0)
//  {
//    if ((nwritten = write(fd, ptr, nleft)) < 0)
//    {
//      if (nleft == n)
//        return -1; /* error, return -1 */
//      else
//        break; /* error, return amount written so far */
//    }
//    else if (nwritten == 0)
//      break;
//    nleft -= nwritten;
//    ptr += nwritten;
//  }
//  return (n - nleft); /* return >= 0 */
//}
//
//

int getData(int fdDaElaborare, void *dest, size_t *sizePtr, int alloc)
{
  size_t size = 0;
printf("cucù\n\n\n\n");
  // control flow flags
  int sizeRead = 0;
  int error = 0;
  int done = 0;

  // read the size
  //readn(fdDaElaborare, &size, sizeof(size));
  void *writeTo = dest;


      // in this situation dest is considered as the address
      // of a pointer that we have to set to the read data
      char **destPtr = dest;

      // malloc enough space
      *destPtr = malloc(sizeof(**destPtr) * size);

      // we have to write into the allocated space
      writeTo = *destPtr;


  readn(fdDaElaborare, writeTo, size);
printf("writeTo:%s\n",writeTo);

  return 0;
}


int riceviDati3(int fd, void *dest, size_t *sizePtr, int alloc)
{
  size_t size = 0;

  // control flow flags
  int sizeRead = 0;
  int error = 0;
  int done = 0;

  // read the size
  int a=readn(fd, &size, sizeof(size));
  if(a<0)
  {
	  printf("boh\n");
  }
  printf("sizeletta:%ld\n",*sizePtr);
  //if (sizeRead)
  {
    // default behaviour: write to dest
    void *writeTo = dest;

   // if (alloc)
    {
      // in this situation dest is considered as the address
      // of a pointer that we have to set to the read data
      char **destPtr = dest;

      // malloc enough space
      *destPtr = malloc(sizeof(**destPtr) * size);

      // we have to write into the allocated space
      writeTo = *destPtr;
    }

    // read the data if writeTo is not NULL
    //if (writeTo)
    {
    	int b=readn(fd, writeTo, size);
    	if(b<0)
    	{
    		printf("boh2\n");
    	}
    	return b;
    }
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
//
//  return -1;
}

