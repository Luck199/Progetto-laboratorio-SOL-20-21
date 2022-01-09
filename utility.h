#ifndef _utility_
#define _utility_

extern int clientTotali;
extern int contatoreCodaFd;
extern int thread_workers;
extern int fdInCoda;
extern int dim_memoria;
extern char name_socket[100];
extern int num_max_file;
extern int ricevutoSIGHUP;
extern int ricevutoSIGINTORQUIT;
extern int clientConnessi;
extern int threadInAttesa;
extern FILE *logFile;
extern struct codaInteri *codaFileDescriptor;
//masterWorkersPipe è una pipe che utilizzano thread main e thread worker per comunicare:
//tramite la cella in posizione 0 il thread main legge quali fileDescriptor sono pronti per essere impostati nell' insieme fd_set
//su 1 i thread workers scrivono tali fileDescriptor
extern int pipeGestioneWorkers[2];
extern int segnaleChiusuraHup;
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
    long data;
    short O_LOCK;
    short O_CREATE;
    //char *data;
    size_t dimensione;

    FILE * puntatoreFile;
    int identificatoreClient;

    size_t lettoriAttivi;
    size_t scrittoriAttivi;

    pthread_mutex_t lockFile;
    pthread_cond_t fileConditionVariable;
//    pthread_cond_t go;
}*array_file;

extern struct info_file *array_file;
extern pthread_mutex_t lockCodaComandi;
extern pthread_mutex_t lockClientConnessi;
extern pthread_mutex_t lockSegnali;
extern pthread_cond_t allClientExitCond;
extern pthread_cond_t CVFileDescriptor ;
#endif
