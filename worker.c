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
}

void* vitaWorker(void*  idWorker)
{
	int indiceWorker=(intptr_t) idWorker;
	int stop=0;
	int primaVolta=0;
	int chiudiConnessione=0;

	char *bufferRicezione=NULL;
	size_t daLeggere=0;
	workers[indiceWorker].threadId=pthread_self();
	char stringaToLog[100]="";
	int fdDaElaborare=0;
	int stoLavorando=0;
	while ((getSegnale() !=2 || (getSegnale() != 1 && getNumClient()>0) ))
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
			pthread_cond_wait(&(CVFileDescriptor), &(lockCodaComandi));

		}
		lasciaCodaComandi();
		if(getSegnale() == 2 || (getSegnale() == 1 && getNumClient()==0))
		{
			printf("TERMINO\n");
			stop=1;
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
			if(fdDaElaborare == -1)
			{
				chiudiConnessione=1;

			}
			else
			{

				int readReturnValue=1;
				readReturnValue=riceviDati(fdDaElaborare,&bufferRicezione,&daLeggere);
				if(readReturnValue == 0 )
				{
					// se entro in questo if significa che sono finiti
					//i dati da leggere su questo file Descriptor
					chiudiConnessione=1;
				}

				else if ((readReturnValue == -1) && (errno == EAGAIN))
				{
					perror("SERVER-> Errore: il fileDescriptor si riferisce a un file diverso da un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK), e la lettura si bloccherebbe. \n");
					chiudiConnessione=1;
				}

				else if ((readReturnValue == -1) && (errno == EWOULDBLOCK))
				{
					perror("SERVER-> Errore: Il descrittore di file fd si riferisce a un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK) e la lettura si bloccherebbe.\n");
					chiudiConnessione=1;
				}
				else if ((readReturnValue == -1) && (errno == EIO))
				{
					perror("SERVER-> Errore input/output");
					chiudiConnessione=1;
				}
				else if ((readReturnValue == -1) && (errno == EBADF))
				{
					perror("SERVER-> Errore: fileDescriptor non valido o non è  aperto in lettura.\n");
					chiudiConnessione=1;
				}
				else if ((readReturnValue == -1) && (errno == EFAULT))
				{
					perror("SERVER-> Errore: bufferRicezione è fuori dal tuo spazio degli indirizzi accessibile");
					chiudiConnessione=1;
				}
				else if ((readReturnValue == -1) && (errno == EINTR))
				{
					perror("SERVER-> Errore: La chiamata è stata interrotta da un segnale prima che i dati fossero letti\n");
					chiudiConnessione=1;
				}
				else if ((readReturnValue == -1) && (errno == EINVAL))
				{
					perror("SERVER-> Errore: fd è attaccato ad un oggetto che non è adatto per la lettura; oppure il file è stato aperto con il flag O_DIRECT, e l'indirizzo specificato in bufferRicezione, il valore specificato in sizeof(bufferiRicezione) o l'offset del file non è adeguatamente allineato.\n");
					chiudiConnessione=1;
				}
				else if ((readReturnValue == -1) && (errno == EISDIR))
				{
					perror("SERVER-> Errore: il file descriptor si riferiscxe ad una directory\n");
					chiudiConnessione=1;
				}

				if(chiudiConnessione != 1)
				{
					const char puntoVirgola[2] = ";";
					char *token=NULL;
					char * rest=NULL;
					token=strtok_r(bufferRicezione, puntoVirgola, &rest);
					if( token != NULL )
					{
						if(strcmp(token,"OPEN_FILE")==0)
						{
							//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
							//i controlli sui parametri saranno effettuati da tale funzone
							int openFileServerReturnValue=0;
							int flag=0;


							size_t dimPath =0;
							char * path=NULL;

							int readReturnValue2=riceviDati(fdDaElaborare,&path,&dimPath);
							if(readReturnValue2<0)
							{
								printf("errore\n");
							}




							char *flag_array = NULL;

							size_t dimFlagArray=0;
							int readReturnValue3=riceviDati(fdDaElaborare,&flag_array,&dimFlagArray);
							if(readReturnValue3<0)
							{
								printf("errore\n");
							}

							flag=atoi(flag_array);

							accediStrutturaFile();
							openFileServerReturnValue=openFileServer(path,flag,fdDaElaborare);
							lasciaStrutturaFile();

							free(path);
							free(flag_array);
							char  daInviare[200]="";

							//visualizzaArrayFile();
							if(openFileServerReturnValue != 1)
							{
								strncpy(daInviare,"OPEN_FILE: riscontrato errore",31);
								if(flag==1 || flag ==2)
								{
									strncpy(stringaToLog,"OPEN_FILE_LOCK: riscontrato errore",35);
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
									strncpy(stringaToLog,"OPEN_FILE_LOCK: eseguita operazione",37);
									scriviSuLog(stringaToLog,0);
								}
								else
								{
									strncpy(stringaToLog,"OPEN_FILE: eseguita operazione",31);
									scriviSuLog(stringaToLog,0);
								}
								strncpy(daInviare,"OPEN_FILE eseguita correttamente!",35);
							}

							size_t dimStringaDaInviare=strlen(daInviare)+1;
							inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
							inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);
							strncpy(stringaToLog,"Richiesta Servita dal thread",32);
							scriviSuLog(stringaToLog,1,indiceWorker);
							free(bufferRicezione);
						}
					}
					if(strcmp(bufferRicezione,"LOCK_FILE")==0)
					{
						//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
						//i controlli sui parametri saranno effettuati da tale funzone
						int lockFileServerReturnValue=0;


						char * path=NULL;
						size_t dimPath=0;
						riceviDati(fdDaElaborare,&path,&dimPath);
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


						free(path);

						size_t dimStringaDaInviare=strlen(daInviare);
						inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						free(bufferRicezione);
					}
					if(strcmp(bufferRicezione,"UNLOCK_FILE")==0)
					{
						//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
						//i controlli sui parametri saranno effettuati da tale funzone
						int unlockFileServerReturnValue=0;



						char * path=NULL;
						size_t dimPath=0;
						riceviDati(fdDaElaborare,&path,&dimPath);






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
						free(path);
						size_t dimStringaDaInviare=strlen(daInviare);
						inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						free(bufferRicezione);

					}
					if(strcmp(bufferRicezione,"CLOSE_FILE")==0)
					{
						printf("Arrivata operazione CLOSE_FILE\n");
						int closeFileServerReturnValue=0;

						size_t dimPath=0;
						char * path=NULL;
						riceviDati(fdDaElaborare,&path,&dimPath);
						accediStrutturaFile();
						closeFileServerReturnValue=closeFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						free(path);
						char daInviare[200]="";

						if(closeFileServerReturnValue != 1)
						{
							strncpy(daInviare,"CLOSE_FILE: riscontrato errore",32);
							strncpy(stringaToLog,"CLOSE_FILE: riscontrato errore",33);
							scriviSuLog(stringaToLog,0);

						}
						else
						{
							strncpy(daInviare,"CLOSE_FILE eseguita correttamente!\n",36);
							strncpy(stringaToLog,"CLOSE_FILE: eseguita operazione",32);
							scriviSuLog(stringaToLog,0);
						}
						size_t dimStringaDaInviare=strlen(daInviare)+1;
						inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);
						free(bufferRicezione);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
					}
					if(strcmp(bufferRicezione,"REMOVE_FILE")==0)
					{
						int removeFileServerReturnValue=0;
						char * path=NULL;

						size_t dimPath=0;
						riceviDati(fdDaElaborare,&path,&dimPath);

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
						size_t dimStringaDaInviare=strlen(daInviare);
						inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						free(path);
					}
					if(strcmp(bufferRicezione,"APPEND_TO_FILE")==0)
					{
						int appendToFileServerReturnValue=0;
						size_t dimPath=0;
						char * path=NULL;
						riceviDati(fdDaElaborare, &path, &dimPath);
						char *bufferRicezione=NULL;
						size_t dimBufferRicezione=0;
						riceviDati(fdDaElaborare, &bufferRicezione, &dimBufferRicezione);
						char * rest3=NULL;
						char * buffer=strtok_r(bufferRicezione, puntoVirgola, &rest3);
						char * size_array=strtok_r(NULL, puntoVirgola, &rest3);
						size_t size = atoi(size_array);

						accediStrutturaFile();
						appendToFileServerReturnValue=appendToFileServer(path,buffer,size,fdDaElaborare);
						lasciaStrutturaFile();
						char daInviare[200]="";
						if(appendToFileServerReturnValue != 1)
						{
							strncpy(daInviare,"APPEND_TO_FILE: riscontrato errore",36);
							strncpy(stringaToLog,"APPEND_TO_FILE: riscontrato errore",36);
							scriviSuLog(stringaToLog,0);

						}
						else
						{
							strncpy(daInviare,"APPEND_TO_FILE eseguita correttamente!\n",42);
							strncpy(stringaToLog,"APPEND_TO_FILE: eseguita operazione",38);
							scriviSuLog(stringaToLog,0);
						}


						size_t dimStringaDaInviare=strlen(daInviare);
						inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);





						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
					}

					if(strcmp(bufferRicezione,"READ_FILE")==0)
					{
						printf("Arrivata operazione READ_FILE\n");
						char * buffer2;
						char * path;
						path=strtok_r(NULL, puntoVirgola, &rest);

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
							strncpy(result,"errore",7);
							size_t a=strlen(result)+1;
							inviaDati(fdDaElaborare,&a,sizeof(size_t));
							inviaDati(fdDaElaborare,&result,a);
						}
						else
						{
							strncpy(stringaToLog,"READ_FILE_di_byte",32);
							scriviSuLog(stringaToLog,1,dimFile);

							inviaDati(fdDaElaborare,&dimFile,sizeof(size_t));
							inviaDati(fdDaElaborare,buffer2,dimFile);
						}

						free(buffer2);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);

					}
					if(strcmp(bufferRicezione,"READ_N_FILE")==0)
					{
						int N=0;
//						char* dirname;
//						char risposta[200]="";
//						int read_N_FileServerReturnValue=-1;
						size_t a=0;
						char * bufferRicevi=NULL;
						riceviDati(fdDaElaborare, &bufferRicevi, &a);

						N=atoi(bufferRicevi);


						if(N>numFilePresenti || N==0)
						{
							N=numFilePresenti;
						}
						printf("WORKER-> N= %d\n\n",N);

						int i;
						char * buffer2=NULL;
						size_t dimFile=0;
						size_t lungPath=0;
						char stringa[200]="";
						char *path=NULL;
//						int fileLettiEffettivi=0;
						accediStrutturaFile();

						//adesso nella variabile N ho il numero corretto di file che devo inviare al client,
						//invierò i primi N della struttura contenente i file
						for(i=0;i<N;i++)
						{
							if(array_file[i].identificatoreClient!=fdDaElaborare && array_file[i].O_LOCK!=1)
							{
								continue;
							}
							buffer2=malloc(sizeof(char)*array_file[i].dimensione);
							memcpy(buffer2,array_file[i].byteFile,array_file[i].dimensione);
							path=malloc(sizeof(char)*(strlen(array_file[i].path)+1));

							strncpy(path,array_file[i].path,strlen(array_file[i].path)+1);
							lungPath=strlen(array_file[i].path);
							inviaDati(fdDaElaborare,&lungPath,sizeof(size_t));
							inviaDati(fdDaElaborare,path,lungPath);


							dimFile=array_file[i].dimensione;
							inviaDati(fdDaElaborare,&dimFile,sizeof(size_t));
							inviaDati(fdDaElaborare,buffer2,dimFile);
							free(buffer2);
							free(path);
						}
						lasciaStrutturaFile();
						strncpy(stringa,"READ_N_FILE: eseguita operazione",34);
						size_t c=strlen(stringa);
						inviaDati(fdDaElaborare,&c,sizeof(size_t));
						inviaDati(fdDaElaborare,&stringa,c);





						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						free(bufferRicezione);

					}

					if(strcmp(bufferRicezione,"WRITE_FILE")==0)
					{
						printf("Arrivata operazione WRITE_FILE\n");
						//
						//
												int writeFileServerReturnValue=-1;
												char * path=NULL;
												size_t lunghezzaPath=0;
												riceviDati(fdDaElaborare, &path, &lunghezzaPath);

												if(path == NULL)
												{
													printf("errore!!!!!!");
													writeFileServerReturnValue=-1;
												}
												printf("il worker esegue write file sul file %s \n",path);

												size_t sizeFile=0;
												void * dati= NULL;
												riceviDati(fdDaElaborare, &dati, &sizeFile);
												accediStrutturaFile();
												writeFileServerReturnValue=writeFileServer(path,dati,sizeFile,fdDaElaborare);
												lasciaStrutturaFile();

												strncpy(stringaToLog,"WRITE_FILE_di_byte",32);
												scriviSuLog(stringaToLog,1,sizeFile);
												char daInviare[300]="";
												if(writeFileServerReturnValue == -1)
												{
													strncpy(daInviare,"WRITE_FILE: riscontrato errore",31);
													strncpy(stringaToLog,"WRITE_FILE: riscontrato errore",31);
													scriviSuLog(stringaToLog,0);
												}
												if(writeFileServerReturnValue == 1)
												{
													strncpy(daInviare,"WRITE_FILE eseguita correttamente!",35);
													strncpy(stringaToLog,"WRITE_FILE di byte",32);
													scriviSuLog(stringaToLog,1,sizeFile);
												}
												size_t a=strlen(daInviare)+1;
												inviaDati(fdDaElaborare,&a,sizeof(size_t));
												inviaDati(fdDaElaborare,&daInviare,a);


												strncpy(stringaToLog,"Richiesta Servita dal thread",32);
												scriviSuLog(stringaToLog,1,indiceWorker);
												free(bufferRicezione);
					}

			}

		}



			if(chiudiConnessione == 1)
			{
				int menoUno = -1;
				accediPipeWorker();
				HANDLE_WRNS(scriviNBytes(pipeGestioneWorkers[1], &menoUno, sizeof(menoUno)), sizeof(menoUno), NOOP,
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
				free(bufferRicezione);
			}
		}
	}
	pthread_exit(NULL);
}
