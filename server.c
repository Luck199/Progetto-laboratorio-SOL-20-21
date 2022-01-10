//Created on: 23 set 2021

#include "comunicazioneClientServer.h"
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
#include <sys/stat.h>
#include "worker.h"
#include "coda.h"
#include "utility.h"
#include "apiServer.h"
#include "gestioneFile.h"

#define E_SQ_MALLOC 1
#define E_SQ_MUTEX_LOCK 2
#define E_SQ_GENERAL 3
#define E_SQ_EMPTY_QUEUE 4
#define E_SQ_MUTEX_COND 5
#define E_SQ_QUEUE_DELETED 6
#define E_SQ_NO_QUEUE 7
#define E_SQ_NULL_ELEMENT 8


#define MAXPIPE 2
#define MAXLUNGHEZZA 150
#define namesize 32
#define SOCKNAME "./sockfile"
#define UNIX_PATH_MAX 108
#define N 100



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


#define AAIN(code, message) \
  if (code == NULL)         \
  {                         \
    perror(message);        \
    exit(EXIT_FAILURE);     \
  }

#define HUP 1
#define QUIT_INT 2


void inizializzaScritturaLog(char *nomeLogFile);
void creaCreatoreThreadWorkers();
void* creaThreadWorkers();
void creaWorkers(int idWorker);
static void *gestoreSegnali(void *arg);
static void createDetachSigHandlerThread(int pipe);
static void maskSIGPIPEAndHandledSignals();


pthread_cond_t endServer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lockEndServer = PTHREAD_MUTEX_INITIALIZER;

pthread_t tidWorker;
pthread_t threadGestoreSegnali;
int sigalarm_flag = 0;
sigset_t set;
int segnale=0;
pthread_t tidCreatoreWorkers;
struct sockaddr_un sa;

// ABORT_ABRUPTLY_IF_NON_ZERO
#define error_handling(codice, messaggio) \
  if (codice != 0)             \
  {                          \
    perror(messaggio);         \
    exit(EXIT_FAILURE);      \
  }

int fd_skt;

int main(int argc, char **argv)
{
	int i;
	int primaVolta=0;
	int segnale = 0;
	int errore = 0;
	int pipeGestioneSegnali[MAXPIPE];
	char nomeLogFile[namesize];

		codaFileDescriptor = malloc(sizeof(struct codaInteri));

	codaFileDescriptor->fileDescriptor=0;
	codaFileDescriptor->next=NULL;
	codaFileDescriptor->prec=NULL;



	//stringa che verrà utilizzata per scrivere sul file di log in mutua esclusione
	char stringaToLog[MAXLUNGHEZZA];



	//masterWorkersPipe è una pipe che utilizzano thread main e thread worker per comunicare:
	//tramite la cella in posizione 0 il thread main legge quali fileDescriptor sono pronti per essere impostati nell' insieme fd_set
	//su 1 i thread workers scrivono tali fileDescriptor
	//int pipeGestioneWorkers[MAXPIPE];


	int bindReturnValue;
	int listenReturnValue;
	int fd_hwm = 0,fd;
	int acceptReturnValue;
	int selectReturnValue;
	//int totaleClient=0;
	int closeReturnValue=0;
	int joinReturnValue=0;




	//leggo i parametri dal file di configurazione
	letturaFile(argv[1], nomeLogFile);



	//Alloco e inizializzo l' array dinamico che utilizzerò per gestire i file.
	//array di struct
	allocaStrutturaFile();







	//preparo il descrittore del file di log per dare la possibilità di scriverci sopra
	inizializzaScritturaLog(nomeLogFile);

	//configuro la gestione dei segnali
	error_handling(pipe(pipeGestioneSegnali), "SERVER-> la creazione della pipe per i segnali ha riscontrato un errore\n");
	strncpy(stringaToLog,"pipe per gestione dei segnali creata correttamente.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog, 0);




	error_handling(pipe(pipeGestioneWorkers), "SERVER-> la creazione della pipe per comunicare con i thread worker ha riscontrato un errore\n");
	strncpy(stringaToLog,"pipe per la gestione dei thread workers creata correttamente.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog, 0);



	//Mando in esecuzione il thread che si occuperà della gestione dei segnali
	createDetachSigHandlerThread(pipeGestioneSegnali[1]);


	maskSIGPIPEAndHandledSignals();
	strncpy(stringaToLog,"thread che si occuperà di gestire i segnali creato in maniera corretta.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog, 0);

	inizializza_worker();


	creaCreatoreThreadWorkers();
	strncpy(stringaToLog,"La creazione dei thread workers è riuscita correttamente",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,0);

	//inizializzo il server e controllo eventuali errori nella creazione del socket TCP
	struct sockaddr_un sa;
	//(void)unlink(name_socket);

	strcpy(sa.sun_path, name_socket);
	sa.sun_family = AF_UNIX;
	fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd_skt == -1 && errno == EINTR)
	{
		perror("SERVER-> la funzione per la creazione del socket è stata interrotta a causa di un segnale");
		strncpy(stringaToLog,"creazione socket interrotta da un segnale.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	   	exit(EXIT_FAILURE);
	}
	else if (fd_skt == -1)
	{
		perror("SERVER-> la funzione per la creazione del socket ha riscontrato un errore\n");
		strncpy(stringaToLog,"riscontrato problema nella creazione del socket.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	    exit(EXIT_FAILURE);
	}
	strncpy(stringaToLog,"socket creato correttamente.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog, 0);

	//Eseguo l' operazione di bind per il mio socket e controllo eventuali errori
	bindReturnValue = bind(fd_skt, (struct sockaddr *)&sa, sizeof(sa));
	if (bindReturnValue == -1 && errno == EINTR)
	{
		perror("SERVER-> la funzione bind è stata interrotta dall' arrivo di un segnale\n");
		strncpy(stringaToLog,"Funzione bind interrotta dall' arrivo di un segnale",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	    exit(EXIT_FAILURE);
	}
	else if (bindReturnValue == -1)
	{
		perror("SERVER-> un errore è stato riscontrato dalla funzione bind\n");
		strncpy(stringaToLog,"La funzione bind ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	    exit(EXIT_FAILURE);
	}
	strncpy(stringaToLog,"Funzione bind eseguita in maniera corretta.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog, 0);

	//Mi metto in ascolto di nuove connessioni, gestendo poi eventuali errori
	listenReturnValue = listen(fd_skt, SOMAXCONN);
	if (listenReturnValue == -1 && errno == EINTR)
	{
		perror("SERVER-> la funzione listen è stata interrotta dall' arrivo di un segnale\n");
		strncpy(stringaToLog,"Funzione listen interrotta dall' arrivo di un segnale.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	    exit(EXIT_FAILURE);
	}
	else if (listenReturnValue == -1)
	{
	    perror("SERVER-> un errore è stato riscontrato dalla funzione bind\n");
		strncpy(stringaToLog,"La funzione listen ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	    exit(EXIT_FAILURE);
	}
	strncpy(stringaToLog,"Funzione listen eseguita in maniera corretta.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog, 0);
	fd_hwm = 0;
	fd_set set, read_set;

	// determine the higher fd for now
	if (fd_skt > fd_hwm)
	{
		fd_hwm = fd_skt;
	}
	if (pipeGestioneWorkers[0] > fd_hwm)
	{
		accediPipeWorker();
		fd_hwm = pipeGestioneWorkers[0];
		lasciaPipeWorker();
	}
	if (pipeGestioneSegnali[0] > fd_hwm)
	{
		fd_hwm = pipeGestioneSegnali[0];
	}

	FD_ZERO(&set);
	FD_SET(fd_skt, &set);
	accediPipeWorker();
	FD_SET(pipeGestioneWorkers[0], &set);
	lasciaPipeWorker();
	FD_SET(pipeGestioneSegnali[0], &set);



	//ciclo dove verranno gestite le connessioni con i vari client.
	//Verrà interrotto solo all' arrivo di un segnale
	while (segnale != QUIT_INT && (segnale != HUP || getNumClient() > 0))
	{
		read_set = set;
		selectReturnValue = select(fd_hwm + 1, &read_set, NULL, NULL, NULL);
		if (selectReturnValue == -1 && errno == EINTR)
		{
			perror("SERVER-> la funzione select è stata interrotta dall' arrivo di un segnale\n");
			strncpy(stringaToLog,"Funzione select interrotta dall' arrivo di un segnale.",MAXLUNGHEZZA);
			scriviSuLog(stringaToLog, 0);
			exit(EXIT_FAILURE);
		}
		else if (selectReturnValue == -1)
		{
			perror("SERVER-> un errore è stato riscontrato dalla funzione select\n");
			strncpy(stringaToLog,"La funzione select ha riscontrato un errore.",MAXLUNGHEZZA);
			scriviSuLog(stringaToLog, 0);
			exit(EXIT_FAILURE);
		}
		else
		{
			strncpy(stringaToLog,"Funzione select eseguita in maniera corretta",MAXLUNGHEZZA);
			scriviSuLog(stringaToLog, 0);
			for (fd = 0; fd <= fd_hwm && segnale != QUIT_INT && (segnale != HUP || getNumClient() > 0); fd++)
			{
				if (FD_ISSET(fd, &read_set))
				{
					if (fd == fd_skt && segnale == 0)
					{
						//Entro nel ramo if se non sono presenti segnali

						//Operazione di accettazione di un nuovo client
						acceptReturnValue = accept(fd_skt, NULL, 0);

						if (acceptReturnValue == -1 && errno == EINTR)
						{
							perror("SERVER-> la funzione accept è stata interrotta dall' arrivo di un segnale\n");
							strncpy(stringaToLog,"Funzione accept interrotta dall' arrivo di un segnale.",MAXLUNGHEZZA);
							scriviSuLog(stringaToLog, 0);
							exit(EXIT_FAILURE);
						}
						else if (acceptReturnValue == -1)
						{
							perror("SERVER-> un errore è stato riscontrato dalla funzione accept\n");
							strncpy(stringaToLog,"La funzione accept ha riscontrato un errore.",MAXLUNGHEZZA);
							scriviSuLog(stringaToLog, 0);
							exit(EXIT_FAILURE);
						}
						strncpy(stringaToLog,"Funzione accept eseguita in maniera corretta.",MAXLUNGHEZZA);
						scriviSuLog(stringaToLog, 0);
						//un nuovo client si è connesso, aumento il numero dei client connessi
						incrementaNumClient();

						int totClienti=getNumClient();
						strncpy(stringaToLog,"Si è connesso un nuovo client, adesso il totale ammonta a",MAXLUNGHEZZA);
						scriviSuLog(stringaToLog,1,totClienti);

						//gestione delle connessioni
						FD_SET(acceptReturnValue, &set);
						if (acceptReturnValue > fd_hwm)
						{
							fd_hwm = acceptReturnValue;
						}
						char  daInviare[200]="";
						strncpy(daInviare,"connessione eseguita correttamente!\n",37);
						size_t a=strlen(daInviare);
						inviaDati(acceptReturnValue,&a,sizeof(size_t));
						inviaDati(acceptReturnValue,&daInviare,a);
					}
					else if (fd == pipeGestioneWorkers[0])
					{
						//Un client è stato servito: recupero il suo indice

						int fd;

						accediPipeWorker();
						HANDLE_WRNS(leggiNBytes(pipeGestioneWorkers[0], &fd, sizeof(fd)), sizeof(fd), ;, errore = 1;);
						lasciaPipeWorker();
						if (errore)
						{
							perror("SERVER-> Errore nella lettura dalla pipe per gestione dei worker\n");
							strncpy(stringaToLog,"Errore riscontrato nella lettura dalla pipe per la gestione dei workers.",MAXLUNGHEZZA);
							scriviSuLog(stringaToLog, 0);
							exit(EXIT_FAILURE);
						}
						else
						{
							strncpy(stringaToLog,"Lettura dalla pipe per la gestione dei workers avvenuta in maniera corretta.",MAXLUNGHEZZA);
							scriviSuLog(stringaToLog, 0);
							if (fd == -1)
							{
								//un client si è disconnesso, quindi decremento il numero di quelli connessi
		    						//clientConnessi--;
//		    						strncpy(stringaToLog,"Un client si è disconnesso, adesso il totale ammonta a",MAXLUNGHEZZA);
//		    						scriviSuLog(stringaToLog,1,clientConnessi);
							}
							else
							{
								FD_SET(fd, &set);
								if (fd > fd_hwm)
								{
									fd_hwm = fd;
								}
							}
						}
					}
					else if (fd == pipeGestioneSegnali[0])
					{
						//è arrivato un segnale, recupero il suo indice dalla pipe dedicata
						HANDLE_WRNS(leggiNBytes(pipeGestioneSegnali[0], &segnale, sizeof(segnale));, sizeof(segnale), ;, errore = 1;);

						if (errore != 0)
						{
							perror("SERVER-> Errore nella lettura della pipe utilizzata per la gestione dei segnali\n");
							strncpy(stringaToLog,"Errore riscontrato nella lettura dalla pipe per la gestione dei segnali.",MAXLUNGHEZZA);
							scriviSuLog(stringaToLog, 0);
							exit(EXIT_FAILURE);
						}
						strncpy(stringaToLog,"Lettura dalla pipe per la gestione dei segnali avvenuta in maniera corretta.",MAXLUNGHEZZA);
						scriviSuLog(stringaToLog, 0);
					}
					else
					{
						//Nel caso in cui un client gia connesso abbia richiesto un'operazione,
						//invio quest' ultima ai thread workers per essere gestita, e  rimuovo il file descriptor dall' insieme
						strncpy(stringaToLog,"Ricevuta una richiesta dal client",MAXLUNGHEZZA);
						scriviSuLog(stringaToLog,1,fd);

						FD_CLR(fd, &set);
						if (fd == fd_hwm)
						{
							fd_hwm--;
						}

						// control flow flags
						int shouldClose = 0;
						int shouldExit = 0;

						int *fileDescriptorPointer = malloc(sizeof(*fileDescriptorPointer));
						if (fileDescriptorPointer == NULL)
						{
							shouldClose = 1;
						}
						else
						{
							//invio il fd ai thread workers, con l' ausilio della pipe dedicata
							*fileDescriptorPointer = fd;

							//diviso in due volte per via del primo inserimento
							if(primaVolta==0)
							{
								accediCodaComandi();
								//printf("prima volta che inserisco fd\n");
								codaFileDescriptor->fileDescriptor=fd;
								contatoreCodaFd++;
								pthread_cond_signal(&CVFileDescriptor);

								//printf("numero fd presenti in coda: %d\n",contatoreCodaFd);
								//StampaLista_Interi(codaFileDescriptor);
								primaVolta=1;
								lasciaCodaComandi();
							}
							else
							{
								//printf("seconda volta\n");
								enqueueCodaFileDescriptor(codaFileDescriptor, fd);
							}
							if (errore == E_SQ_MUTEX_COND || errore == E_SQ_MUTEX_LOCK)
							{
								perror("Mutex error");
								shouldClose = 1;
								shouldExit = 1;
							}
							else if (errore != 0)
							{
								shouldClose = 1;
							}
						}

						if (shouldClose)
						{
							close(fd);
						}

						if (shouldExit)
						{
							exit(EXIT_FAILURE);
						}
						free(fileDescriptorPointer);
					}
				}
			}
		}
	}

	accediCodaComandi();
	pthread_cond_broadcast(&(CVFileDescriptor));
	printf("fatta broadcast!\n");
	broadcast=1;
	lasciaCodaComandi();

	//gestione worker all' arrivo di un segnale di terminazione immediata
	int fdNew=0;
	int guasto=0;
	if(segnale == QUIT_INT)
	{
		//////printf("Segnale di quit o di int\n");
//		accediCodaComandi();
		while(getNumClient()>0)
		{
			fdNew=dequeueCodaFileDescriptor(codaFileDescriptor,&guasto);
			if(fdNew != -1)
			{
				int closeReturnValue = close(fdNew);
				if ((closeReturnValue == -1) && (errno == EBADF))
				{
					perror("SERVER-> Errore: file descriptor aperto non valido\n");
				}

				if ((closeReturnValue == -1) && (errno == EINTR))
				{
					perror("SERVER-> Errore: close interrotta da un segnale\n");
				}
				if ((closeReturnValue == -1) && (errno == EIO))
				{
					perror("SERVER-> Errore input/output");
				}
				if ((closeReturnValue == -1) && ((errno == ENOSPC) || (errno ==EDQUOT)))
				{
					perror("SERVER-> Errore: scrittura in spazio di archiviazione non disponibile!\n");
				}

				decrementaNumClient();
				strncpy(stringaToLog,"Un client si è disconnesso, adesso il totale ammonta a",MAXLUNGHEZZA);
				scriviSuLog(stringaToLog,1,clientConnessi);
			}

		}
//		lasciaCodaComandi();
	}

	//gestione worker all' arrivo di un segnale di terminazione leggera

//	if(segnale == HUP)
//	{
//
//		//accediCodaComandi();
//		//attesa fino a  che i thread non hanno terminato
//		pthread_mutex_lock(&lockClientConnessi);
//		while(getNumClient() > 0)
//		{
//			printf("\n\ncucù\n\n\n");
//
//			printf("ASPETTO CHE I CLIENT SI TOLGANO DI CULO\n");
//			pthread_cond_wait(&allClientExitCond, &lockClientConnessi);
//			printf("CLIENT ASPETTATI\n");
//		}
//		pthread_mutex_unlock(&lockClientConnessi);
//
//	}
	for (i=0;i<thread_workers;i++)
	{
		printf("SERVER-> Attendo il worker {%d}\n", workers[i].id_worker);
		joinReturnValue=pthread_join(workers[i].threadId, NULL);
		if(joinReturnValue != 0)
		{
			strncpy(stringaToLog,"La funzione pthread_join per l' attesa dei thread workers ha riscontrato un errore.",MAXLUNGHEZZA);
			scriviSuLog(stringaToLog, 0);
		}
		printf("SERVER-> Atteso il worker {%d} \n", workers[i].id_worker);
	}



//	joinReturnValue=pthread_join(tidWorker, NULL);
//	if(joinReturnValue != 0)
//	{
//		strncpy(stringaToLog,"La funzione pthread_join per l' attesa del thread worker ha riscontrato un errore.",MAXLUNGHEZZA);
//		scriviSuLog(stringaToLog, 0);
//	}


	joinReturnValue=pthread_join(tidCreatoreWorkers,NULL);
	if(joinReturnValue != 0)
	{
		strncpy(stringaToLog,"La funzione pthread_join per l' attesa del thread creatore dei workers ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	}

	joinReturnValue=pthread_join(threadGestoreSegnali,NULL);
	if(joinReturnValue != 0)
	{
		strncpy(stringaToLog,"La funzione pthread_join per l' attesa del thread per la gestione dei segnali ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	}

	strncpy(stringaToLog,"Thread attesi correttamente tramite funzione pthread_join.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog, 0);
	free(workers);

	closeReturnValue = close(fd_skt);
	if((closeReturnValue == -1) && (errno == EINTR))
	{
		strncpy(stringaToLog,"La funzione di chiusura del socket ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,0);
		perror("La funzione di chiusura del socket ha riscontrato un errore\n");
	    return -1;
	}
	strncpy(stringaToLog,"Funzione di chiusura del socket riuscita correttamente.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,0);


	accediCodaComandi();
	free(codaFileDescriptor);
	lasciaCodaComandi();
	short verificaSeStrutturaVuota=0;
	printf("Stampe finali:\n");
	printf("-> Numero di file massimo memorizzato nel server: %d\n",numMaxFilePresenti);
	printf("-> Dimensione massima in Mbytes raggiunta dal file storage: %d\n",(int)maxMemoriaRaggiunta);
	printf("-> Numero di volte in cui l' algoritmo di rimpiazzamento della cache è stato eseguito per selezionare uno o più file \"vittima\": %d\n",numVolteAlgoritmoRimpiazzo);
	printf("-> lista dei file contenuti nello storage al momento della chiusura del server:\n");
	for(i=0;i<num_max_file;i++)
	{
		if(strcmp(array_file[i].path,"vuoto")!=0)
		{
			verificaSeStrutturaVuota=1;
			printf("\t-> %s\n",array_file[i].path);
		}
	}
	if(verificaSeStrutturaVuota==0)
	{
		printf("\t-> non sono presenti file nella struttura dati\n");
	}


	strncpy(stringaToLog,"maxMemoriaRaggiunta",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,1,(int)maxMemoriaRaggiunta);


	strncpy(stringaToLog,"NumRimpiazzi",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,1,numVolteAlgoritmoRimpiazzo);

	strncpy(stringaToLog,"NumMaxConnessioniContemporanee",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,1,numMaxconnessioniContemporanee);



	fclose(logFile);
	deallocaStrutturaFile();

	printf("SERVER-> finito Server, i client connessi risultano: %d\n",clientConnessi);
	printf("Sono passati dal server: %d client\n",clientTotali);
	return 0;
}


void inizializzaScritturaLog(char *nomeLogFile)
{
	char stringaToLog[MAXLUNGHEZZA];
	if((logFile = fopen(nomeLogFile, "w")) == NULL)
	{
		perror("SERVER-> Error fopen");
//		strncpy(stringaToLog,"La funzione fopen per file di log ha riscontrato un errore.",MAXLUNGHEZZA);
//		scriviSuLog(stringaToLog,0);
		exit(EXIT_FAILURE);
	}
	strncpy(stringaToLog,"Funzione fopen per file di log riuscita correttamente.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,0);
}



static void *gestoreSegnali(void *argument)
{
	char stringaToLog[MAXLUNGHEZZA];
	sigset_t pset;
	int *pipePtr = argument;
	int pipe = *pipePtr;
	int numeroSegnale;
	int tipoSegnale = 0;
	free(pipePtr);

	//azzero la maschera puntata da pset
	error_handling(sigemptyset(&pset), "SERVER-> errore nell' azzerare la maschera puntata da pset")

	//metto a 1 la posizione del segnale indicato come secondo parametro nella maschera pset
	error_handling(sigaddset(&pset, SIGINT), "SERVER-> addset SIGINT: operazione fallita")
	error_handling(sigaddset(&pset, SIGQUIT), "SERVER-> addset SIGQUIT: operazione fallita")
	error_handling(sigaddset(&pset, SIGHUP), "SERVER-> addset SIGHUP: operazione fallita")
	error_handling(sigaddset(&pset, SIGTSTP), "SERVER-> addset SIGTSTP: operazione fallita")
	error_handling(sigaddset(&pset, SIGTERM), "SERVER-> addset SIGTERM: operazione fallita")

	//applico la maschero pset
	error_handling(pthread_sigmask(SIG_SETMASK, &pset, NULL), "SERVER-> il settaggio della nuova maschera dei segnali ha riscontrato un errore\n")



	//Mi metto in attesa dell' arrivo di un segnale
	error_handling(sigwait(&pset, &numeroSegnale), "SERVER-> suspension of execution of the signal handler thread has failed")

	if (numeroSegnale == SIGTSTP || numeroSegnale == SIGTERM)
	{
		//////printf("SERVER-> arresto in corso\n");
		exit(EXIT_FAILURE);
	}
	else if (numeroSegnale == SIGQUIT || numeroSegnale == SIGINT)
	{
		printf("SERVER-> Arrivato SIGQUIT o SIGINT\n");
		strncpy(stringaToLog,"Arrivato segnale SIGQUIT o segnale SIGINT",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,0);
		tipoSegnale = QUIT_INT;
		accediSegnali();
		segnale_globale=2;
		lasciaSegnali();
		segnaleChiusuraHup = QUIT_INT;
	}
	else if (numeroSegnale == SIGHUP)
	{
		printf("SERVER-> arrivato SIGHUP\n");
		strncpy(stringaToLog,"Arrivato segnale SIGHUP",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,0);
		tipoSegnale = 1;
		accediSegnali();
		segnale_globale=1;
		lasciaSegnali();
		segnaleChiusuraHup = HUP;
	}
	else
	{
		////printf("SERVER-> segnale di indice %d non gestito!\n", numeroSegnale);
		strncpy(stringaToLog,"Arrivato segnale non gestito, il cui indice è",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,1,numeroSegnale);
		exit(EXIT_FAILURE);
	}

	if (tipoSegnale)
	{
		HANDLE_WRNS(scriviNBytes(pipe, &tipoSegnale, sizeof(tipoSegnale)), sizeof(tipoSegnale), NOOP, perror("invio dati al thread main fallito");)
	}

	pthread_exit(NULL);
}



static void createDetachSigHandlerThread(int pipe)
{
	int *puntatorePipe = malloc(sizeof(*puntatorePipe));
	AAIN(puntatorePipe, "SERVER-> la creazione dell' thread per la gestione dei segnali ha riscontrato un errore\n")
	*puntatorePipe = pipe;
	error_handling(pthread_create(&threadGestoreSegnali, NULL, gestoreSegnali, puntatorePipe), "SERVER-> la creazione dell' thread per la gestione dei segnali ha riscontrato un errore\n")
}


static void maskSIGPIPEAndHandledSignals()
{
	struct sigaction sga;

	//Maschero tutti i segnali
	sigset_t pset;
	error_handling(sigfillset(&pset), "SERVER-> il settaggio dei segnali ha risocntrato un errore\n");
	error_handling(pthread_sigmask(SIG_SETMASK, &pset, NULL), "SERVER-> il settaggio della nuova maschera dei segnali ha riscontrato un errore\n")

	//Ignoro il segnale SIGPIPE
	memset(&sga, 0, sizeof(sga));
	sga.sa_handler = SIG_IGN;
	error_handling(sigaction(SIGPIPE, &sga, NULL), "SERVER-> il mascheramento del segnale SIGPIPE ha risocntrato un errore\n")

	// mask the signals handled by the dedicated signal handler thread
	error_handling(sigemptyset(&pset), "SERVER-> set of the POSIX signal set has failed");
	error_handling(sigaddset(&pset, SIGINT), "SERVER-> addset SIGINT: operazione fallita")
	error_handling(sigaddset(&pset, SIGQUIT), "SERVER-> addset SIGQUIT: operazione fallita")
	error_handling(sigaddset(&pset, SIGTSTP), "SERVER-> addset SIGTSTP: operazione fallita")
	error_handling(sigaddset(&pset, SIGTERM), "SERVER-> addset SIGTERM: operazione fallita")
	error_handling(sigaddset(&pset, SIGHUP), "SERVER-> addset SIGHUP: operazione fallita")

	//ristabilisco gli altri segnali
	pthread_sigmask(SIG_SETMASK, &pset, NULL);
}


void creaWorkers(int idWorker)
{
	char stringaToLog[MAXLUNGHEZZA];
	////printf("SERVER-> crea workers\n");
	if((pthread_create(&tidWorker, NULL, &vitaWorker, (void*)(intptr_t)idWorker))!=0)
	{
		perror("SERVER-> Errore nella funzione pthread_create\n");
		strncpy(stringaToLog,"La creazione di un thread worker ha riscontrato un problema",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,0);
		exit(EXIT_FAILURE);
	}
	else
	{

	}
}



//funzione che si occupa della creazione di tutti i thread worker, grazie all' utilizzo della funzione creaWorkers
void* creaThreadWorkers()
{
	int i;
	////printf("SERVER-> crea thread workers\n");
	for(i=0;i<thread_workers; i++)
	{
		creaWorkers(i);
	}
	pthread_exit(NULL);
}


//funzione che si occupa di creare il thread creatore che gestirà la creazione e la terminazione dei vari thread worker
void creaCreatoreThreadWorkers()
{
	int err;
	char stringaToLog[MAXLUNGHEZZA];
	if ( (err=pthread_create(&tidCreatoreWorkers, NULL, &creaThreadWorkers ,NULL) )!= 0 )
	{
		perror("SERVER-> Errore nella funzione pthread_create\n");
		strncpy(stringaToLog,"La creazione del thread creatore dei thread workers ha riscontrato un problema",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,0);
	}
	else
	{
		////printf("SERVER-> Creato creatore\n");

	}
}
