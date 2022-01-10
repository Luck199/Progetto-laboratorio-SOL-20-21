//Created on: 24 nov 2021
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <sys/time.h>

#include "comunicazioneClientServer.h"

#include "gestioneFile.h"
#include "utility.h"

#define MAXSTRING 250
pthread_mutex_t lockStrutturaFile = PTHREAD_MUTEX_INITIALIZER;

int memoriaDisponibile;
int numFileDisponibili;
int posizioneLibera;
int numFilePresenti;
int filePiuVecchio;
int numMaxFilePresenti;
float maxMemoriaRaggiunta;
int numVolteAlgoritmoRimpiazzo;
char * cartellaClient;
char * cartellaServer;


void allocaStrutturaFile()
{
	int i=0;

	memoriaDisponibile = dim_memoria;
	numFileDisponibili = num_max_file;
	posizioneLibera = 0;
	numFilePresenti = 0;
	filePiuVecchio = 0;
	array_file = calloc(num_max_file, sizeof(struct info_file));//per array

	for(i=0;i<num_max_file;i++)
	{
		array_file[i].path=(char*)malloc(sizeof(char)*MAXSTRING);
		strncpy(array_file[i].path,"vuoto",6);

		array_file[i].data=0;
		array_file[i].fileAperto=0;
		array_file[i].dimensione=0;
		array_file[i].lettoriAttivi=0;
		array_file[i].scrittoriAttivi=0;
		array_file[i].puntatoreFile = NULL;
		array_file[i].identificatoreClient=0;
		array_file[i].byteFile=(char*)malloc(sizeof(char)*MAXSTRING);
		strncpy(array_file[i].byteFile,"vuoto",6);
		pthread_mutex_init(&(array_file[i].lockFile), NULL);
		pthread_cond_init(&(array_file[i].fileConditionVariable), NULL);


	}
	numMaxFilePresenti=0;
	maxMemoriaRaggiunta=0;
	numVolteAlgoritmoRimpiazzo=0;
	//Allocazione struttura file terminata con successo
}


void deallocaStrutturaFile()
{
	int i=0;
	for(i=0;i<num_max_file;i++)
	{
		free(array_file[i].path);
		if(array_file[i].fileAperto!=0 )
		{
			fclose(array_file[i].puntatoreFile);
			array_file[i].fileAperto=0;
		}
		pthread_mutex_destroy(&(array_file[i].lockFile));
		pthread_cond_destroy(&(array_file[i].fileConditionVariable));
		free(array_file[i].byteFile);

	}
	free(array_file);
}

void accediStrutturaFile()
{
	int errore=0;
	if ( ( errore=pthread_mutex_lock(&lockStrutturaFile)) != 0 )
	{
		perror("lock struttura file\n");
		pthread_exit(&errore);
	}
	//assunto lock totale
}


void lasciaStrutturaFile()
{
	int errore=0;
	if ( ( errore=pthread_mutex_unlock(&lockStrutturaFile)) != 0 )
	{
		perror("unlock struttura file\n");
		pthread_exit(&errore);
	}
	//lasciato lock totale
}
void visualizzaArrayFile()
{
	int i=0;
//	printf("\n\n\narray dei file: \n");
//	printf("numero di file presenti: %d\n",numFilePresenti);
//	printf("memoria disponibile: %d\n",memoriaDisponibile);
//	printf("numero dei file ancora inseribili: %d\n",numFileDisponibili);
//
	//***********************************
	//Tale procedura dà errore con valgrind, non utilizzare in progetto finale
	//***********************************
	for(i=0;i<num_max_file;i++)
	{
		printf("path[%d]:%s\n",i,array_file[i].path);
//		printf("posizione: %d -> file : %s\n",i,array_file[i].path);
		printf("dimensione in byte: %ld\n",array_file[i].dimensione);
		//printf("data e ora di inserimento: %ld\n",array_file[i].data);

	}
}

int aggiungiFile(char * path, char * buf, size_t sizeFile, int fdDaElaborare)
{
	int posDiRitorno=0;
	if(sizeFile>dim_memoria)
	{
		printf("Il file richiesto è più grande di tutta la memoria disponibile! Non può essere inserito!\n");
		return -1;
	}
	else
	{
		int verificaInserimentoReturnValue=verificaInserimento(sizeFile,  fdDaElaborare);
		if(verificaInserimentoReturnValue != 1)
		{
			perror("Errore nell' inserimento del nuovo file\n");
			return -1;
		}
		//se arrivo qui vuol dire che l' inserimento del file può essere fatto

		while(array_file[posizioneLibera].O_CREATE != 0)
		{
			posizioneLibera=(posizioneLibera+1) % num_max_file;
		}

		if(strlen(array_file[posizioneLibera].path)<(strlen(path)+1) && array_file[posizioneLibera].path!=NULL)
		{
			free(array_file[posizioneLibera].path);
			array_file[posizioneLibera].path=malloc(sizeof(char)*(strlen(path)+1));
		}
		strncpy(array_file[posizioneLibera].path,path,strlen(path)+1);
		//array_file[posizioneLibera].data= time(NULL);
		if(array_file[posizioneLibera].byteFile != NULL)
		{
			free(array_file[posizioneLibera].byteFile);
		}

		array_file[posizioneLibera].byteFile=malloc(sizeof(char)*sizeFile);
		strncpy(array_file[posizioneLibera].byteFile,buf,sizeFile);
		array_file[posizioneLibera].dimensione=sizeFile;
		numFilePresenti++;
		array_file[posizioneLibera].O_CREATE=1;


		//ho aggiunto correttamente un file alla struttura, modifico le variabili necessarie
		posDiRitorno=posizioneLibera;
		posizioneLibera=(posizioneLibera+1) % num_max_file;
		numFileDisponibili-=1;
		memoriaDisponibile=memoriaDisponibile-sizeFile;
		maxMemoriaRaggiunta=maxMemoriaRaggiunta+sizeFile;
		numMaxFilePresenti++;
//		visualizzaArrayFile();
		return posDiRitorno;

	}

}

int verificaInserimento(int dimFile, int fdDaElaborare)
{
	if((memoriaDisponibile >= dimFile) && (numFileDisponibili>0))
	{
		return 1;
	}
	else
	{
		//siamo in un caso di capacity misses, applico l' algoritmo di rimpiazzo con la politica FIFO
		//Se lo applico fifo tramite la funzione verificaInserimento vuol dire che non mi interessa quale file venga eliminato
		//dalla struttura dati, quinid come secondo argomento passo -1.
		while(memoriaDisponibile < dimFile || numFileDisponibili==0)
		{
			applicaFifo(fdDaElaborare,-1);
		}
		//visualizzaArrayFile();
		return 1;
	}
}

void applicaFifo(int fdDaElaborare, int daSalvare)
{

	//printf("applico fifo\n");
	//grazie alla politica fifo, so che una volta eliminato il file più vecchio, il quale
	//indice è presente nella variabile filePiuVecchio, il file presente nell' array alla sua destra ( in modulo ) sarà sempre il più vecchio
	int i=filePiuVecchio,trovato=0;
//	size_t daLiberare=0;

	char espelliPath[300]="";
	char * espelliDati=NULL;

	size_t dimEspulso=0;

	while(trovato != 1)
	{
		//La variabile daSalvare indica quale file non deve essere eliminato, perchè si vuole liberare
		//spazio proprio per quel file, quindi risulterebbe un errore cancellarlo

		//l' ultima condizione del costrutto if è necessaria nelle situazione in cui si abbia bisogno di memoria e non
		//di un posto per un file nella struttura
		if((array_file[i].O_LOCK==0) && daSalvare != i && (array_file[i].fileAperto==0))//(array_file[i].lettoriAttivi == 0) &&  (array_file[i].scrittoriAttivi == 0))
		{
			espelliDati=malloc(sizeof(char)*(array_file[i].dimensione+1));

			//elimino il file in questa posizione, perchè è quello che è da più tempo nell' array e in questo momento non è in stato di lock
			strncpy(espelliPath,array_file[i].path,strlen(array_file[i].path)+1);
			memcpy(espelliDati,array_file[i].byteFile,array_file[i].dimensione);
//			printf("dimEspulso:%ld\n",dimEspulso);

			dimEspulso=array_file[i].dimensione;
			strncpy(array_file[i].path,"vuoto",6);

			array_file[i].data=0;
//			daLiberare=array_file[i].dimensione;

			array_file[i].dimensione=0;
			array_file[i].O_LOCK = 0;
			array_file[i].O_CREATE = 0;
			array_file[i].lettoriAttivi=0;
			array_file[i].scrittoriAttivi=0;
			trovato = 1;

			filePiuVecchio=(i+1)%num_max_file;
			numFilePresenti--;
			posizioneLibera= i;
		}
		else
		{
			i=(i+1)%num_max_file;
		}
	}
	size_t dimPath=strlen(espelliPath)+1;
	inviaDati(fdDaElaborare,&dimPath,sizeof(size_t));
	inviaDati(fdDaElaborare,&espelliPath,dimPath);

	size_t b=strlen(espelliDati)+1;
	inviaDati(fdDaElaborare,&dimEspulso,sizeof(size_t));
	inviaDati(fdDaElaborare,&espelliDati,dimEspulso);
//	printf("lunghezzadatiLatoServer:%ld\n",b);

//	printf("EspelliDati:%s\n",espelliDati);
	numFileDisponibili+=1;
	memoriaDisponibile=memoriaDisponibile+dimEspulso;
	numVolteAlgoritmoRimpiazzo++;
//	free(espelliPath);
//	free(espelliDati);
}


void assumiLockFileLettura(int indiceFile)
{
	int errore=0;
	if ( ( errore=pthread_mutex_lock(&array_file[indiceFile].lockFile)) != 0 )
	{
		perror("lock singolo file\n");
		pthread_exit(&errore);
	}
	array_file[indiceFile].lettoriAttivi++;
}

void lasciaLockFileLettura(int indiceFile)
{
	int errore=0;
	if ( ( errore=pthread_mutex_unlock(&array_file[indiceFile].lockFile)) != 0 )
	{
		perror("lock singolo file\n");
		pthread_exit(&errore);
	}
	array_file[indiceFile].lettoriAttivi--;
}

void assumiLockFileScrittura(int indiceFile,int fdDaElaborare)
{
	int errore=0,entrato=0;
	while((array_file[indiceFile].O_LOCK == 1) && (array_file[indiceFile].identificatoreClient!=0) && (array_file[indiceFile].identificatoreClient != fdDaElaborare))
	{
		entrato=1;
//		printf("vorrei scrivere sul file, ma mi metto in attesa della lock, buonanotte!\n");
		pthread_cond_wait(&(array_file[indiceFile].fileConditionVariable), &(lockStrutturaFile));
//		printf("Sono stato svegliato, buongiorno!\n");
	}
	if(entrato==1)
	{
		entrato=0;
	}
	if(strcmp(array_file[indiceFile].path,"vuoto")==0)
	{
		printf("si vuole lockare un file che non esiste più!\n");
		return;
	}
	if((errore=pthread_mutex_lock(&array_file[indiceFile].lockFile))!=0)
	{
		perror("lock singolo file\n");
		pthread_exit(&errore);
	}
	array_file[indiceFile].O_LOCK =1;
	array_file[indiceFile].scrittoriAttivi++;
	array_file[indiceFile].identificatoreClient =fdDaElaborare;
}

int lasciaLockFileScrittura(int indiceFile,int fdDaElaborare)
{
	int errore=0;
	if((array_file[indiceFile].O_LOCK == 0) || (array_file[indiceFile].identificatoreClient==0) || (array_file[indiceFile].identificatoreClient != fdDaElaborare))
	{
//		printf("Si vuole rilasciare una lock che non si possiede! errore!\n");
		return -1;
	}

	if ( ( errore=pthread_mutex_unlock(&array_file[indiceFile].lockFile)) != 0 )
	{
		perror("lock singolo file\n");
		pthread_exit(&errore);
	}
	array_file[indiceFile].scrittoriAttivi--;
	array_file[indiceFile].O_LOCK = 0;
	array_file[indiceFile].identificatoreClient = 0;
	return 0;
}

int cercaFile(char* pathname)
{
	int i=0;
	for(i=0; i<num_max_file; i++)
	{
		if(strcmp(array_file[i].path,pathname) == 0 )
		{
			return i;
		}
	}

	return -1;
}

int openFileServer(char *path, int flag, int fdDaElaborare)
{
//	verifico se il flag ricevuto ha un valore uguale a 0 oppure a 1
	if (flag < 0 || flag > 2)
	{
		errno = EINVAL;
		return -1;
	}
//	se il file richiesto è presente nell' array e il flag risulta 0 ( ovvero O_CREATE) ritorno errore
	int indiceFile=cercaFile(path);

	if(((flag == 0)||(flag==2)) && (indiceFile>=0))
	{
		printf("si vuole creare un file già presente, errore!\n");
		return -1;
	}
//	se il file richiesto non è presente nell' array e il flag risulta 1 ( ovvero O_LOCK) ritorno errore
	if((flag == 1) && (indiceFile == -1))
	{
		printf("si vuole lockare un file non presente, errore!\n");
		return -1;
	}

//	Inserisco file se non risulta presente, ovviamente vuoto
	if(indiceFile == -1)
	{
		char * bufNuovoFile=malloc(sizeof(char)*6);

		strncpy(bufNuovoFile,"vuoto",6);//il buffer del nuovo file sarà chiaramente vuoto
		indiceFile=aggiungiFile(path,bufNuovoFile,6, fdDaElaborare);

		free(bufNuovoFile);
	}

	//Se sono arrivato qui vuol dire che posso procedere ad eseguire l' operazione di openFile


	if(flag != 0)
	{
		assumiLockFileScrittura(indiceFile,fdDaElaborare);
	}

	printf("file %s acquisito dal client %d\n",array_file[indiceFile].path,array_file[indiceFile].identificatoreClient);
	array_file[indiceFile].puntatoreFile=fopen(array_file[indiceFile].path, "a");


	if(array_file[indiceFile].puntatoreFile == NULL)
	{
		perror("SERVER -> Error fopen");
//		strncpy(stringaToLog,"La funzione fopen per file di log ha riscontrato un errore.",MAXLUNGHEZZA);
//		scriviSuLog(stringaToLog,0);
		return -1;
	}
	array_file[indiceFile].fileAperto=1;

	return 1;
}

int closeFileServer(char *path,int fdDaElaborare)
{
	int indiceFile=cercaFile(path);
	if( (indiceFile == -1))
	{
		printf("si vuole chiudere un file non presente, errore!\n");
		return -1;
	}
	if(array_file[indiceFile].identificatoreClient != fdDaElaborare)
	{
		printf("non puoi chiudere un file che non hai lockato te! \n");
		return -1;
	}
	if(array_file[indiceFile].puntatoreFile == NULL)
	{
		printf("Non posso chiudere un file non aperto\n");
		return -1;
	}
	fclose(array_file[indiceFile].puntatoreFile);
	array_file[indiceFile].fileAperto=0;
	pthread_cond_signal(&(array_file[indiceFile].fileConditionVariable));
	int uvalue=0;
	uvalue= lasciaLockFileScrittura(indiceFile,fdDaElaborare);
	if(uvalue==-1)
	{
		//CONTROLLA ERRORE
		//Nel caso in cui si stia entranto in questo if vuol dire che si è richiesta un' operazioen di unlock su un file su cui
		//non si era eseguita l' operazione di lock
		printf("UNLOCK_CLOSE: errore\n");
	}
	return 1;
}


int lockFileServer(char *path, int fdDaElaborare)
{
	int indiceFile=cercaFile(path);
	if( (indiceFile == -1))
	{
		printf("si vuole effettuare l' operazione di lock su  un file non presente, errore!\n");
		return -1;
	}
	assumiLockFileScrittura(indiceFile, fdDaElaborare);
	return 1;
}


int unlockFileServer(char *path, int fdDaElaborare)
{
	int indiceFile=cercaFile(path);
	if((indiceFile == -1))
	{
		printf("si vuole effettuare l' operazione di lock su  un file non presente, errore!\n");
		return -1;
	}
	int unlockReturnValue=lasciaLockFileScrittura(indiceFile, fdDaElaborare);
	if(unlockReturnValue == -1)
	{
		return -1;
	}
	return 1;
}

int applicaRemove(char *path)
{
	int trovato=0,i=0,daLiberare=0;
	printf("eliminato file\n");

	while(trovato != 1)
	{
		if(strcmp(array_file[i].path, path)==0)
		{
			//elimino il file che ho richiesto
			printf("elimino il file %s\n",array_file[i].path);
			strncpy(array_file[i].path,"vuoto",6);
			array_file[i].data=0;
			daLiberare=array_file[i].dimensione;
			array_file[i].dimensione=0;
			array_file[i].O_LOCK = 0;
			array_file[i].O_CREATE = 0;
			array_file[i].lettoriAttivi=0;
			array_file[i].scrittoriAttivi=0;
			trovato = 1;

			filePiuVecchio=(i+1)%num_max_file;
			numFilePresenti--;
			posizioneLibera= i;
		}
		else
		{
			i=(i+1)%num_max_file;
		}
	}
	numFileDisponibili+=1;
	memoriaDisponibile=memoriaDisponibile+daLiberare;

	return 1;
}


int removeFileServer(char * path, int fdDaElaborare)
{
	int indiceFile=-1;
	int applicaRemoveResult=-1;
	/**
	 * per rimuovere un file verifico che :
	 * -> sia presente
	 * -> sia in lock
	 * -> sia chiuso
	 * -> lo abbia messo io in stato di lock
	 * -> lo chiudo e lo unlock
	 */
	indiceFile=cercaFile(path);
	if( (indiceFile == -1))
	{
		printf("si vuole remove un file non presente, errore!\n");
		return -1;
	}
	if(array_file[indiceFile].O_LOCK != 1)
	{
		printf("non puoi rimuovere un file che non hai lockato! \n");
		return -1;
	}
	if(array_file[indiceFile].identificatoreClient != fdDaElaborare)
	{
		printf("non puoi rimuovere un file che non hai aperto te! \n");
		return -1;
	}
	if(array_file[indiceFile].puntatoreFile !=NULL)
	{
		closeFileServer(path,fdDaElaborare);
	}
	if(array_file[indiceFile].fileAperto != 0)
	{
		printf("non puoi rimuovere un file aperto! \n");
		return -1;
	}

	printf("file da eliminare presente!\n\n\n\n\n\n\n\n");

	applicaRemoveResult = applicaRemove(path);
	if(applicaRemoveResult == -1)
	{
		return -1;
	}
	pthread_cond_signal(&(array_file[indiceFile].fileConditionVariable));
	lasciaLockFileScrittura(indiceFile, fdDaElaborare);



	return 1;

}

int appendToFileServer(char* path,char* buf, size_t size, int fdDaElaborare)
{
	int indiceFile=0;
	indiceFile=cercaFile(path);
	/**
	 * per scrivere su un file, verifico che :
	 * -> il file sia presente in memoria
	 * -> il file sia in stato di lock
	 * -> il file sia posseduto da me
	 * -> per la scrittura richiesta ci sia abbastanza memoria
	 */
	if( (indiceFile == -1))
	{
		printf("si vuole scrivere su un file non presente, errore!\n");
		return -1;
	}
	if(array_file[indiceFile].O_LOCK == 1 && array_file[indiceFile].identificatoreClient != fdDaElaborare)
	{
		printf("non puoi scrivere su un file che non hai lockato! \n");
		return -1;
	}

	if(array_file[indiceFile].O_LOCK == 0)
	{
		assumiLockFileScrittura(indiceFile, fdDaElaborare);
	}


//	size_t modificaDiMemoria=0;
	//è necessario riallocare memoria, ne rialloco tanta quanto appena necessario
//	modificaDiMemoria=size+array_file[indiceFile].dimensione;
	//		realloc(array_file[indiceFile].byteFile, (array_file[indiceFile].dimensione+sizeFile)*sizeof(char));
	if(array_file[indiceFile].byteFile != NULL)
	{
		free(array_file[indiceFile].byteFile);
	}
	array_file[indiceFile].byteFile=malloc(sizeof(char) * (size+array_file[indiceFile].dimensione+1));

//	int espelliFile=0;
	int daSalvare=-1;
	daSalvare=indiceFile;
//	in questo caso per fare spazio al filemi interessa solamente aumentare la memoria disponibile,
//	e non il numero di file, che comunque rimane invariato
	if(memoriaDisponibile<size )
	{
//		espelliFile=1;
		applicaFifo(fdDaElaborare, daSalvare);
	}

	array_file[indiceFile].dimensione=size+array_file[indiceFile].dimensione+1;
	memoriaDisponibile=memoriaDisponibile-(size+array_file[indiceFile].dimensione+10);
	maxMemoriaRaggiunta=maxMemoriaRaggiunta+(size+array_file[indiceFile].dimensione+10);

	fwrite(buf, sizeof(char) , strlen(buf) , array_file[indiceFile].puntatoreFile);
	lasciaLockFileScrittura(indiceFile, fdDaElaborare);
	return 1;
}


int readFileServer(char* path, char * buffer2,size_t *dimFile,int fdDaElaborare)
{
	int indiceFile=0;
	indiceFile=cercaFile(path);
	/**
	 * per leggere un file, verifico che:
	 * -> il file sia presente in memoria
	 * -> il file sia in stato di lock
	 * -> il file sia posseduto da me
	 */
	if( (indiceFile == -1))
	{
		printf("si vuole leggere un file non presente, errore!\n");
//		return "errore";
		return -1;
	}


	if(array_file[indiceFile].O_LOCK == 0 || array_file[indiceFile].identificatoreClient != fdDaElaborare)
	{
		printf("non puoi leggere un file che non hai lockato! \n");
		return -1;
	}

	*dimFile=array_file[indiceFile].dimensione;
	return indiceFile;
}

int readNFileServer(int N,  int fdDaElaborare)
{
//	short tuttiFilePresenti=0;
	int fileLetti=0;
	if(N<=0 || N>= num_max_file)
	{
		//devo leggere tutti i file in quella cartella
//		tuttiFilePresenti=1;
	}


	return fileLetti;

}

int writeFileServer(char* path, char  * dati, size_t sizeFile, int fdDaElaborare)
{
	int indiceFile=0;
	indiceFile=cercaFile(path);
	/**
	 * per scrivere su un file, verifico che :
	 * -> il file sia presente in memoria
	 * -> il file sia in stato di lock
	 * -> il file sia posseduto da me
	 * -> la size del file si abbastanza grande da contenere i nuovi byte
	 */
	if((indiceFile == -1))
	{
		printf("si vuole scrivere nella memoria di un file non presente, errore!\n");
		return -1;
	}

	//verifico che il file che desidero inserire nel server non sia più grande di tutta la memoria
	if(sizeFile>dim_memoria)
	{
		printf("il file che si desidera inserire nella memoria è più grande di tutto lo spazio disponibile\n");
		return -1;
	}
//	size_t modificaDiMemoria=0;

	if((array_file[indiceFile].dimensione+sizeFile)>dim_memoria)
	{
		printf("Non si può allocare questa porzione di memoria\n");
		return -1;
	}
	if(array_file[indiceFile].dimensione<sizeFile)
	{
		//è necessario riallocare memoria, ne rialloco tanta quanto appena necessario
//		modificaDiMemoria=sizeFile+array_file[indiceFile].dimensione;
//		realloc(array_file[indiceFile].byteFile, (array_file[indiceFile].dimensione+sizeFile)*sizeof(char));
		if(array_file[indiceFile].byteFile != NULL)
		{
			free(array_file[indiceFile].byteFile);
		}
		array_file[indiceFile].byteFile=malloc(sizeof(char) * (sizeFile+100));
	}
//	int espelliFile=0;
	int daSalvare=-1;
	daSalvare=indiceFile;
	if(memoriaDisponibile<sizeFile )//|| numFilePresenti == num_max_file)
	{
//		espelliFile=1;
		applicaFifo(fdDaElaborare, daSalvare);
	}

	memcpy(array_file[indiceFile].byteFile , dati, sizeFile);
//	printf("DatiInseriti:%s\n",array_file[indiceFile].byteFile);
	memoriaDisponibile=memoriaDisponibile-sizeFile;
	if((maxMemoriaRaggiunta-array_file[indiceFile].dimensione)<sizeFile)
	{
		maxMemoriaRaggiunta=maxMemoriaRaggiunta-array_file[indiceFile].dimensione+sizeFile;
	}
	array_file[indiceFile].dimensione=sizeFile;
//	printf("grandezzaFile:%ld\n",array_file[indiceFile].dimensione);
//	visualizzaArrayFile();

	return 1;
}


