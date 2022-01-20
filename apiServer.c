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


int fd_socket = -1;

int abilitaStampe=0;
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


char* relativoToAssoluto(const char *fd)
{
	char *ptr;
	ptr = realpath(fd, actualpath);
	if((ptr== NULL) && (errno==EACCES))
	{
		if(abilitaStampe==1)
		{
			printf("Errore: l'autorizzazione di lettura o ricerca è stata negata per un componente del prefisso del percorso.\n");
		}
		return NULL;
	}

	else if((ptr== NULL) && (errno==EINVAL))
	{
		if(abilitaStampe==1)
		{
			printf("Errore: il path risulta NULL.\n");
		}
		return NULL;
	}

	else if((ptr== NULL) && (errno==EIO))
	{
		if(abilitaStampe==1)
		{
			printf("Errore: si è verificato un errore di I/O durante la lettura dal filesystem.\n");
		}
		return NULL;
	}

	else if((ptr== NULL) && (errno==ELOOP))
	{
		if(abilitaStampe==1)
		{
			printf("Errore: Sono stati rilevati troppi collegamenti simbolici nella traduzione del percorso.\n");
		}
		return NULL;
	}

	else if((ptr== NULL) && (errno==ENAMETOOLONG))
	{
		if(abilitaStampe==1)
		{
			printf("Un componente di un percorso ha superato i caratteri NAME_MAX o un intero percorso ha superato i caratteri PATH_MAX. \n");
		}
		return NULL;
	}

	else if((ptr== NULL) && (errno==ENOENT))
	{
		if(abilitaStampe==1)
		{
			printf("Errore: il file non risulta presente in questa directory.\n");
		}
		return NULL;
	}

	else if((ptr== NULL) && (errno==ENOMEM))
	{
		if(abilitaStampe==1)
		{
			printf("Errore: fuori dalla memoria.\n");
		}
		return NULL;
	}

	else if((ptr== NULL) && (errno==ENOTDIR))
	{
		if(abilitaStampe==1)
		{
			printf("Errore: un componente del path prefisso non è una directory\n");
		}
		return NULL;
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

	if(readReturnValue > 0 && errno == 0)
	{
		if(strncmp(bufferRicezione,"OPEN_CONNECTION: riscontrato errore",36)==0)
		{
			errno=EBADF;
			perror("Fd non valido\n");
		}
		else
		{
			if(abilitaStampe==1)
			{
				printf("CLIENT-> risposta server: %s\n",bufferRicezione);
			}

		}
	}
	else
	{
		//la read ha riscontrato qualche errore, verifico quale esso sia grazie ad errno
		if ((readReturnValue == -1) && (errno == EAGAIN) && (abilitaStampe == 1 ))
		{
			perror("CLIENT-> Errore: Il file descriptor fd fa riferimento a un file diverso da un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK)\n");
			return -1;
		}

		if ((readReturnValue == -1) && (errno == EBADF) && (abilitaStampe == 1 ))
		{
			perror("CLIENT-> Errore: fd non è un file descriptor valido o non è aperto per la lettura.\n");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EIO) && (abilitaStampe == 1 ))
		{
			perror("CLIENT-> Errore input/output");
			return -1;
		}
		if ((readReturnValue == -1) && ((errno == EAGAIN) || (errno ==EWOULDBLOCK)))
		{
			//perror("CLIENT-> Errore: scrittura in spazio di archiviazione non disponibile!\n");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EFAULT) && (abilitaStampe == 1 ))
		{
			perror("CLIENT-> Errore: bufferRicezione è al di fuori del tuo spazio di indirizzi accessibile.");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EINTR) && (abilitaStampe == 1 ))
		{
			perror("CLIENT-> Errore: la chiamata è stata interrotta da un segnale prima che venissero letti i dati");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EINVAL) && (abilitaStampe == 1 ))
		{
			perror("CLIENT-> Errore: fd è attaccato a un oggetto che non è adatto per la lettura;"
					" oppure il file è stato aperto con il flag O_DIRECT, "
					"e l'indirizzo specificato in buf, il valore "
					"specificato in sizeof(bufferRicezione) o l'offset del file non è adeguatamente allineato.");
			return -1;
		}
		if ((readReturnValue == -1) && (errno == EISDIR) && (abilitaStampe == 1 ))
		{
			perror("CLIENT-> Errore: il file descriptor fd fa riferimento ad una directory");
			return -1;
		}
	}

	return 0;
}


int closeConnection(const char* sockname)
{
	//effettuo la chiusura della connessione, grazie all'operazione close sul socket
	int closeReturnValue = close(fd_socket);
	if ((closeReturnValue == -1) && (errno == EBADF))
	{
		perror("CLIENT-> Errore: file descriptor aperto non valido\n");
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
	if(abilitaStampe == 1 )
	{
		printf("CLIENT-> La connessione è stata chiusa correttamente\n");
	}
	return closeReturnValue;
}


int openFile(const char* pathname, int flags)
{
//	printf("CLIENT-> faccio open File\n");
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
//	inviaDati(fd_socket,&a,sizeof(size_t));
//
//
//	inviaDati(fd_socket,daInviare,strlen(daInviare));

	size_t op=100;
	writen(fd_socket,&op,sizeof(op));


	pathname=relativoToAssoluto(pathname);
	if(pathname == NULL)
	{
		return -1;
	}
//printf("CLIENT -> pathname:%s\n",pathname);
	strcpy(daInviare,pathname);
	a=strlen(daInviare)+1;
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);


//	char flags_array[3];
//	sprintf(flags_array, "%d", flags);
//	strcpy(daInviare,flags_array);
//	a=strlen(daInviare);
//	inviaDati(fd_socket,&a,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,strlen(daInviare));

	writen(fd_socket,&flags,sizeof(flags));


	int exit=0;
	char * pathEspulso=NULL;
	char * datiEspulsi=NULL;
	int entrato=0;
	while(exit!=1)
	{
		int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
		if(readReturnValue>0)
		{
			if(strncmp(bufferRicezione,"OPEN_FILE: riscontrato errore",50)==0)
			{
				errno=EBADF;
//				perror("File descriptor non valido\n");

				return -1;
			}
			if(strncmp(bufferRicezione,"OPEN_FILE eseguita correttamente!",34)==0)
			{
				exit=1;
				if(abilitaStampe==1)
				{
					printf("OPEN_FILE eseguita correttamente!\n");
				}
				free(bufferRicezione);
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
					free(bufferRicezione);
				}
			}
		}
		else
		{
			if(abilitaStampe == 1 )
			{
				printf("Client -> Errore open\n");
			}
			return -1;
		}
	//
	}
	if(pathEspulso!=NULL)
	{
		printf("pathEspulso:%s\ndatiEspulsi:%s\n",pathEspulso,datiEspulsi);
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
	size_t op=104;
	writen(fd_socket,&op,sizeof(op));

	strncpy(daInviare,pathname,strlen(pathname)+1);

//	strncat(daInviare,puntoVirgola,2);


	size_t a=strlen(daInviare)+1;
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,&daInviare,a);
	a=0;

	int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
//	printf("buffericezione:%s",bufferRicezione);
	if(strcmp(bufferRicezione,"errore")==0)
	{
		printf("arrivato errore\n");
		return -1;
	}
//	if(errno==EINVAL)
//	{
//		return -1;
//	}
	if(readReturnValue > 0)
	{
		printf("CLIENT-> risposta server: %s\n",bufferRicezione);
		FILE *file;
//		if(errno==0)
		{
			printf("faccio fopen\n\n\n\n\n");
			file = fopen("foto.jpg","w");
			if( file==NULL )
			{
				perror("Client -> Errore in apertura del file");
				return -1;
			}
			else
			{

				int w = fwrite(bufferRicezione, sizeof(char), a, file);
				if(w<0)
				{
					perror("CLIENT -> ERRORE fwrite\n");
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
	char * dirNameSalvato=NULL;
	char appoggio[20];
	int numFile=0;
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}

	size_t op=105;
	writen(fd_socket,&op,sizeof(op));

	char *directoryCorrente=NULL;
	size_t sizeDirectoryCorrente=150;
	directoryCorrente=malloc(sizeof(char)*sizeDirectoryCorrente);
	directoryCorrente=getcwd(directoryCorrente,sizeDirectoryCorrente);
//	printf("READNFILE: lavoro nella directory:%s\n",directoryCorrente);
	if(dirname != NULL)
	{
		dirname=relativoToAssoluto(dirname);
		//	printf("la cartella che utilizzo è:%s\n",dirname);
		dirNameSalvato=malloc(sizeof(char)*(strlen(dirname)+1));
		strncpy(dirNameSalvato,dirname,strlen(dirname)+1);
	}
	printf("dirname:%s\n",dirname);




//	size_t b=strlen(daInviare);
//	inviaDati(fd_socket,&b,sizeof(size_t));
//	inviaDati(fd_socket,&daInviare,b);
//	sprintf(appoggio, "%d", N);

	strcpy(daInviare,appoggio);



	size_t a=strlen(daInviare);
//	inviaDati(fd_socket,&a,sizeof(size_t));
//	inviaDati(fd_socket,&daInviare,a);
	writen(fd_socket,&N,sizeof(N));
	a=0;
	short exit=0;
	int readReturnValue=0;
	short leggoPath=0;
//	FILE *file;
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
		if(readReturnValue>0 )
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

//					if(errno==0)
					{
						FILE *file;
//						if(errno=0 && datiEspulsi != NULL && pathEspulso != NULL)
						{
							int chdirReturnValue=0;
							chdirReturnValue=chdir(dirNameSalvato);
							if(chdirReturnValue != 0)
							{
								//è stato settato errno
								perror("Errore nell' utilizzo di chdir\n");
								return -1;
							}
							char* token1;
							char * pathRelativo;
							char* rest = pathLetto;

							while ((token1 = strtok_r(rest, "/", &rest))!=NULL)
							{
								pathRelativo=token1;

							}

							file = fopen(pathRelativo,"w");
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
							chdirReturnValue=chdir(directoryCorrente);
							if(chdirReturnValue != 0)
							{
								//è stato settato errno
								perror("Errore nell' utilizzo di chdir\n");
								return -1;
							}

						}
						numFile++;
					}

					leggoPath=0;
//
				}
			}
		}
		else
		{
			if(abilitaStampe==1)
			{
				printf("Client -> Errore READ_N_FILE\n");
			}
			return -1;
		}

	}

	free(datiLetti);
	free(pathLetto);



	return numFile;
}


int writeFile(const char* pathname, const char* dirname)
{
	char daInviare[200]="WRITE_FILE;";
	char *bufferRicezione=NULL;
	char * dirNameSalvato=NULL;
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
	if(dirname != NULL)
	{
		dirname=relativoToAssoluto(dirname);
		dirNameSalvato=malloc(sizeof(char)*(strlen(dirname)+1));
		strncpy(dirNameSalvato,dirname,strlen(dirname)+1);
	}
	printf("CLIENT -> dirname:%s\n",dirname);
	char * path2="";
//	printf("path2:%s\n",path2);
	path2=relativoToAssoluto(pathname);


	//recupero il file da poter inviare
	void *buf = NULL;
	size_t size = 0;

	int recuperaFileReturnValue=0;
	recuperaFileReturnValue=recuperaFile(path2, &buf, &size);
	if(recuperaFileReturnValue == -1)
	{
		if(abilitaStampe == 1 )
		{
			printf("Errore nel recupero del file\n ");
		}
		return -1;
	}
//printf("client-> size file:%ld\nbufFIle:%s\n",size,buf);
	size_t op=106;
	size_t a=0;
//	size_t a=strlen(daInviare);
//	inviaDati(fd_socket,&a,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,strlen(daInviare));
	writen(fd_socket,&op,sizeof(op));


	strncpy(daInviare,path2,strlen(path2));
	//	char size_array[10];
	//	sprintf(size_array, "%ld", size);
	//	strncat(daInviare,";",2);
	//	strncat(daInviare,size_array,strlen(size_array));

	size_t b=strlen(daInviare)+1;
	inviaDati(fd_socket,&b,sizeof(size_t));
	inviaDati(fd_socket,daInviare,b);
	size=size+1;
//	printf("Client->size:%ld\n",size);
//	printf("CLIENT -> buf:%s\n",buf);
	inviaDati(fd_socket,&size,sizeof(size_t));
	inviaDati(fd_socket,buf, size);


//	size_t dimBufferRicezione=0;
	int exit=0;
	char * pathEspulso=NULL;
	char * datiEspulsi=NULL;
	int entrato=0;
	size_t dimEspulsi=0;

	while(exit!=1)
	{
		int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);

		if(readReturnValue>0)
		{
			if(strncmp(bufferRicezione,"WRITE_FILE: riscontrato errore",30)==0)
			{
				errno=EBADF;
				printf("Esco Con Errore\n" );
				return -1;

			}
			if(strncmp(bufferRicezione,"WRITE_FILE eseguita correttamente!",35)==0)
			{
				exit=1;
				free(bufferRicezione);
			}
			else
			{
				if(entrato==0)
				{
					dimEspulsi=a;
					pathEspulso=malloc(sizeof(char)*dimEspulsi);
					strncpy(pathEspulso, bufferRicezione,dimEspulsi);
					free(bufferRicezione);
					entrato=1;
				}
				else
				{
					dimEspulsi=a;
					datiEspulsi=malloc(sizeof(char)*dimEspulsi);
					memcpy(datiEspulsi, bufferRicezione, dimEspulsi);
					free(bufferRicezione);
				}
			}
		}
		else
		{
			if((abilitaStampe == 1 ))
			{
				printf("Client -> Errore write\n");
			}
			return -1;
		}
	}

	FILE *file;
	if(errno==0 && datiEspulsi != NULL && pathEspulso != NULL && dirname!=NULL)
	{
		int chdirReturnValue=0;
		chdirReturnValue=chdir(dirNameSalvato);
		if(chdirReturnValue != 0)
		{
			//è stato settato errno
			perror("Errore nell' utilizzo di chdir\n");
			return -1;
		}
		char* token;
		char * pathRelativo;
		char* rest = pathEspulso;

		while ((token = strtok_r(rest, "/", &rest))!=NULL)
		{
			pathRelativo=token;
		}

		file = fopen(pathRelativo,"w");
		if( file==NULL )
		{
			perror("Errore in apertura del file");
			return -1;
		}
		else
		{
			int w = fwrite(datiEspulsi, sizeof(char), dimEspulsi, file);
			if(w<0)
			{
				printf("CLIENT -> ERRORE fwrite\n");
			}
		}
		fclose(file);
		chdirReturnValue=chdir(directoryCorrente);
		if(chdirReturnValue != 0)
		{
			//è stato settato errno
			perror("Errore nell' utilizzo di chdir\n");
			return -1;
		}

	}
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
	size_t op=107;

	writen(fd_socket,&op,sizeof(op));


	char *directoryCorrente=NULL;
	size_t sizeDirectoryCorrente=150;
	directoryCorrente=malloc(sizeof(char)*sizeDirectoryCorrente);
	directoryCorrente=getcwd(directoryCorrente,sizeDirectoryCorrente);
//	dirname=relativoToAssoluto(dirname);
	char * dirNameSalvato=malloc(sizeof(char)*(strlen(dirname)+1));
	strncpy(dirNameSalvato,dirname,strlen(dirname)+1);



	size_t b=strlen(daInviare);
//	inviaDati(fd_socket,&b,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strcpy(daInviare,pathname);


	size_t d=strlen(daInviare)+1;
	inviaDati(fd_socket,&d,sizeof(size_t));
	inviaDati(fd_socket,daInviare,d);

	//strcat(daInviare,";");
//	printf("CLIENT -> BUFFER:%s\n\n\n\n",buf);
	strcpy(daInviare,buf);
	d=strlen(buf)+1;
	inviaDati(fd_socket,&d,sizeof(size_t));
	inviaDati(fd_socket,daInviare,d);


	size+=1;
	writen(fd_socket,&size,sizeof(size));
//	char size_array[10];
//	sprintf(size_array, "%ld", size);
//	strcat(daInviare,";");
//	strcat(daInviare,size_array);
//
//
//
	size_t a=strlen(daInviare);
//	inviaDati(fd_socket,&a,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,a);

	size_t dimBufferRicezione=0;
	int exit=0;
	char * pathEspulso=NULL;
	char * datiEspulsi=NULL;
	int entrato=0;
	size_t dimEspulsi=0;
	while(exit!=1)
	{
		a=0;
		dimBufferRicezione=0;
		int readReturnValue=riceviDati(fd_socket,&bufferRicezione,&dimBufferRicezione);

		if(readReturnValue>0 )
		{
			if(strcmp(bufferRicezione,"APPEND_TO_FILE: riscontrato errore")==0)
			{
				errno=EBADF;
				printf("errore append");
				return -1;
			}
			if(strncmp(bufferRicezione,"APPEND_TO_FILE eseguita correttamente!",41)==0)
			{
				exit=1;
				free(bufferRicezione);
//				printf("CLIENT -> esco\n");
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
					dimEspulsi=dimBufferRicezione;
					free(bufferRicezione);
				}
			}
		}
		else
		{
			if(abilitaStampe == 1 )
			{
				printf("Client -> Errore append\n");
			}
			return -1;

		}
	}

	FILE *file;
	if(errno==5 && datiEspulsi != NULL && pathEspulso != NULL)
	{
		int chdirReturnValue=0;
		chdirReturnValue=chdir(dirNameSalvato);
		if(chdirReturnValue != 0)
		{
			//è stato settato errno
			perror("Errore nell' utilizzo di chdir\n");
			return -1;
		}
		char* token;
		char * pathRelativo;
		char* rest = pathEspulso;

		while ((token = strtok_r(rest, "/", &rest))!=NULL)
		{
			pathRelativo=token;
		}

		file = fopen(pathRelativo,"w");
		if( file==NULL )
		{
			perror("Errore in apertura del file");
			return -1;
		}
		else
		{
			int w = fwrite(datiEspulsi, sizeof(char), dimEspulsi, file);
			if(w<0)
			{
				printf("CLIENT -> ERRORE fwrite\n");
			}
		}
		fclose(file);
		chdirReturnValue=chdir(directoryCorrente);
		if(chdirReturnValue != 0)
		{
			//è stato settato errno
			perror("Errore nell' utilizzo di chdir\n");
			return -1;
		}

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
	size_t op=102;

	writen(fd_socket,&op,sizeof(op));

//	inviaDati(fd_socket,&b,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strncpy(daInviare,pathname,strlen(pathname));




	size_t a=strlen(daInviare)+1;
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);

	readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
	if(readReturnValue > 0  && (abilitaStampe==1))
	{
		printf("CLIENT-> risposta server per file %s: %s\n",pathname,bufferRicezione);
	}
	else if (abilitaStampe==1)
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

	size_t op=103;
	writen(fd_socket,&op,sizeof(op));

//	size_t b=strlen(daInviare);
//	inviaDati(fd_socket,&b,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strncpy(daInviare,pathname,strlen(pathname));

	size_t a=strlen(daInviare)+1;
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);

	readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
	if(readReturnValue > 0 && abilitaStampe == 1)
	{
		printf("CLIENT-> risposta server per file %s: %s\n",pathname,bufferRicezione);
	}
	else if (abilitaStampe==1)
	{
		printf("CLIENT -> errore in operazione lock \n");
	}
	return 0;
}

int closeFile(const char* pathname)
{
	char daInviare[200]="CLOSE_FILE";
	char * bufferRicezione= NULL;
	statoFd=statoFileDescriptor();
	if(statoFd < 0)
	{
		return -1;
	}

//	size_t b=strlen(daInviare)+1;
//	printf("CLIENT -> B:%ld\n",b);
	size_t op=101;
	writen(fd_socket,&op,sizeof(op));
//	inviaDati(fd_socket,&b,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strcpy(daInviare,pathname);

	size_t a=strlen(daInviare)+1;
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);

//	a=0;
	int	readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);

	if(readReturnValue > 0 )
	{
		if(strncmp(bufferRicezione,"CLOSE_FILE: riscontrato errore",50))
		{
//			errno=EBADF;
//			perror("File descriptor non valido\n");
			if(abilitaStampe==1)
			{
				printf("CLIENT-> risposta server: %s\n",bufferRicezione);
			}
			return -1;

		}
		else if (abilitaStampe == 1)
		{
			if(abilitaStampe==1)
			{
				printf("CLIENT-> risposta server: %s\n",bufferRicezione);
			}
		}
	}
//	else
//	{
//		//la read ha riscontrato qualche errore, verifico quale esso sia grazie ad errno
//		if ((readReturnValue == -1) && (errno == EAGAIN))
//		{
//			perror("CLIENT-> Errore: Il file descriptor fd fa riferimento a un file diverso da un socket ed è stato contrassegnato come non bloccante (O_NONBLOCK)\n");
//			return -1;
//		}
//
//		if ((readReturnValue == -1) && (errno == EBADF))
//		{
//			perror("CLIENT-> Errore: fd non è un file descriptor valido o non è aperto per la lettura.\n");
//			return -1;
//		}
//		if ((readReturnValue == -1) && (errno == EIO))
//		{
//			perror("CLIENT-> Errore input/output");
//			return -1;
//		}
//		if ((readReturnValue == -1) && ((errno == EAGAIN) || (errno ==EWOULDBLOCK)))
//		{
//			//perror("CLIENT-> Errore: scrittura in spazio di archiviazione non disponibile!\n");
//			return -1;
//		}
//		if ((readReturnValue == -1) && (errno == EFAULT))
//		{
//			perror("CLIENT-> Errore: bufferRicezione è al di fuori del tuo spazio di indirizzi accessibile.");
//			return -1;
//		}
//		if ((readReturnValue == -1) && (errno == EINTR))
//		{
//			perror("CLIENT-> Errore: la chiamata è stata interrotta da un segnale prima che venissero letti i dati");
//			return -1;
//		}
//		if ((readReturnValue == -1) && (errno == EINVAL))
//		{
//			perror("CLIENT-> Errore: fd è attaccato a un oggetto che non è adatto per la lettura;"
//					" oppure il file è stato aperto con il flag O_DIRECT, "
//					"e l'indirizzo specificato in buf, il valore "
//					"specificato in sizeof(bufferRicezione) o l'offset del file non è adeguatamente allineato.");
//			return -1;
//		}
//		if ((readReturnValue == -1) && (errno == EISDIR))
//		{
//			perror("CLIENT-> Errore: il file descriptor fd fa riferimento ad una directory");
//			return -1;
//		}
//	}
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
	size_t op=108;

	writen(fd_socket,&op,sizeof(op));

//	size_t b=strlen(daInviare);
//	inviaDati(fd_socket,&b,sizeof(size_t));
//	inviaDati(fd_socket,daInviare,b);

	pathname=relativoToAssoluto(pathname);
	strncpy(daInviare,pathname,strlen(pathname));

	size_t a=strlen(daInviare)+1;
	inviaDati(fd_socket,&a,sizeof(size_t));
	inviaDati(fd_socket,daInviare,a);

	readReturnValue=riceviDati(fd_socket,&bufferRicezione,&a);
	if(readReturnValue > 0 )
	{
		if(abilitaStampe == 1)
		{
			printf("CLIENT-> risposta server per file %s: %s\n",pathname,bufferRicezione);
		}
	}
	else
	{
		perror("CLIENT -> errore in operazione remove \n");
		return -1;
	}
	return 0;
}


int recuperaFile(const char *path, void **fileBuffer, size_t *size)
{
//	  AIN(path, "invalid path argument for readLocalFile", errno = EINVAL; return -1;)
//	  AIN(bufPtr, "invalid bufPtr argument for readLocalFile", errno = EINVAL; return -1;)
//	  AIN(size, "invalid size argument for readLocalFile", errno = EINVAL; return -1;)

	  FILE *fptr = NULL;
	  *fileBuffer = NULL;

	  // control flow flags
	  int error = 0;
	  int closeFile = 0;
	  int freeBuf = 0;

	  // open the file
	  fptr = fopen(path, "r");
//	  AIN(fptr, "cannot open the file in readLocalFile", error = 1;)

	  // go to the end of the file
//	  if (!error)
	  {
	    fseek(fptr, 0L, SEEK_END);
	  }

	  // read its size
//	  if (!error)
	  {
	    *size = ftell(fptr);
//	    AINO(*size, "readLocalFile internal error: ftell", error = 1; closeFile = 1;)
	  }

	  // rewind the file pointer
//	  if (!error)
	  {
	    errno = 0;
	    rewind(fptr);
	    if (errno)
	    {
	      error = 1;
	      closeFile = 1;
	      perror("readLocalFile internal error: rewind");
	    }
	  }

	  // alloc enough space
	  if (!error)
	  {
	    *fileBuffer = malloc(sizeof(char) * (*size));
//	    AIN(*bufPtr, "readLocalFile internal error: malloc", error = 1; closeFile = 1;)
	  }

	  // read the file into the buffer
//	  if (!error)
	  {
	    int readSize = fread(*fileBuffer, sizeof(char), *size, fptr);
	    if (readSize < *size)
	    {
	      perror("readLocalFile internal error: fread");
	      error = 1;
	      freeBuf = 1;
	    }

	    closeFile = 1;
	  }

	  if (closeFile)
	  {
	    errno = 0;
	    fclose(fptr);
	    if (errno)
	    {
	      perror("readLocalFile internal error: fclose");
	      error = 1;
	      freeBuf = 1;
	    }
	  }

	  if (freeBuf)
	  {
	    free(*fileBuffer);
	    *fileBuffer = NULL;
	  }

//	  if (error)
//	  {
//	    return -1;
//	  }
//	  else
	  {
//	    P(printf("successfully read the file %s from disk of size %zd\n", path, *size));
	    return 0;
	  }
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
		 if(abilitaStampe==1)
		 {
			 perror("ERRORE è stato passato alla funzione un valore non valido");
		 }
		 return -1;
	 }


	 int leggiTuttiIFile = *numFile2 <= 0;
	 DIR *dir = opendir(dirName);
	 if(dir==NULL)
	 {
		 if(abilitaStampe==1)
		 {
			 perror("ERRORE nella funzione opendir\n");
		 }
		 return -1;
	 }

	 // Eseguo operazione cd nella directory selezionata
	 int chDirReturnValue=0;
	 chDirReturnValue=chdir(dirName);
	 if(chDirReturnValue==-1 && errno!=0)
	 {
		 if(abilitaStampe == 1)
		 {
			 perror("ERRORE nella funzione chdir\n");
		 }
		 return -1;
	 }


	 struct dirent *file = NULL;
	 //Leggo ogni entry presente nella directory fino a che non ho letto tutti i file oppure ho raggiunto il limite
	 while ((leggiTuttiIFile || *numFile2) && (file = readdir(dir)) != NULL)
	 {
		 char *filename = file->d_name;
		 struct stat s;
		 stat(filename, &s);



		 int isFileCurrentDir = isCurrentDirOrParentDir(filename);

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
					 if(abilitaStampe==1)
					 {
						 perror("ERRORE nella funzione closedir\n");
						 return -1;
					 }
				 }
			 }
			 else
			 {
				 //Ritorno nella directory che sto elaborando

				 int chdirReturnValue=0;

				 chdirReturnValue=chdir("..");
				 if(chdirReturnValue==-1 && errno!=0)
				 {
					 if(abilitaStampe==1)
					 {
						 perror("ERRORE nella funzione closedir\n");
					 }
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
				 char *path =NULL;
				 path=malloc(sizeof(char)*lunghezza);
				 path=relativoToAssoluto(filename);
				 if(strcmp(arrayPath[posizioneArray],"")!=0)
				 {
					 posizioneArray++;
				 }
				 strcpy(arrayPath[posizioneArray],path);
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

