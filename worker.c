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

	//char *bufferRicezione=NULL;
//	bufferRicezione=malloc(100*sizeof(char));
//	strcpy(bufferRicezione,"ciaociaociaociaociaociaociaociaociaociao");
	//size_t daLeggere=0;
	workers[indiceWorker].threadId=pthread_self();
	char stringaToLog[100]="";
	int fdDaElaborare=0;
	int stoLavorando=0;
	while ((getSegnale() !=2 || (getSegnale() != 1 && getNumClient()>0) ))
	{
		accediCodaComandi();
		while((contatoreCodaFd == 0) && (broadcast==0))
		{
			printf("contatoreCodaFd:%d \nbroadcast:%d\n",contatoreCodaFd,broadcast);
			if(stoLavorando==1)
			{
				//tramite questo if sto verificando se, nel caso stessi elborando delle richieste
				//di un  mio client ma fossi in condizione di mettermi in wait, non aspetto perchè continuo
				//l' elaborazione di tale richiesta
				break;
			}
			printf("attendo\n");
			pthread_cond_wait(&(CVFileDescriptor), &(lockCodaComandi));
			printf("Svegliato!!\n");
		}
		lasciaCodaComandi();
		if(getSegnale() == 2 || (getSegnale() == 1 && getNumClient()==0))
		{
//			Termina la vita del thread
			printf("TERMINA THREAD!!!\n");
			pthread_exit(NULL);

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

				size_t readReturnValue=0;
				size_t operazione=0;
				//size_t ritorno=0;
				readReturnValue=readn(fdDaElaborare,&operazione,sizeof(operazione));
				//printf("Server->operazione:%ld\n",operazione);
				//printf("Server->ritorno:%ld\n",ritorno);

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
					perror("SERVER-> Errore: Il fileDescriptor si riferisce a un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK) e la lettura si bloccherebbe.\n");
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
					//const char puntoVirgola[2] = ";"
					//char *token=NULL;
					//char * rest=NULL;
//					token=strtok_r(bufferRicezione, puntoVirgola, &rest);
//					if( token != NULL )
					{
						if(operazione==100)
						{
							//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
							//i controlli sui parametri saranno effettuati da tale funzone
							int openFileServerReturnValue=0;
							int flag=0;


							size_t dimPath =0;
							char * path=NULL;

							int readReturnValue2=riceviDati(fdDaElaborare,&path,&dimPath);
//							//printf("path:%s\n",path);

							if(readReturnValue2<0)
							{
								perror("WORKER -> errore riceviDati\n");
							}
//							char *flag_array = NULL;

							//size_t dimFlagArray=0;
							readReturnValue2=readn(fdDaElaborare,&flag,sizeof(flag));
							if(readReturnValue2<0)
							{
								perror("WORKER -> errore riceviDati\n");
							}

//							flag=atoi(flag_array);

//							//printf("path:%s\nflag:%ld\n",path,flag);
							accediStrutturaFile();
							printf("BBB\n");
							openFileServerReturnValue=openFileServer(path,flag,fdDaElaborare);
							lasciaStrutturaFile();

							free(path);
//							free(flag_array);
							char  daInviare[200]="";
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
							//free(bufferRicezione);
						}
					}
					if(operazione==102)
					{
						//sistemo tutti i dati ricevuti e li invio alla funzione dedicata
						//i controlli sui parametri saranno effettuati da tale funzone
						int lockFileServerReturnValue=0;
						char * path=NULL;
						size_t dimPath=0;
						//size_t operazione=0;
//						readn(fdDaElaborare,operazione,sizeof(operazione));
						riceviDati(fdDaElaborare,&path,&dimPath);
						accediStrutturaFile();
						lockFileServerReturnValue=lockFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						char  daInviare[200]="";

						if(lockFileServerReturnValue != 1)
						{

							strncpy(daInviare,"LOCK_FILE: riscontrato errore\n",31);
//							strncpy(stringaToLog,"LOCK_FILE: riscontrato errore",32);
//							scriviSuLog(stringaToLog,0);
						}
						else
						{
							strncpy(daInviare,"LOCK_FILE eseguita correttamente!\n",35);
//							strncpy(stringaToLog,"LOCK_FILE: eseguita operazione",31);
//							scriviSuLog(stringaToLog,0);

						}

						free(path);

						size_t dimStringaDaInviare=strlen(daInviare);
						inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						//free(bufferRicezione);
					}
					if(operazione==103)
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
						size_t dimStringaDaInviare=strlen(daInviare);
						inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						free(path);
						//free(bufferRicezione);
					}
					if(operazione==101)
					{
						int closeFileServerReturnValue=0;
						size_t dimPath=0;
						char * path=NULL;
						riceviDati(fdDaElaborare,&path,&dimPath);
						//printf("path ricevuto:%s\n",path);
						accediStrutturaFile();
						closeFileServerReturnValue=closeFileServer(path,fdDaElaborare);
						lasciaStrutturaFile();

						free(path);
						char daInviare[200]="";

						if(closeFileServerReturnValue != 1)
						{
							strncpy(daInviare,"CLOSE_FILE: riscontrato errore",32);
							strncpy(stringaToLog,"CLOSE_FILE: riscontrato errore",31);
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
//						//free(bufferRicezione);
//						visualizzaArrayFile();
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
					}
					if(operazione==108)
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
						//free(bufferRicezione);
					}
					if(operazione == 107)
					{
						int appendToFileServerReturnValue=0;
						size_t dimPath=0;
						char * path=NULL;
						char *buffer=NULL;
						//size_t size=0;
						riceviDati(fdDaElaborare, &path, &dimPath);
						//char *bufferRicezione=NULL;
						size_t sizeBytes=0;
						riceviDati(fdDaElaborare, &buffer, &sizeBytes);

//						//printf("\n\n\n\nbuffer:%s\n\n\n\n",buffer);
//						readn(fdDaElaborare,&size,sizeof(size));
//						riceviDati(fdDaElaborare, &bufferRicezione, &dimBufferRicezione);
//						char * rest3=NULL;
//						char * buffer=strtok_r(bufferRicezione, puntoVirgola, &rest3);
//						char * size_array=strtok_r(NULL, puntoVirgola, &rest3);
//						size_t size = atoi(size_array);

//						accediStrutturaFile();
//						//printf("Server:\n");
						//printf("path:%s\nbuffer:%s\nsizeBytes:%ld",path,buffer,sizeBytes);
//						//printf("buffer:%s\n",buffer);
						accediStrutturaFile();
						appendToFileServerReturnValue=appendToFileServer(path,buffer,sizeBytes,fdDaElaborare);
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
							strncpy(daInviare,"APPEND_TO_FILE eseguita correttamente!",41);
							strncpy(stringaToLog,"APPEND_TO_FILE: eseguita operazione",38);
							scriviSuLog(stringaToLog,0);
						}
						free(path);
						size_t dimStringaDaInviare=strlen(daInviare);
						inviaDati(fdDaElaborare,&dimStringaDaInviare,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,dimStringaDaInviare);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						free(buffer);
					}

					if(operazione == 104)
					{
						//printf("Arrivata operazione READ_FILE\n");
						char * buffer2=NULL;
						char * path=NULL;
						size_t dimPath=0;
						riceviDati(fdDaElaborare,&path,&dimPath);
						size_t dimFile=0;
						char result[200]="";
						int indice=-1;
						accediStrutturaFile();
						indice=readFileServer(path,buffer2,&dimFile,fdDaElaborare);
						lasciaStrutturaFile();
						if(indice!=-1)
						{
							buffer2=malloc(sizeof(char)*array_file[indice].dimensione);
							memcpy(buffer2,array_file[indice].byteFile,array_file[indice].dimensione);
						}
						if(indice==-1)
						{
							//printf("mando errore");
							strncpy(result,"errore",7);
							size_t a=strlen(result)+1;
							inviaDati(fdDaElaborare,&a,sizeof(size_t));
							inviaDati(fdDaElaborare,&result,a);
						}
						else
						{
							dimFile-=1;
							strncpy(stringaToLog,"READ_FILE_di_byte",32);
							scriviSuLog(stringaToLog,1,dimFile);

							inviaDati(fdDaElaborare,&dimFile,sizeof(size_t));
							inviaDati(fdDaElaborare,buffer2,dimFile);
						}
						free(path);
						free(buffer2);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);


					}
					if(operazione == 105)
					{
						int N=0;
						//size_t a=0;
						//char * bufferRicevi=NULL;
//						riceviDati(fdDaElaborare, &bufferRicevi, &a);
						readn(fdDaElaborare,&N,sizeof(N));
//						N=atoi(bufferRicevi);
						int i;
						char * buffer2=NULL;
						size_t dimFile=0;
						size_t lungPath=0;
						char stringa[200]="";
						char *path=NULL;
						printf("READNFILE ha letto N:%d\n",N);
						accediStrutturaFile();

						if(N>numFilePresenti || N==0 || N<0)
						{
							N=numFilePresenti;
						}
//						//adesso nella variabile N ho il numero corretto di file che devo inviare al client,
//						//invierò i primi N della struttura contenente i file
						for(i=0;i<N;i++)
						{
//							if(array_file[i].identificatoreClient!=fdDaElaborare && array_file[i].O_LOCK!=1)
//							{
//								continue;
//							}
							if(array_file[i].O_CREATE!=1)
							{
								continue;
							}
							buffer2=malloc(sizeof(char)*array_file[i].dimensione);
							memcpy(buffer2,array_file[i].byteFile,array_file[i].dimensione);
							path=malloc(sizeof(char)*(strlen(array_file[i].path)+1));

							strncpy(path,array_file[i].path,strlen(array_file[i].path)+1);
							lungPath=strlen(array_file[i].path)+1;
							inviaDati(fdDaElaborare,&lungPath,sizeof(size_t));
							inviaDati(fdDaElaborare,path,lungPath);

							dimFile=array_file[i].dimensione;
							inviaDati(fdDaElaborare,&dimFile,sizeof(size_t));
							inviaDati(fdDaElaborare,buffer2,dimFile);
							strncpy(stringaToLog,"READ_FILE_di_byte",32);
							scriviSuLog(stringaToLog,1,dimFile);
							free(buffer2);
							free(path);
						}
						lasciaStrutturaFile();
						strncpy(stringa,"READ_N_FILE: eseguita operazione",34);
						size_t c=strlen(stringa)+1;
						inviaDati(fdDaElaborare,&c,sizeof(size_t));
						inviaDati(fdDaElaborare,stringa,c);





						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						//free(bufferRicezione);

					}

					if(operazione == 106)
					{

						int writeFileServerReturnValue=-1;
						char * path=NULL;
						size_t lunghezzaPath=0;
						riceviDati(fdDaElaborare, &path, &lunghezzaPath);

						if(path == NULL)
						{
							perror("WORKER -> errore riceviDati");
							writeFileServerReturnValue=-1;
						}

						size_t sizeFile=0;
						char * dati= NULL;
						riceviDati(fdDaElaborare, &dati, &sizeFile);
//						//printf("SERVER-> dati:%s\n",dati);
						accediStrutturaFile();
						writeFileServerReturnValue=writeFileServer(path,dati,sizeFile,fdDaElaborare);
						lasciaStrutturaFile();


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
//							strncpy(stringaToLog,"WRITE_FILE di byte",32);
							sizeFile-=1;
							scriviSuLog(stringaToLog,1,sizeFile);
							strncpy(stringaToLog,"WRITE_FILE_di_byte",32);
							scriviSuLog(stringaToLog,1,sizeFile);
						}
						size_t a=strlen(daInviare)+1;
						inviaDati(fdDaElaborare,&a,sizeof(size_t));
						inviaDati(fdDaElaborare,&daInviare,a);

						free(dati);
						free(path);
						strncpy(stringaToLog,"Richiesta Servita dal thread",32);
						scriviSuLog(stringaToLog,1,indiceWorker);
						//free(bufferRicezione);
					}
				}
			}

			if(chiudiConnessione == 1)
			{
				if(fdDaElaborare!=-1)
				{
					close(fdDaElaborare);
					strncpy(stringaToLog,"CLOSE: eseguita operazione",27);
					scriviSuLog(stringaToLog, 0);
					decrementaNumClient();

					strncpy(stringaToLog,"Un client si è disconnesso, adesso il totale ammonta a",150);
					scriviSuLog(stringaToLog,1,getNumClient());
				}

				//operazioneEseguita=0;
				stoLavorando=0;
//				continue;
				//free(bufferRicezione);
			}
		}
	}
	printf("thread concluso!\n");
}
