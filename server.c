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



#define MAXPIPE 2
#define MAXLUNGHEZZA 150
#define namesize 32
#define SOCKNAME "./sockfile"
#define UNIX_PATH_MAX 108
#define N 100


void inizializzaScritturaLog(char *nomeLogFile);
void creaCreatoreThreadWorkers();
void* creaThreadWorkers();
void creaWorkers(int idWorker);
void *gestoreSegnali();
void creatoreThreadGestoreSegnali();
void mascheraSegnali();
void creaThreadGestioneConnessioni();
void* gestioneConnessioni();
int serverDeveTerminare();


pthread_cond_t endServer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lockEndServer = PTHREAD_MUTEX_INITIALIZER;

pthread_t tidWorker;
pthread_t threadGestoreSegnali;
int sigalarm_flag = 0;
sigset_t set;
int segnale=0;
pthread_t tidCreatoreWorkers;
pthread_t tidGestoreConnessioni;
struct sockaddr_un sa;

int fd_skt;
int fd_hwm = 0;
fd_set fd_set_connessioni;
fd_set read_set;
int fd_connessioni;

int main(int argc, char **argv)
{
	int i;

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


	int bindReturnValue;
	int listenReturnValue;
	int closeReturnValue=0;
	int joinReturnValue=0;
	int errore_lock_log=0;

	//leggo i parametri dal file di configurazione
	letturaFile(argv[1], nomeLogFile);

	//Alloco e inizializzo l' array dinamico che utilizzerò per gestire i file.
	//array di struct
	allocaStrutturaFile();

	//preparo il descrittore del file di log per dare la possibilità di scriverci sopra
	inizializzaScritturaLog(nomeLogFile);

	//configuro la gestione dei segnali
	//Mando in esecuzione il thread che si occuperà della gestione dei segnali
	creatoreThreadGestoreSegnali();

	mascheraSegnali();
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


	//determino l' fd più grande
	if (fd_skt > fd_hwm)
	{
		fd_hwm = fd_skt;
	}

	FD_ZERO(&fd_set_connessioni);
	FD_SET(fd_skt, &fd_set_connessioni);

	creaThreadGestioneConnessioni();

	joinReturnValue = pthread_join(threadGestoreSegnali, NULL);
	if(joinReturnValue != 0)
	{
		strncpy(stringaToLog,"La funzione pthread_join per l' attesa del thread gestore dei segnali ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	}
	printf("TERMINOOOOOOOOOOOOOOO\n\n\n\n\n\n");

	//Effettuo l' operazione di broadcast per svegliare eventuali thread che sono in wait
	accediCodaComandi();
	pthread_cond_broadcast(&(CVFileDescriptor));
	broadcast=1;
	lasciaCodaComandi();

	//gestione worker all' arrivo di un segnale di terminazione immediata
	int fdNew=0;
	int guasto=0;

	if(getSegnale() == SIGQUIT || getSegnale() == SIGINT)
	{
		//Se entro qui dentro significa che è arrivato il segnale  SIGINT o SIGQUIT
		while(getNumClient()>0)
		{
			printf("C'e' sempre qualche client\n");
			fdNew=dequeueCodaFileDescriptor(codaFileDescriptor,&guasto);
			printf("dequeueFileDescriptor {%d}\n", fdNew);
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
		printf("clienti finit\n");
	}

	printf("Waiting workers {%d}\n", thread_workers);
	for (i=0;i<thread_workers;i++)
	{
		////printf("SERVER-> Attendo il worker {%d}\n", workers[i].id_worker);
		joinReturnValue=pthread_join(workers[i].threadId, NULL);
		if(joinReturnValue != 0)
		{
			strncpy(stringaToLog,"La funzione pthread_join per l' attesa dei thread workers ha riscontrato un errore.",MAXLUNGHEZZA);
			scriviSuLog(stringaToLog, 0);
		}
		////printf("SERVER-> Atteso il worker {%d} \n", workers[i].id_worker);
	}


	printf("Waiting creatore workers\n");
	joinReturnValue=pthread_join(tidCreatoreWorkers,NULL);
	if(joinReturnValue != 0)
	{
		strncpy(stringaToLog,"La funzione pthread_join per l' attesa del thread creatore dei workers ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	}

/*	printf("Waiting gestore segnali\n");
	joinReturnValue=pthread_join(threadGestoreSegnali,NULL);
	if(joinReturnValue != 0)
	{
		strncpy(stringaToLog,"La funzione pthread_join per l' attesa del thread per la gestione dei segnali ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog, 0);
	}
*/
	printf("Waiting gestore connessioni\n");
	joinReturnValue=pthread_join(tidGestoreConnessioni,NULL);
	if(joinReturnValue != 0)
	{
		strncpy(stringaToLog,"La funzione pthread_join per l' attesa del thread che gestisce le connessioni ha riscontrato un errore.",MAXLUNGHEZZA);
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
	float maxMemRaggiuntaInMb=0;
	maxMemRaggiuntaInMb=(float)maxMemoriaRaggiunta/1048576;
	printf("Stampe finali:\n");
	printf("-> Numero di file massimo memorizzato nel server: %d\n",numMaxFilePresenti);
	printf("-> Dimensione massima in Mbytes raggiunta dal file storage: %f %ld\n",maxMemRaggiuntaInMb, maxMemoriaRaggiunta);
	printf("-> Numero di volte in cui l' algoritmo di rimpiazzamento della cache è stato eseguito per selezionare uno o più file \"vittima\": %d\n",numVolteAlgoritmoRimpiazzo);
	printf("-> lista dei file contenuti nello storage al momento della chiusura del server:\n");
	for(i=0;i<num_max_file;i++)
	{
		if(strncmp(array_file[i].path,"",1)!=0)
		{
			verificaSeStrutturaVuota=1;
			printf("\t-> %s\n",array_file[i].path);
		}
	}
	if(verificaSeStrutturaVuota==0)
	{
		printf("\t-> non sono presenti file nella struttura dati\n");
	}


	strncpy(stringaToLog,"numMaxFilePresenti",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,1,numMaxFilePresenti);



	strncpy(stringaToLog,"maxMemoriaRaggiunta",MAXLUNGHEZZA);

	//Questa scrittura su log viene fatta senza la funzione apposita, a causa del tipo diverso del dato
	if((errore_lock_log=pthread_mutex_lock(&lockScritturaLog))!=0)
	{
		errno=errore_lock_log;
		perror("lock scrittura log");
		pthread_exit(&errore_lock_log);
	}
	fprintf(logFile, "%s: %f \n" ,stringaToLog, maxMemRaggiuntaInMb);
	pthread_mutex_unlock(&lockScritturaLog);






	strncpy(stringaToLog,"NumRimpiazzi",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,1,numVolteAlgoritmoRimpiazzo);

	strncpy(stringaToLog,"NumMaxConnessioniContemporanee",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,1,numMaxconnessioniContemporanee);



	fclose(logFile);
	deallocaStrutturaFile();
	//printf("***************\n");
	//printf("FINE SERVER!!!\n");
//	//printf("SERVER-> finito Server, i client connessi risultano: %d\n",clientConnessi);
	return 0;
}


void inizializzaScritturaLog(char *nomeLogFile)
{
	char stringaToLog[MAXLUNGHEZZA];
	if((logFile = fopen(nomeLogFile, "w")) == NULL)
	{
		perror("SERVER-> Error fopen");
		strncpy(stringaToLog,"La funzione fopen per file di log ha riscontrato un errore.",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,0);
		exit(EXIT_FAILURE);
	}
	strncpy(stringaToLog,"Funzione fopen per file di log riuscita correttamente.",MAXLUNGHEZZA);
	scriviSuLog(stringaToLog,0);
}



void *gestoreSegnali()
{
	char stringaToLog[MAXLUNGHEZZA];
	int sigEmptySetReturnValue=0;
	int sigAddSetReturnValue=0;
	sigset_t pset;
	int numeroSegnale = 0;

	//azzero la maschera puntata da pset
	sigEmptySetReturnValue=sigemptyset(&pset);
	if(sigEmptySetReturnValue == -1)
	{
		perror("SERVER -> errore nell' operazione sigEmptySet\n");
	}

	//metto a 1 la posizione del segnale indicato come secondo parametro nella maschera pset
	sigAddSetReturnValue=sigaddset(&pset, SIGINT);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER -> errore nell' operazione sigAddSet\n");
	}
	sigAddSetReturnValue=sigaddset(&pset, SIGQUIT);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER -> errore nell' operazione sigAddSet\n");
	}

	sigAddSetReturnValue=sigaddset(&pset, SIGHUP);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER -> errore nell' operazione sigAddSet\n");
	}
	sigAddSetReturnValue=sigaddset(&pset, SIGTSTP);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER -> errore nell' operazione sigAddSet\n");
	}
	sigAddSetReturnValue=sigaddset(&pset, SIGTERM);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER -> errore nell' operazione sigAddSet\n");
	}
	int pthreadSigmaskReturnValue=0;
	//applico la maschero pset
	pthreadSigmaskReturnValue=pthread_sigmask(SIG_SETMASK, &pset, NULL);
	if(pthreadSigmaskReturnValue!=0)
	{
		perror("SERVER -> errore nell' operazione pthread_sigmask\n");
	}

	int sigwaitReturnValue=0;

	printf("Sto aspettando i segnoaliaaa\n");
	//Mi metto in attesa dell' arrivo di un segnale
	sigwaitReturnValue=sigwait(&pset, &numeroSegnale);

	if(sigwaitReturnValue!=0)
	{
		perror("SERVER -> errore nell' operazione sigwait");
	}
	if (numeroSegnale == SIGTSTP || numeroSegnale == SIGTERM)
	{
		//Nel caso di arrivo del segnale SIGSTP o SIGTERM termino.
		exit(EXIT_FAILURE);
	}
	else if (numeroSegnale == SIGQUIT || numeroSegnale == SIGINT || numeroSegnale == SIGHUP)
	{
		////printf("SERVER-> Arrivato SIGQUIT o SIGINT\n");
		strncpy(stringaToLog,"Arrivato segnale di terminazione",MAXLUNGHEZZA);
		printf("arrivato segnale di chiusura {%s}\n", strsignal(numeroSegnale));
		scriviSuLog(stringaToLog,1,strsignal(numeroSegnale));
		accediSegnali();
		segnale_globale=numeroSegnale;
		lasciaSegnali();
	}
	else
	{
		//Questo segnale non è gestito
		strncpy(stringaToLog,"Arrivato segnale non gestito, il cui indice è",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,1,strsignal(numeroSegnale));
		pthread_exit(NULL);
	}

	pthread_exit(NULL);
}



void creatoreThreadGestoreSegnali()
{
	if(	pthread_create(&threadGestoreSegnali, NULL, gestoreSegnali, NULL)!=0)
	{
		perror("SERVER-> Errore nella funzione pthread_create\n");
		exit(EXIT_FAILURE);
	}
}


void mascheraSegnali()
{
	struct sigaction sga;
	int sigEmptySetReturnValue=0;
	int sigAddSetReturnValue=0;
	//Maschero tutti i segnali
	sigset_t pset;
	int sigFillSetReturnValue=0;
	int pthreadSigMaskReturnValue=0;
	sigFillSetReturnValue=sigfillset(&pset);
	if(sigFillSetReturnValue == -1)
	{
		perror("SERVER -> errore nell' operazione sigfillset\n");
	}
	pthreadSigMaskReturnValue=pthread_sigmask(SIG_SETMASK, &pset, NULL);
	if(pthreadSigMaskReturnValue != 0 )
	{
		perror("SERVER -> errore nell' operazione pthread_sigmask\n");
	}
	//Ignoro il segnale SIGPIPE
	memset(&sga, 0, sizeof(sga));
	sga.sa_handler = SIG_IGN;
	int sigActionreturnValue=0;
	sigActionreturnValue=sigaction(SIGPIPE, &sga, NULL);
	if(sigActionreturnValue == -1)
	{
		perror("SERVER -> errore nell' operazioen sigaction");
	}
	// maschero i segnali gestiti dal thread
	sigEmptySetReturnValue=sigemptyset(&pset);
	if(sigEmptySetReturnValue==-1)
	{
		perror("SERVER-> l' operazione sigEmptySet ha riscontrato un errore\n");
	}
	sigAddSetReturnValue=sigaddset(&pset, SIGINT);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER-> l' operazione sigaddset ha riscontrato un errore\n");
	}
	sigAddSetReturnValue=sigaddset(&pset, SIGQUIT);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER-> l' operazione sigaddset ha riscontrato un errore\n");
	}
	sigAddSetReturnValue=sigaddset(&pset, SIGTSTP);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER-> l' operazione sigaddset ha riscontrato un errore\n");
	}
	sigAddSetReturnValue=sigaddset(&pset, SIGTERM);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER-> l' operazione sigaddset ha riscontrato un errore\n");
	}
	sigAddSetReturnValue=sigaddset(&pset, SIGHUP);
	if(sigAddSetReturnValue==-1)
	{
		perror("SERVER-> l' operazione sigaddset ha riscontrato un errore\n");
	}

	//ristabilisco gli altri segnali
	int pthreadSigmaskReturnValue=0;
	pthreadSigmaskReturnValue=pthread_sigmask(SIG_SETMASK, &pset, NULL);
	if(pthreadSigmaskReturnValue != 0)
	{
		perror("SERVER -> errore nell' operazione pthread_sigmask");
	}
}


void creaWorkers(int idWorker)
{
	char stringaToLog[MAXLUNGHEZZA];
	//////printf("SERVER-> crea workers\n");
	if((pthread_create(&tidWorker, NULL, &vitaWorker, (void*)(intptr_t)idWorker))!=0)
	{
		perror("SERVER-> Errore nella funzione pthread_create\n");
		strncpy(stringaToLog,"La creazione di un thread worker ha riscontrato un problema",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,0);
		exit(EXIT_FAILURE);
	}

}



//funzione che si occupa della creazione di tutti i thread worker, grazie all' utilizzo della funzione creaWorkers
void* creaThreadWorkers()
{
	int i;
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
}

void creaThreadGestioneConnessioni(){
	int err;
	char stringaToLog[MAXLUNGHEZZA];
	if ( (err=pthread_create(&tidGestoreConnessioni, NULL, &gestioneConnessioni ,NULL) )!= 0 )
	{
		perror("SERVER-> Errore nella funzione pthread_create\n");
		strncpy(stringaToLog,"La creazione del thread gestore connessioni ha riscontrato un problema",MAXLUNGHEZZA);
		scriviSuLog(stringaToLog,0);
	}
}

void* gestioneConnessioni(){
	//ciclo dove verranno gestite le connessioni con i vari client.
	//Verrà interrotto solo all' arrivo di un segnale
	char stringaToLog[MAXLUNGHEZZA];
	int selectReturnValue = 0;
	int acceptReturnValue = 0;
	int primaVolta = 0;

	while (!serverDeveTerminare())
	{
		printf("A--- Segnale {%s}\n", strsignal(getSegnale()));
		read_set = fd_set_connessioni;
		struct timeval tv;
		tv.tv_sec = 15;
		tv.tv_usec = 0;
		selectReturnValue = select(fd_hwm + 1, &read_set, NULL, NULL, &tv);
		printf("SELECT ESEGUITA\n");
		if (selectReturnValue == -1 && errno == EINTR)
		{
			printf("SELECT interrotta da segnale\n");
			perror("SERVER-> la funzione select è stata interrotta dall' arrivo di un segnale\n");
			strncpy(stringaToLog,"Funzione select interrotta dall' arrivo di un segnale.",MAXLUNGHEZZA);
			scriviSuLog(stringaToLog, 0);
			pthread_exit(NULL);
		}
		else if (selectReturnValue == -1)
		{
			printf("SELECT ita male\n");
			perror("SERVER-> un errore è stato riscontrato dalla funzione select\n");
			strncpy(stringaToLog,"La funzione select ha riscontrato un errore.",MAXLUNGHEZZA);
			scriviSuLog(stringaToLog, 0);
			pthread_exit(NULL);
		}
		else if (selectReturnValue == 0){
			printf("Select Timeout Expired\n");
		}
		else
		{
			printf("SELECT ita bene\n");
			strncpy(stringaToLog,"Funzione select eseguita in maniera corretta",MAXLUNGHEZZA);
			scriviSuLog(stringaToLog, 0);
			for (fd_connessioni = 0; fd_connessioni <= fd_hwm && !serverDeveTerminare(); fd_connessioni++)
			{
				if (FD_ISSET(fd_connessioni, &read_set))
				{
					if (fd_connessioni == fd_skt && segnale == 0)
					{
						//Entro nel ramo if se non sono presenti segnali

						//Operazione di accettazione di un nuovo client
						acceptReturnValue = accept(fd_skt, NULL, 0);

						if (acceptReturnValue == -1 && errno == EINTR)
						{
							perror("SERVER-> la funzione accept è stata interrotta dall' arrivo di un segnale\n");
							strncpy(stringaToLog,"Funzione accept interrotta dall' arrivo di un segnale.",MAXLUNGHEZZA);
							scriviSuLog(stringaToLog, 0);
							pthread_exit(NULL);
						}
						else if (acceptReturnValue == -1)
						{
							perror("SERVER-> un errore è stato riscontrato dalla funzione accept\n");
							strncpy(stringaToLog,"La funzione accept ha riscontrato un errore.",MAXLUNGHEZZA);
							scriviSuLog(stringaToLog, 0);
							pthread_exit(NULL);
						}
						strncpy(stringaToLog,"Funzione accept eseguita in maniera corretta.",MAXLUNGHEZZA);
						scriviSuLog(stringaToLog, 0);
						//un nuovo client si è connesso, aumento il numero dei client connessi
						incrementaNumClient();

						int totClienti=getNumClient();
						strncpy(stringaToLog,"Si è connesso un nuovo client, adesso il totale ammonta a",MAXLUNGHEZZA);
						scriviSuLog(stringaToLog,1,totClienti);

						//gestione delle connessioni
						FD_SET(acceptReturnValue, &fd_set_connessioni);
						if (acceptReturnValue > fd_hwm)
						{
							fd_hwm = acceptReturnValue;
						}
						char  daInviare[200]="";
						int inviaDatiReturnValue=0;
						strncpy(daInviare,"connessione eseguita correttamente!\n",37);
						size_t a=strlen(daInviare);
						printf("INVIO DATI\n");
						inviaDatiReturnValue=inviaDati(acceptReturnValue,&a,sizeof(size_t));
						if(inviaDatiReturnValue <= 0)
						{
							perror("SERVER -> errore nell' operazione inviaDati");
						}
						printf("INVIO DATI\n");
						inviaDatiReturnValue=inviaDati(acceptReturnValue,&daInviare,a);
						if(inviaDatiReturnValue <= 0)
						{
							perror("SERVER -> errore nell' operazione inviaDati");
						}
					}
					else
					{
						//Nel caso in cui un client gia connesso abbia richiesto un'operazione,
						//invio quest' ultima ai thread workers per essere gestita, e  rimuovo il file descriptor dall' insieme
						strncpy(stringaToLog,"Ricevuta una richiesta dal client",MAXLUNGHEZZA);
						scriviSuLog(stringaToLog,1,fd_connessioni);

						FD_CLR(fd_connessioni, &fd_set_connessioni);
						if (fd_connessioni == fd_hwm)
						{
							fd_hwm--;
						}
						printf("ACCODO RICHIESTA\n");
						//diviso in due volte per via del primo inserimento
						if(primaVolta==0)
						{
							accediCodaComandi();
							codaFileDescriptor->fileDescriptor=fd_connessioni;
							contatoreCodaFd++;
							pthread_cond_signal(&CVFileDescriptor);
							primaVolta=1;
							lasciaCodaComandi();
						}
						else
						{
							//dalla seconda volta in poi
							enqueueCodaFileDescriptor(codaFileDescriptor, fd_connessioni);
						}
					}
				}
			}
		}
	}
	pthread_exit(NULL);
}

int serverDeveTerminare(){
	int serverDeveTerminare = 0;
	if (getSegnale() == SIGQUIT || getSegnale() == SIGINT || (getSegnale() == SIGHUP && getNumClient() == 0)){
		serverDeveTerminare = 1;
	}
	return serverDeveTerminare;
}
