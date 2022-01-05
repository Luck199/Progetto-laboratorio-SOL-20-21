
#ifndef GESTIONEFILE_H_
#define GESTIONEFILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdarg.h>



void allocaStrutturaFile();//funzione per allocare la struttura dati che conterrà i miei file
void deallocaStrutturaFile();//funzione per deallocare la struttura dati che conterrà i miei file
void accediStrutturaFile();//funzione per ottenere il lock per accedere in mutua esclusione la struttura dati che conterrà i miei file
void lasciaStrutturaFile();//funzione per rilasciare il lock per accedere in mutua esclusione la struttura dati che conterrà i miei file
int aggiungiFile(char * path, char * buf, size_t sizeFile, int fdDaElaborare);//funzione che aggiunge un nuovo file alla struttura dati che simulerà la memoria del server e ritorna 1 in caso di successo, -1 in caso di errore
void visualizzaArrayFile();//funzione che stampa le info di ogni file
int cercaFile(char* pathname);//funzione che cerca il file identificato da pathname all' interno dell' array dei file, e restituisce uno se è presente al suo interno, -1 altrimenti
int verificaInserimento(int dimFile, int fdDaElaborare);//funzione che verifica se è presente spazio nell' array per inserire un file: in caso positivo ritorna 1, altrimenti applica l' algoritmo di rimpiazzo
void applicaFifo(int fdDaElaborare, int daSalvare);//funzione che applica l' algoritmo di rimpiazzo fifo nell' array dei file
void assumiLockFileLettura(int indiceFile);
void lasciaLockFileLettura(int indiceFile);
void assumiLockFileScrittura(int indiceFile, int fdDaElaborare);
int lasciaLockFileScrittura(int indiceFile,int fdDaElaborare);
int openFileServer(char *path, int flag, int fdDaElaborare);//funzione open file lato server, implementa la reale funzione di aprire un file,superati i vari controlli
int closeFileServer(char *path,int fdDaElaborare);
int lockFileServer(char *path, int fdDaElaborare);
int unlockFileServer(char *path, int fdDaElaborare);
int removeFileServer(char * path, int fdDaElaborare);
int applicaRemove(char *path);
int appendToFileServer(char* path,char* buf, size_t size,int fdDaElaborare);
int readFileServer(char* path, char *buffer2,size_t *dimFile,int fdDaElaborare);
int readNFileServer(int N,  int fdDaElaborare);
int writeFileServer(char* path, char  * dati, size_t sizeFile,int fdDaElaborare);



extern int numMaxFilePresenti;
extern float maxMemoriaRaggiunta;
extern int numVolteAlgoritmoRimpiazzo;
extern int memoriaDisponibile;//memoria disponibile nell' array
extern int numFileDisponibili;//numero di file che posso ancora inserire nell' array
extern int posizioneLibera;//prossima posizione dove poter scrivere nell' array dei miei file
extern int filePiuVecchio;//posizione dell' array dei file dove è memorizzato il file più vecchio, quello da eliminare con la politica FIFO
extern int numFilePresenti;//numero dei file presenti all' interno dell' array


#endif /* GESTIONEFILE_H_ */
