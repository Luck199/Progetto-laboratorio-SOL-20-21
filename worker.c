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


#include "comunicazioneClientServer.h"
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

				int readReturnValue=1;
				readReturnValue=riceviDati(fdDaElaborare,&bufferRicezione,&daLeggere);


				if(readReturnValue == 0)
				{
					//printf("finiti i dati da leggere su questo fd!\n");
					chiudiConnessione=1;
//					lasciaCodaComandi();
				}

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
					char * rest=NULL;
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

							char  daInviare[200]="";


							if(openFileServerReturnValue != 1)
							{
								strncpy(daInviare,"OPEN_FILE: riscontrato errore\n",31);
								if(flag==1 || flag ==2)
								{
									strncpy(stringaToLog,"OPEN_FILE_LOCK: riscontrato errore",32);
									scriviSuLog(stringaToLog,0);
								}
								else
								{
									strncpy(stringaToLog,"OPEN_FILE: riscontrato errore",32);
									scriviSuLog(stringaToLog,0);
								}
							}
							else
							{
								if(flag==1 || flag ==2)
								{
									strncpy(stringaToLog,"OPEN_FILE_LOCK: eseguita operazione",31);
									scriviSuLog(stringaToLog,0);
								}
								else
								{
									strncpy(stringaToLog,"OPEN_FILE: eseguita operazione",31);
									scriviSuLog(stringaToLog,0);

								}
								strncpy(daInviare,"OPEN_FILE eseguita correttamente!\n",35);
							}
							size_t a=strlen(daInviare);
							sendData(fdDaElaborare,&a,sizeof(size_t));
							sendData(fdDaElaborare,&daInviare,a);
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

						char * path=NULL;//strtok_r(NULL, puntoVirgola, &rest);
					//	printf("il worker fa la lock file sul file : %s\n",path);
						size_t b=0;
						riceviDati(fdDaElaborare,&path,&b);
						accediStrutturaFile();
						lockFileServerReturnValue=lockFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						char  daInviare[200]="";


						if(lockFileServerReturnValue != 1)
						{

							strncpy(daInviare,"LOCK_FILE: riscontrato errore\n",31);
							strncpy(stringaToLog,"LOCK_FILE: riscontrato errore",32);
							scriviSuLog(stringaToLog,0);
						}
						else
						{
							strncpy(daInviare,"LOCK_FILE eseguita correttamente!\n",35);
							strncpy(stringaToLog,"LOCK_FILE: eseguita operazione",31);
							scriviSuLog(stringaToLog,0);

						}




						size_t a=strlen(daInviare);
						sendData(fdDaElaborare,&a,sizeof(size_t));
						sendData(fdDaElaborare,&daInviare,a);
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


						char * path=NULL;//strtok_r(NULL, puntoVirgola, &rest);
						//	printf("il worker fa la lock file sul file : %s\n",path);
						size_t b=0;
						riceviDati(fdDaElaborare,&path,&b);





						//char * path=strtok_r(NULL, puntoVirgola, &rest);
						printf("il worker fa la unlock file sul file : %s\n",path);

						accediStrutturaFile();
						unlockFileServerReturnValue=unlockFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						char daInviare[200]="";
						if(unlockFileServerReturnValue != 1)
						{
							strncpy(daInviare,"UNLOCK_FILE: riscontrato errore\n",33);
							strncpy(stringaToLog,"UNLOCK_FILE: riscontrato errore",34);
							scriviSuLog(stringaToLog,0);
						}

						else
						{
							strncpy(daInviare,"UNLOCK_FILE eseguita correttamente!\n",37);
							strncpy(stringaToLog,"UNLOCK_FILE: eseguita operazione",33);
							scriviSuLog(stringaToLog,0);

						}
						size_t a=strlen(daInviare);
						sendData(fdDaElaborare,&a,sizeof(size_t));
						sendData(fdDaElaborare,&daInviare,a);
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


						char daInviare[200]="";

						if(closeFileServerReturnValue != 1)
						{
							strncpy(daInviare,"CLOSE_FILE: riscontrato errore\n",32);
							strncpy(stringaToLog,"CLOSE_FILE: riscontrato errore",33);
							scriviSuLog(stringaToLog,0);

						}
						else
						{
							strncpy(daInviare,"CLOSE_FILE eseguita correttamente!\n",36);
							strncpy(stringaToLog,"CLOSE_FILE: eseguita operazione",32);
							scriviSuLog(stringaToLog,0);
						}
						//operazioneEseguita=1;

						size_t a=strlen(daInviare);
						sendData(fdDaElaborare,&a,sizeof(size_t));
						sendData(fdDaElaborare,&daInviare,a);

						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
					}
					if(strcmp(bufferRicezione,"REMOVE_FILE")==0)
					{
						int removeFileServerReturnValue=0;
						printf("Arrivata operazione REMOVE_FILE\n");
						char * path=NULL;//strtok_r(NULL, puntoVirgola, &rest);

						size_t b=0;
						riceviDati(fdDaElaborare,&path,&b);

						accediStrutturaFile();
						removeFileServerReturnValue=removeFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						char daInviare[200]="";
						if(removeFileServerReturnValue != 1)
						{
							strncpy(daInviare,"REMOVE_FILE: riscontrato errore",32);
							strncpy(stringaToLog,"REMOVE_FILE: riscontrato errore",32);
							scriviSuLog(stringaToLog,0);
						}

						else
						{
							strncpy(daInviare,"REMOVE_FILE eseguita correttamente!\n",37);
							strncpy(stringaToLog,"REMOVE_FILE: eseguita operazione",33);
							scriviSuLog(stringaToLog,0);
						}
						size_t a=strlen(daInviare);
						sendData(fdDaElaborare,&a,sizeof(size_t));
						sendData(fdDaElaborare,&daInviare,a);
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
						//appendToFileServerReturnValue=appendToFileServer(path,buffer,size,dirname,fdDaElaborare);
						lasciaStrutturaFile();
						char daInviare[200]="";
						if(appendToFileServerReturnValue != 1)
						{
							strncpy(daInviare,"APPEND_TO_FILE: riscontrato errore",32);
							strncpy(stringaToLog,"APPEND_TO_FILE: riscontrato errore",32);
							scriviSuLog(stringaToLog,0);

						}
						else
						{
							strncpy(daInviare,"APPEND_TO_FILE eseguita correttamente!\n",37);
							strncpy(stringaToLog,"APPEND_TO_FILE: eseguita operazione",33);
							scriviSuLog(stringaToLog,0);
						}


						size_t a=strlen(daInviare);
						sendData(fdDaElaborare,&a,sizeof(size_t));
						sendData(fdDaElaborare,&daInviare,a);





						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						//operazioneEseguita=1;
					}

					if(strcmp(bufferRicezione,"READ_FILE")==0)
					{
						printf("Arrivata operazione READ_FILE\n");
						char * buffer2;
						char * path;
						path=strtok_r(NULL, puntoVirgola, &rest);
//						printf("il worker esegue read file sul file %s \n",path);

						size_t dimFile=0;
						char result[200]="";
						accediStrutturaFile();
						int indice=-1;
						indice=readFileServer(path,buffer2,&dimFile,fdDaElaborare);
						if(indice!=-1)
						{
							buffer2=malloc(sizeof(char)*array_file[indice].dimensione);
							memcpy(buffer2,array_file[indice].byteFile,array_file[indice].dimensione);
						}
						lasciaStrutturaFile();
						if(indice==-1)
						{
							printf("mando errore\n");
							strncpy(result,"errore",6);
							size_t a=strlen(result)+1;
							sendData(fdDaElaborare,&a,sizeof(size_t));
							sendData(fdDaElaborare,&result,a);
						}
						else
						{
							printf("\n\n\n\ninvio al client: %s!!! e la sua lunghezza is: %ld \n\n\n\n",buffer2,dimFile);
							strncpy(stringaToLog,"READ_FILE di byte",32);
							scriviSuLog(stringaToLog,1,dimFile);

							sendData(fdDaElaborare,&dimFile,sizeof(size_t));
							sendData(fdDaElaborare,buffer2,dimFile);
						}

						free(buffer2);
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


						if(N>numFilePresenti || N==0)
						{
							N=numFilePresenti;
						}
						printf("WORKER-> N= %d, dirname= %s\n\n",N,dirname);

						int i;
						char * buffer2=NULL;
						size_t dimFile=0;
						size_t lungPath=0;
						char stringa[30]="";
						char *path=NULL;
						int fileLettiEffettivi=0;
						accediStrutturaFile();

						//adesso nella variabile N ho il numero corretto di file che devo inviare al client,
						//invierò i primi N della struttura contenente i file
						for(i=0;i<N;i++)
						{
							printf("ciao a tutti!\n\n\n\n\n");
							if(array_file[i].identificatoreClient!=fdDaElaborare && array_file[i].O_LOCK!=1)
							{
								continue;
							}
							buffer2=malloc(sizeof(char)*array_file[i].dimensione);
							memcpy(buffer2,array_file[i].byteFile,array_file[i].dimensione);
							path=malloc(sizeof(char)*(strlen(array_file[i].path)+1));

							memcpy(path,array_file[i].path,strlen(array_file[i].path)+1);
							printf("path: %s\n",path);
							lungPath=strlen(array_file[i].path);
							sendData(fdDaElaborare,&lungPath,sizeof(size_t));
							sendData(fdDaElaborare,path,lungPath);


							dimFile=array_file[i].dimensione;
							sendData(fdDaElaborare,&dimFile,sizeof(size_t));
							sendData(fdDaElaborare,buffer2,dimFile);
						}
						strncpy(stringa,"finito",8);
						size_t a=strlen(stringa);
						sendData(fdDaElaborare,&a,sizeof(size_t));
						sendData(fdDaElaborare,&stringa,a);

						lasciaStrutturaFile();




//						char daInviare[200]="";
//
//						char appoggio[10]="";
//
//						if(read_N_FileServerReturnValue != -1)
//						{
//							strncpy(daInviare,"READ_N_FILE: riscontrato errore",32);
							strncpy(stringaToLog,"READ_N_FILE: riscontrato errore",32);
							scriviSuLog(stringaToLog,0);
//						}
//						else
//						{
//							strncpy(daInviare,"READ_N_FILE eseguita correttamente!\n",37);
							strncpy(stringaToLog,"READ_N_FILE: eseguita operazione",33);
							scriviSuLog(stringaToLog,0);
//						}
//						size_t a=strlen(daInviare);
//						sendData(fdDaElaborare,&a,sizeof(size_t));
//						sendData(fdDaElaborare,&daInviare,a);

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
						char * path= malloc(200*sizeof(char));
						//printf("array_file[0].path:%s\n",array_file[0].path);
						path=strtok_r(NULL, puntoVirgola, &rest);
						if(path == NULL)
						{
							writeFileServerReturnValue=-1;
						}
//						else
//						{
//
//						}
//						printf("il worker esegue write file sul file %s \n",path);
						strncpy(stringaToLog,"Arrivata richiesta di writeFile",33);
						scriviSuLog(stringaToLog,0);
						size_t sizeFile=0;
//						char *sizeArray="";
//						sizeArray=strtok_r(NULL, puntoVirgola, &rest);
//						sizeFile=atoi(sizeArray);
						//printf("Worker -> size:%ld\n\n\n",sizeFile);
//						printf("worker-> path:%s\n",path);
						//write(fdDaElaborare,"ok",3);
						//riceviData
						//void* buf2=strtok_r(NULL, puntoVirgola, &rest);
						//printf("buf2:%s\n",buf2);
						void * dati;
						//dati= malloc(sizeFile*sizeof(char));
						riceviDati(fdDaElaborare, &dati, &sizeFile);
						//printf("dati : %s\n",dati);
						accediStrutturaFile();
						writeFileServerReturnValue=writeFileServer(path,dati,sizeFile,fdDaElaborare);
						lasciaStrutturaFile();


						free(dati);
						char daInviare[200]="";
						if(writeFileServerReturnValue != 1)
						{
							strncpy(daInviare,"WRITE_FILE: riscontrato errore",32);
							strncpy(stringaToLog,"WRITE_FILE: riscontrato errore",33);
							scriviSuLog(stringaToLog,0);
						}
						else
						{
							strncpy(daInviare,"WRITE_FILE eseguita correttamente!",35);
							strncpy(stringaToLog,"WRITE_FILE di byte",32);
							scriviSuLog(stringaToLog,1,sizeFile);
						}
						size_t a=strlen(daInviare);
						sendData(fdDaElaborare,&a,sizeof(size_t));
						sendData(fdDaElaborare,&daInviare,a);

						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						//free(path);
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
				free(bufferRicezione);
				//operazioneEseguita=0;
				lasciaPipeWorker();
				stoLavorando=0;
			}
		}
	}
	pthread_exit(NULL);
}
