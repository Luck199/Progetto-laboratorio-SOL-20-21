#ifndef _utility_
#define _utility_

extern int contatoreCodaFd;
extern int thread_workers;
extern int dim_memoria;
extern char name_socket[100];
extern int num_max_file;
extern int ricevutoSIGHUP;
extern int ricevutoSIGINTORQUIT;
extern int clientConnessi;
extern FILE *logFile;
extern struct codaInteri *codaFileDescriptor;
//masterWorkersPipe è una pipe che utilizzano thread main e thread worker per comunicare:
//tramite la cella in posizione 0 il thread main legge quali fileDescriptor sono pronti per essere impostati nell' insieme fd_set
//su 1 i thread workers scrivono tali fileDescriptor
extern int pipeGestioneWorkers[2];
extern int segnaleChiusuraHup;


void letturaFile(char *config, char *nomeFilelog);
void decrementaNumClient();
void incrementaNumClient();
int getNumClient();
void accediCodaComandi();
void lasciaCodaComandi();
void enqueueCodaFileDescriptor(struct codaInteri *codaFileDescriptor, int fileDescriptorPointer);
int dequeueCodaFileDescriptor(struct codaInteri *codaFileDescriptor);
void accediPipeWorker();
void lasciaPipeWorker();
void scriviSuLog(char * stringa, int count, ...);

ssize_t readn(int fd, void *v_ptr, size_t n);
ssize_t writen(int fd, void *v_ptr, size_t n);

extern struct struttura_workers
{
	int id_worker;						//Indica l' id del thread worker
	pthread_t threadId;
}*workers;

extern struct struttura_workers *workers;


extern struct info_file
{
    char *path;
    long data;
    //char *data;
    size_t dimensione;

    FILE * puntatoreFile;
//    OwnerId currentlyLockedBy; // OwnerId (client id), 0 if no owner
//    OwnerId ownerCanWrite;     // OwnerId (client id), 0 if no owner
//    List_T waitingLockers;     // OwnerIds (client ids)
//    List_T openedBy;           // OwnerIds (client ids)

    size_t lettoriAttivi;
    size_t scrittoriAttivi;

    pthread_mutex_t lockFile;
//    pthread_mutex_t ordering;
//    pthread_cond_t go;
}*array_file;

extern struct info_file *array_file;

extern pthread_mutex_t lockClientConnessi;
extern pthread_cond_t allClientExitCond;
#endif
