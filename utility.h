#ifndef _utility_
#define _utility_

extern int threadInAreaNonSafe;
extern int clientTotali;
extern int contatoreCodaFd;
extern int thread_workers;
extern int fdInCoda;
extern long dim_memoria;
extern char name_socket[100];
extern int num_max_file;
extern int ricevutoSIGHUP;
extern int ricevutoSIGINTORQUIT;
extern int clientConnessi;
extern int threadInAttesa;
extern FILE *logFile;
extern struct codaInteri *codaFileDescriptor;
extern int pipeGestioneWorkers[2];
extern int segnale_globale;
extern int broadcast;
extern int numMaxconnessioniContemporanee;

void letturaFile(char *config, char *nomeFilelog);
void decrementaNumClient();
void incrementaNumClient();
int getNumClient();
void accediCodaComandi();
void lasciaCodaComandi();
void enqueueCodaFileDescriptor(struct codaInteri *codaFileDescriptor, int fileDescriptorPointer);
int dequeueCodaFileDescriptor(struct codaInteri *codaFileDescriptor, int *stop);
void accediPipeWorker();
void lasciaPipeWorker();
void scriviSuLog(char * stringa, int count, ...);
void accediSegnali();
void lasciaSegnali();
int getSegnale();//ottenere il numero del segnale in mutua esclusione


extern struct struttura_workers
{
	int id_worker;						//Indica l' id del thread worker
	pthread_t threadId;
}*workers;

extern struct struttura_workers *workers;


extern struct info_file
{
    char *path;
    short fileAperto;
    char * byteFile;
    short O_LOCK;
    short O_CREATE;
    size_t dimensione;

    FILE * puntatoreFile;
    int identificatoreClient;

    size_t lettoriAttivi;
    size_t scrittoriAttivi;
    struct enqueue_Interi *codaLock;

    pthread_mutex_t lockFile;
    pthread_cond_t fileConditionVariable;
}*array_file;

extern struct info_file *array_file;
extern pthread_mutex_t lockCodaComandi;
extern pthread_mutex_t lockClientConnessi;
extern pthread_mutex_t lockSegnali;
extern pthread_mutex_t lockScritturaLog;
extern pthread_cond_t allClientExitCond;
extern pthread_cond_t fifoConsentita;
extern pthread_cond_t CVFileDescriptor ;
#endif
