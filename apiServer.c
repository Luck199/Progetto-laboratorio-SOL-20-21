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
#include <limits.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <sys/stat.h>



#include "comunicazioneClientServer.h"
#include "apiServer.h"
#include "utility.h"
#include "gestioneFile.h"

#define DIM 100
#define UNIX_PATH_MAX 108

char actualpath [PATH_MAX+1];





#define CHECK_FD_SK(S)                                  \
  CHECK_FD;                                             \
  if (!S || strncmp(s\ocketname, S, UNIX_PATH_MAX) != 0) \
  {                                                     \
    errno = EINVAL;                                     \
    return -1;                                          \
  }

#define AINO(code, message, action) \
  if (code == -1)                   \
  {                                 \
    perror(message);                \
    action                          \
  }




int fd_socket = -1;


int statoFileDescriptor()
{
	if (fd_socket < 0)
	{
		    errno = EBADFD;
		    return -1;
	}
	else
	{
		return 0;
	}
}



//funzione che riceve il path relativo di un file e, se termina con successo, ritorna il path assoluto
//altrimenti stampa l' errore riscontrato e ritorna NULL

//utilizza realpath, ricorda che in relazione devi documentare realpath

//**************************
//DEVI DOCUMENTARE REALPATH!!!!!
// *************************

char* relativoToAssoluto(const char *fd)
{


	char *ptr;
	ptr = realpath(fd, actualpath);
	if((ptr== NULL) && (errno==EACCES))
	{
		printf("Errore: l'autorizzazione di lettura o ricerca è stata negata per un componente del prefisso del percorso.\n");
		return NULL;
	}

	else if((ptr== NULL) && (errno==EINVAL))
	{
		printf("Errore: il path risulta NULL.\n");
		return NULL;
	}

	else if((ptr== NULL) && (errno==EIO))
	{
		printf("Errore: si è verificato un errore di I/O durante la lettura dal filesystem.\n");
		return NULL;
	}

	else if((ptr== NULL) && (errno==ELOOP))
	{
		printf("Errore: Sono stati rilevati troppi collegamenti simbolici nella traduzione del percorso.\n");
		return NULL;
	}

	else if((ptr== NULL) && (errno==ENAMETOOLONG))
	{
		printf("Un componente di un percorso ha superato i caratteri NAME_MAX o un intero percorso ha superato i caratteri PATH_MAX. \n");
		return NULL;
	}

	else if((ptr== NULL) && (errno==ENOENT))
	{
		printf("Errore: il file non risulta presente in questa directory.\n");
		return NULL;
	}

	else if((ptr== NULL) && (errno==ENOMEM))
	{
		printf("Errore: fuori dalla memoria.\n");
		return NULL;
	}

	else if((ptr== NULL) && (errno==ENOTDIR))
	{
		printf("Errore: un componente del path prefisso non è una directory\n");
		return NULL;
	}
	else
	{
		//printf("%s\n",actualpath);
	}

	return actualpath;
}







int statoFd=0;
//Api per interagire con il file server


int openConnection(const char *sockname, int msec, const struct timespec abstime)
{

	char * bufferRicezione=NULL;
	//creo il socket e gestisco gli eventuali errori
	fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd_socket == -1 && errno == EINTR)
	{
		perror("SERVER-> un segnale ha interrotto la funzione socket\n");
		return -1;
	}
	else if (fd_socket == -1)
	{
		perror("SERVER-> la funzione socket ha risocntrato un errore\n");
		return -1;
	}

	//setto la struttura sockaddr_un neccessaria per effettuare la connect
	struct sockaddr_un sa;
	strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	//setto la struttura per poter riprovare la connessione in caso di fallimento,
	//per un tempo max di msec millisecondi
	struct timespec request;
	struct timespec remaining;
	request.tv_sec=msec / 1000;
	request.tv_nsec=(msec % 1000) * 1000000L;

	fd_socket=socket(AF_UNIX, SOCK_STREAM, 0);
	while (connect(fd_socket,(struct sockaddr*)&sa, sizeof(sa)) == -1 )
	{
		if (errno == ENOENT)
		{
			time_t current = time(NULL);

			if (current >= abstime.tv_sec)
			{
				//è finito il tempo
				errno = ETIME;
				return -1;
			}

			if(nanosleep(&request,&remaining)<0)
			{
				printf("errore nanosleep\n");
			}
		}
		else
		{
			return -1;
		}
	}

	size_t a=0;
	int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);

	if(readReturnValue > 0)
	{
		if(strncmp(bufferRicezione,"OPEN_CONNECTION: riscontrato errore",36)==0)
		{
			errno=EBADF;
			perror("File descriptor non valido\n");
		}
		else
		{
			//printf("CLIENT-> risposta server: %s\n",bufferRicezione);
		}
	}
	else
	{
		//la read ha riscontrato qualche errore, verifico quale esso sia grazie ad errno
		if ((readReturnValue == -1) && (errno == EAGAIN))
		{
			perror("CLIENT-> Errore: Il file descriptor fd fa riferimento a un file diverso da un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK)\n");
			return -1;
		}

		if ((readReturnValue == -1) && (errno == EBADF))
		{
			perror("CLIENT-> Errore: fd non è un file descriptor valido o non è aperto per la lettura.\n");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EIO))
		{
			perror("CLIENT-> Errore input/output");
			return -1;
		}
		if ((readReturnValue == -1) && ((errno == EAGAIN) || (errno ==EWOULDBLOCK)))
		{
			//perror("CLIENT-> Errore: scrittura in spazio di archiviazione non disponibile!\n");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EFAULT))
		{
			perror("CLIENT-> Errore: bufferRicezione è al di fuori del tuo spazio di indirizzi accessibile.");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EINTR))
		{
			perror("CLIENT-> Errore: la chiamata è stata interrotta da un segnale prima che venissero letti i dati");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EINVAL))
		{
			perror("CLIENT-> Errore: fd è attaccato a un oggetto che non è adatto per la lettura;"
					" oppure il file è stato aperto con il flag O_DIRECT, "
					"e l'indirizzo specificato in buf, il valore "
					"specificato in sizeof(bufferRicezione) o l'offset del file non è adeguatamente allineato.");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EISDIR))
		{
			perror("CLIENT-> Errore: il file descriptor fd fa riferimento ad una directory");
			return -1;
		}
	}

	return 0;
}

//gestito errno
int closeConnection(const char* sockname)
{
	int closeReturnValue = close(fd_socket);
	if ((closeReturnValue == -1) && (errno == EBADF))
	{
		perror("CLIENT-> Errore: file descritor aperto non valido\n");
	}

	if ((closeReturnValue == -1) && (errno == EINTR))
	{
		perror("CLIENT-> Errore: close interrotta da un segnale\n");
	}
	if ((closeReturnValue == -1) && (errno == EIO))
	{
		perror("CLIENT-> Errore input/output");
	}
	if ((closeReturnValue == -1) && ((errno == ENOSPC) || (errno ==EDQUOT)))
	{
		perror("CLIENT-> Errore: scrittura in spazio di archiviazione non disponibile!\n");
	}
	fd_socket = -1;
	printf("CLIENT-> La connessione è stata chiusa correttamente\n");
	return closeReturnValue;
}



//il client chiama la funzione passandogli solo il path name, sarà poi la procedura che lo concatenerà
//con la stringa OPEN_FILE
int openFile(const char* pathname, int flags)
{
	printf("CLIENT-> faccio open File\n");
	char * bufferRicezione=NULL;
	char daInviare[200]="OPEN_FILE;";

	if (flags < 0 || flags > 2)
	{
		errno = EINVAL;
	    return -1;
	}
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}
	size_t a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,strlen(daInviare));

	pathname=relativoToAssoluto(pathname);
	strcpy(daInviare,pathname);
	a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,strlen(daInviare));


	char flags_array[3];
	sprintf(flags_array, "%d", flags);
	strcpy(daInviare,flags_array);
	a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,strlen(daInviare));




//	size_t b=strlen(daInviare);
//	inviaDati(fd_socket,&b,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,strlen(daInviare));

	int exit=0;
	char * pathEspulso=NULL;
	char * datiEspulsi=NULL;
	int entrato=0;
	while(exit!=1)
	{
		int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
	//	printf("bufferRicezioneOPEN:%s\n",bufferRicezione);

		if(readReturnValue>0)
		{
			if(strncmp(bufferRicezione,"OPEN_FILE: riscontrato errore",50)==0)
			{
				errno=EBADF;
				perror("File descriptor non valido\n");

				return -1;
			}
			if(strncmp(bufferRicezione,"OPEN_FILE eseguita correttamente!",34)==0)
			{
				exit=1;
				free(bufferRicezione);
//				printf("esco\n");
			}
			else
			{
				if(entrato==0)
				{
					pathEspulso=malloc(sizeof(char)*a);
					strncpy(pathEspulso, bufferRicezione,a);
					free(bufferRicezione);
					entrato=1;
				}
				else
				{
					datiEspulsi=malloc(sizeof(char)*a);
					memcpy(datiEspulsi, bufferRicezione, a);
					exit=1;
					free(bufferRicezione);
				}
//
//
			}
		}
		else
		{
			printf("Client -> Errore open\n");
		}
	//
	}


	if(pathEspulso!=NULL || datiEspulsi!=NULL)
	{
//		printf("\n\n\n\nIN OPEN FILE:path espulso: %s \nDati espulsi:%s\n\n\n\n",pathEspulso,datiEspulsi);
	}
	return 0;
}

int readFile(const char* pathname, void** buf, size_t* size)
{
	const char puntoVirgola[2] = ";";

	char *bufferRicezione=NULL;
	char daInviare[200]="READ_FILE;";
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}
	pathname=relativoToAssoluto(pathname);
	strncat(daInviare,pathname,strlen(pathname)+1);

	strncat(daInviare,puntoVirgola,2);


	size_t a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,&daInviare,a);


	int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
	if(errno==EINVAL)
	{
		printf("\n\n\n\neccolo!\n\n\n");
	}
	if(readReturnValue > 0)
	{
		printf("CLIENT-> risposta server: %s\n",bufferRicezione);
		FILE *file;
		//
//		if(errno==0)
		{
			printf("faccio fopen\n\n\n\n\n");
			file = fopen(pathname,"w");
			if( file==NULL )
			{
				perror("Errore in apertura del file");
				return -1;
			}
			else
			{
				int w = fwrite(bufferRicezione, sizeof(char), a, file);
				if(w<0)
				{
					printf("CLIENT -> ERRORE fwrite\n");
				}
			}
			fclose(file);
		}
	}
	else
	{
		perror("CLIENT->Errore READ");
		return -1;
	}



	return 0;
}

int readNFiles(int N, const char* dirname)
{
	char * bufferRicezione=NULL;
	char daInviare[200]="READ_N_FILE;";
	char appoggio[20];
	int numFile=0;
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}
	size_t b=strlen(daInviare);
	inviaDati(fd_socket,&b,sizeof(size_t));
	inviaDati(fd_socket,&daInviare,b);
	sprintf(appoggio, "%d", N);

	strcpy(daInviare,appoggio);

	printf("\n\nda inviare: %s \n\n",daInviare);



	size_t a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,&daInviare,a);
	a=0;
	short exit=0;
	int readReturnValue=0;
	short leggoPath=0;
	FILE *file;
	char *datiLetti=NULL;
	char *pathLetto=NULL;
	while(exit!=1)
	{
		if(bufferRicezione != NULL)
		{
			free(bufferRicezione);
		}
		a=0;
		readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
		printf("a:%ld\n",a);
		printf("bufferRicezione:%s\n",bufferRicezione);
		if(readReturnValue>0)
		{
			if(strncmp(bufferRicezione,"READ_N_FILE: riscontrato errore",32)==0)
			{
				errno=EBADF;
				perror("File descriptor non valido\n");
				return -1;
			}
			else if(strncmp(bufferRicezione,"READ_N_FILE: eseguita operazione",32)==0)
			{
				//errno=EBADF;
				printf("FINE READ_N_FILE!\n");
				exit=1;
			}
			else
			{
				if((leggoPath==0))
				{
					printf("a:%ld\n",a);
//					if(pathLetto != NULL)
//					{
//						free(pathLetto);
//					}

					pathLetto=malloc(sizeof(char)*a);
					strncpy(pathLetto,bufferRicezione,a);
//					free(bufferRicezione);
					leggoPath=1;
					a=0;
				}
				else if(leggoPath==1)
				{
//					if(datiLetti != NULL)
//					{
//						free(datiLetti);
//					}
					datiLetti=malloc(sizeof(char)*a);
					memcpy(datiLetti,bufferRicezione,a);
					if(errno==0)
					{
						printf("faccio fopen\n\n\n\n\n");

						file = fopen(pathLetto,"w");
						if( file==NULL )
						{
							perror("Errore in apertura del file");
							return -1;
						}
						else
						{
							int w = fwrite(datiLetti, sizeof(char), a, file);
							if(w<0)
							{
								printf("CLIENT -> ERRORE fwrite\n");
							}
						}
						fclose(file);
						numFile++;
					}

					leggoPath=0;
//
				}
			}
		}
//		else
//		{
//			printf("Client -> Errore READ_N_FILE\n");
//		}

	}

	free(datiLetti);
	free(pathLetto);



	return numFile;
}


int writeFile(const char* pathname, const char* dirName)
{
	char daInviare[200]="WRITE_FILE;";
	char *bufferRicezione=NULL;
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}
	char *directoryCorrente=NULL;
	size_t sizeDirectoryCorrente=150;
	directoryCorrente=malloc(sizeof(char)*sizeDirectoryCorrente);
	directoryCorrente=getcwd(directoryCorrente,sizeDirectoryCorrente);
//	printf("lavoro nella directory:%s\n",directoryCorrente);
	dirName=relativoToAssoluto(dirName);
//	printf("la cartella che utilizzo è:%s\n",dirName);

	char * path2="";
	path2=relativoToAssoluto(pathname);


	//recupero il file da poter inviare
	void *buf = NULL;
	size_t size = 0;

	int recuperaFileReturnValue=0;
	recuperaFileReturnValue=recuperaFile(path2, &buf, &size);
	if(recuperaFileReturnValue == -1)
	{
		printf("Errore nel recupero del file\n ");
		return -1;
	}


	size_t a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,strlen(daInviare));



	strncpy(daInviare,path2,strlen(path2));
//	char size_array[10];
//	sprintf(size_array, "%ld", size);
//	strncat(daInviare,";",2);
//	strncat(daInviare,size_array,strlen(size_array));

	size_t b=strlen(daInviare);
	inviaDati(fd_socket,&b,sizeof(size_t));
	inviaDati(fd_socket,daInviare,b);


	//writen(fd_socket,(void*)buf,1000);
	inviaDati(fd_socket,&size, sizeof(size_t));
	inviaDati(fd_socket,buf, size);







	size_t dimBufferRicezione=0;
	int exit=0;
	char * pathEspulso=NULL;
	char * datiEspulsi=NULL;
	int entrato=0;
//	printf("CLIENT -> ENTRO NEL WHILE!!\n");

	while(exit!=1)
	{
		a=0;
		dimBufferRicezione=0;
		int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&dimBufferRicezione);
		printf("dimBufferRicezione:%ld\n",dimBufferRicezione);
		printf("CLIENT -> BUFFERRicezioneWRITE:%s\n\n\n\n\n",bufferRicezione);

		if(readReturnValue>0)
		{
			if(strncmp(bufferRicezione,"WRITE_FILE: riscontrato errore",30)==0)
			{
				errno=EBADF;
				return -1;
			}
			if(strncmp(bufferRicezione,"WRITE_FILE eseguita correttamente!",35)==0)
			{
				exit=1;
				free(bufferRicezione);
				printf("CLIENT -> esco\n");
			}
			else
			{
				if(entrato==0)
				{
					pathEspulso=malloc(sizeof(char)*dimBufferRicezione);
					strncpy(pathEspulso, bufferRicezione,dimBufferRicezione);
					free(bufferRicezione);
					entrato=1;
				}
				else
				{

					datiEspulsi=malloc(sizeof(char)*dimBufferRicezione+1);
					memcpy(datiEspulsi, bufferRicezione, dimBufferRicezione);
//					exit=1;
					free(bufferRicezione);
				}
			}
		}
		else
		{
			printf("Client -> Errore close\n");
		}
	}
//	printf("ESCO DAL WHILE!\n");
	if(pathEspulso!=NULL)
	{
//		printf("\nIN WRITE FILE:path espulso: %s \n",pathEspulso);
	}
	if(datiEspulsi != NULL)
	{
//		printf("Dati espulsi:%s\n\n\n\n",datiEspulsi);
	}
//	printf("fine OPerazione\n");


//	int chdirReturnValue=0;
//	chdirReturnValue=chdir(dirName);
//	if(chdirReturnValue != 0)
//	{
//		//è stato settato errno
//		perror("Errore nell' utilizzo di chdir\n");
//		return -1;
//	}
	FILE *file;
	if(errno!=0 && datiEspulsi != NULL && pathEspulso != NULL)
	{
		printf("faccio fopen\n\n\n\n\n");
		file = fopen(pathEspulso,"w");
		if( file==NULL )
		{
			perror("Errore in apertura del file");
			return -1;
		}
		else
		{
			int w = fwrite(datiEspulsi, sizeof(char), a, file);
			if(w<0)
			{
				printf("CLIENT -> ERRORE fwrite\n");
			}
		}
		fclose(file);
	}

//	chdirReturnValue=chdir(directoryCorrente);
//	if(chdirReturnValue != 0)
//	{
//		//è stato settato errno
//		perror("Errore nell' utilizzo di chdir\n");
//		return -1;
//	}






























//	int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
//	if(readReturnValue > 0)
//	{
//		printf("CLIENT-> risposta server: %s\n",bufferRicezione);
//	}
//	else
//	{
//		//perror("CLIENT->Errore READ");
//		return -1;
//	}
//	free(stringaFile);
	return 0;
}


int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
	char daInviare[200]="APPEND_TO_FILE;";
	char *bufferRicezione=NULL;
	statoFd=statoFileDescriptor();

	if(statoFd < 0)
	{
		return -1;
	}
	size_t b=strlen(daInviare);
	inviaDati(fd_socket,&b,sizeof(size_t));
	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strcpy(daInviare,pathname);


	size_t d=strlen(daInviare);
	inviaDati(fd_socket,&d,sizeof(size_t));
	inviaDati(fd_socket,daInviare,d);

	//strcat(daInviare,";");
	strcpy(daInviare,buf);
	char size_array[10];
	sprintf(size_array, "%ld", size);
	strcat(daInviare,";");
	strcat(daInviare,size_array);



	size_t a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);

	int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
	if(readReturnValue > 0)
	{
	//	printf("tutto ok\n");
		printf("CLIENT-> risposta server: %s\n",bufferRicezione);
	}
	else
	{
		printf("CLIENT -> errore in operazione lock \n");
	}
	return 1;
}

int lockFile(const char* pathname)
{
	int readReturnValue=0;
	char *bufferRicezione=NULL;
	char daInviare[200]="LOCK_FILE;";
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}
	size_t b=strlen(daInviare);
	inviaDati(fd_socket,&b,sizeof(size_t));
	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strncpy(daInviare,pathname,strlen(pathname));




	size_t a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);







	//write(fd_socket,daInviare,200);
	readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
	if(readReturnValue > 0)
	{
		printf("CLIENT-> risposta server per file %s: %s\n",pathname,bufferRicezione);
	}
	else
	{
		printf("CLIENT -> errore in operazione lock \n");
	}
	return 0;
}

int unlockFile(const char* pathname)
{
	int readReturnValue=0;
		char *bufferRicezione=NULL;
		char daInviare[200]="UNLOCK_FILE;";
		statoFd=statoFileDescriptor();
		if(statoFd < 0)
		{
			return -1;
		}

		size_t b=strlen(daInviare);
		inviaDati(fd_socket,&b,sizeof(size_t));
		inviaDati(fd_socket,daInviare,b);

		pathname=relativoToAssoluto(pathname);
		strncpy(daInviare,pathname,strlen(pathname));




		size_t a=strlen(daInviare);
		inviaDati(fd_socket,&a,sizeof(size_t));
		inviaDati(fd_socket,daInviare,a);







		//write(fd_socket,daInviare,200);
		readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
		if(readReturnValue > 0)
		{
			printf("CLIENT-> risposta server per file %s: %s\n",pathname,bufferRicezione);
		}
		else
		{
			printf("CLIENT -> errore in operazione lock \n");
		}
		return 0;
}

int closeFile(const char* pathname)
{
	char daInviare[200]="CLOSE_FILE;";
	char * bufferRicezione= NULL;
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}

	size_t b=strlen(daInviare)+1;
	inviaDati(fd_socket,&b,sizeof(size_t));
	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strcpy(daInviare,pathname);

	size_t a=strlen(daInviare)+1;
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);

	a=0;
	int	readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);

	if(readReturnValue > 0)
	{
		if(strncmp(bufferRicezione,"CLOSE_FILE: riscontrato errore",50))
		{
//			errno=EBADF;
//			perror("File descriptor non valido\n");
			printf("CLIENT-> risposta server: %s\n",bufferRicezione);
			return -1;

		}
		else
		{
			printf("CLIENT-> risposta server: %s\n",bufferRicezione);
		}
	}
	else
	{
		//la read ha riscontrato qualche errore, verifico quale esso sia grazie ad errno
		if ((readReturnValue == -1) && (errno == EAGAIN))
		{
			perror("CLIENT-> Errore: Il file descriptor fd fa riferimento a un file diverso da un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK)\n");
			return -1;
		}

		if ((readReturnValue == -1) && (errno == EBADF))
		{
			perror("CLIENT-> Errore: fd non è un file descriptor valido o non è aperto per la lettura.\n");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EIO))
		{
			perror("CLIENT-> Errore input/output");
			return -1;
		}
		if ((readReturnValue == -1) && ((errno == EAGAIN) || (errno ==EWOULDBLOCK)))
		{
			//perror("CLIENT-> Errore: scrittura in spazio di archiviazione non disponibile!\n");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EFAULT))
		{
			perror("CLIENT-> Errore: bufferRicezione è al di fuori del tuo spazio di indirizzi accessibile.");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EINTR))
		{
			perror("CLIENT-> Errore: la chiamata è stata interrotta da un segnale prima che venissero letti i dati");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EINVAL))
		{
			perror("CLIENT-> Errore: fd è attaccato a un oggetto che non è adatto per la lettura;"
					" oppure il file è stato aperto con il flag O_DIRECT, "
					"e l'indirizzo specificato in buf, il valore "
					"specificato in sizeof(bufferRicezione) o l'offset del file non è adeguatamente allineato.");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EISDIR))
		{
			perror("CLIENT-> Errore: il file descriptor fd fa riferimento ad una directory");
			return -1;
		}
	}
	return 0;
}

int removeFile(const char* pathname)
{
	int readReturnValue=0;
	char *bufferRicezione=NULL;
	char daInviare[200]="REMOVE_FILE;";
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}
	size_t b=strlen(daInviare);
	inviaDati(fd_socket,&b,sizeof(size_t));
	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strncpy(daInviare,pathname,strlen(pathname));

	size_t a=strlen(daInviare);
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);

	//write(fd_socket,daInviare,200);
	readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
	if(readReturnValue > 0)
	{
		printf("CLIENT-> risposta server per file %s: %s\n",pathname,bufferRicezione);
	}
	else
	{
		printf("CLIENT -> errore in operazione remove \n");
	}
	return 0;
}


int recuperaFile(const char *path, void **fileBuffer, size_t *size)
{

  FILE *filePointer = NULL;
  *fileBuffer = NULL;

  //printf("path:%s\n\n\n",path);
  // control flow flags
  int error = 0;
  int chiudi = 0;
  int freeBuf = 0;

  // open the file
  filePointer = fopen(path, "r");
  if(filePointer == NULL)
  {
	  perror("SERVER -> Error fopen");
	  return -1;
  }
  // go to the end of the file
  fseek(filePointer, 0L, SEEK_END);


  //mi salvo la grandezza del file
  *size = ftell(filePointer);

  //riporto il puntatore del file all' inizio di esso
  errno = 0;
  rewind(filePointer);
  if (errno)
  {
	  perror("Client -> ERRORE OPERAZIONE REWIND");
  }


  //alloco lo spazio necessario

  *fileBuffer = malloc(sizeof(char) * (*size));
  //fare controllo se sono riuscito ad allocare abbastanza memoria

  //inserisco il file nel buffer
  if (!error)
  {
	  int readSize = fread(*fileBuffer, sizeof(char), *size, filePointer);
	  if (readSize < *size)
	  {
		  perror("readLocalFile internal error: fread");
		  return -1;
	  }

	  chiudi = 1;
  }

  if (chiudi)
  {
	  errno = 0;
	  fclose(filePointer);
	  if (errno)
	  {
		  perror("Client -> ERRORE OPERAZIONE FCLOSE");
		  return -1;
	  }
  }

  if (freeBuf)
  {
	  free(*fileBuffer);
	  *fileBuffer = NULL;
  }

  //printf("successfully read the file %s from disk of size %zd\n", path, *size);
  return 0;

}


int isCurrentDirOrParentDir(char *nomeDirectory)
{
	if (strcmp(nomeDirectory, ".") == 0)
	{
		return 1;
	}
	else if(strcmp(nomeDirectory, "..") == 0)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}


//Funzione che legge N file da una directory
//nel caso in cui siano presenti sottoDirectory, le visita ricorsivamente fino al raggiungimento di N
 int leggiNFileDaDirectory(int *numFile2,const char *dirName, char** arrayPath, int posizioneArray, short bitConteggio, int *numeroFileLetti)
{
		if(numFile2 == NULL)
		{
			perror("ERRORE è stato passato alla funzione un valore non valido");
			return -1;
		}


		int leggiTuttiIFile = *numFile2 <= 0;
		// open the dir
		DIR *dir = opendir(dirName);
		if(dir==NULL)
		{
			perror("ERRORE nella funzione openDir\n");
			return -1;
		}

		// Eseguo operazione cd nella directory selezionata
		int chDirReturnValue=0;
		chDirReturnValue=chdir(dirName);
		if(chDirReturnValue==-1 && errno!=0)
      	{
      		perror("ERRORE nella funzione chdir\n");
      	}


		struct dirent *file = NULL;
		//Leggo ogni entry presente nella directory fino a che non ho letto tutti i file oppure ho raggiunto il limite
		while ((leggiTuttiIFile || *numFile2) && (file = readdir(dir)) != NULL)
		{
			char *filename = file->d_name;
			struct stat s;
			stat(filename, &s);

			//  stat(filename, &s);


			int isFileCurrentDir = isCurrentDirOrParentDir(filename);
			// int isFileParentDir = isCurrentDir(filename);
			//**************************
			//SCRIVERE IN RELAZIONE CHE UTILIZZO STAT CHE NON é POSIX!!!
			//******************************
			int isDirectory = S_ISDIR(s.st_mode);
			int isFileRegolare = S_ISREG(s.st_mode);

			//Tramite questi tre if, se ho selezionato un file
			//speciale lo salto, non considerandolo nel conteggio
			if (isFileCurrentDir == 1 || isFileCurrentDir == 2)
			{
				continue;
			}
			else if (!isDirectory && !isFileRegolare)
			{
				//se entro dentro questo if significato che
				//grazie all' utilizzo della struttura stat,
				//sono riuscito ad identificare che il file considerato in questo momento non
				//risulta un file regolare e nemmeno una directory
				continue;
			}





			else if (isDirectory)
			{
				int chiamataRicorsivaReturnValue = leggiNFileDaDirectory( numFile2,filename, arrayPath,posizioneArray,bitConteggio, numeroFileLetti);
				if (chiamataRicorsivaReturnValue == -1)
				{
					int closeDirReturnValue=0;
					closeDirReturnValue=closedir(dir);
					if(closeDirReturnValue==-1 && errno!=0)
					{
						perror("ERRORE nella funzione closedir\n");
					}
				}
				else
				{
					//Ritorno nella directory che sto elaborando
					// cd in the current dir again

					int chdirReturnValue=0;

					chdirReturnValue=chdir("..");
					if(chdirReturnValue==-1 && errno!=0)
					{
						perror("ERRORE nella funzione closedir\n");
					}

				}
			}
			else if (isFileRegolare)
			{

				if(bitConteggio == 0)
				{
					(*numeroFileLetti)++;
				}
				else
				{
					int lunghezza=strlen(relativoToAssoluto(filename))+1;
					char *path =NULL;//Sistemare il discorso della free
					path=malloc(sizeof(char)*lunghezza);
					path=relativoToAssoluto(filename);
					if(strcmp(arrayPath[posizioneArray],"")!=0)
					{
						posizioneArray++;
					}
					printf("path:%s\n",path);
					strcpy(arrayPath[posizioneArray],path);

				//	free(path);



				}
				if (!leggiTuttiIFile)
				{
					(*numFile2)--;
				}


			}
		}


		closedir(dir);
		return 0;


}

