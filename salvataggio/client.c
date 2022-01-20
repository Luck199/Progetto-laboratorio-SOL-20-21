#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include "utility.h"
#include "apiServer.h"
#include "client.h"
#include "coda.h"







#include <sys/syscall.h>
#include <stddef.h>

#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>


#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/un.h>        /* ind AF_UNIX */
#include <errno.h>
#include <assert.h>
#include <limits.h>






#define SOCKNAME "./sockfile"
#define socketLUNG 100
#define N 100
#define O_CREATE 0
#define O_LOCK 1
#define CREATELOCK 2



void enqueueString(struct struttura_coda *coda,char * stringa);



int parser(struct struttura_coda *comandi);
void ritardo();
struct sockaddr_un sa;
char * dirNAme;
char * nomesocket;
int numFile=0;
int numFileCheLeggo=0;
int ritardoFraRichieste=0;

int main(int argc, char **argv)
{
	nomesocket=(char*)malloc(socketLUNG*sizeof(char));

	dirNAme=(char*)malloc(80*sizeof(char));
	struct struttura_coda *comandi = malloc(sizeof(struct struttura_coda));
	struct struttura_coda *files = malloc(sizeof(struct struttura_coda));
	comandi->opzione=NULL;
	comandi->next = NULL;
	comandi->prec = NULL;

	files->opzione=NULL;
	files->next = NULL;
	files->prec = NULL;
	char * stringa=(char*)malloc(1000*sizeof(char));
	strcpy(stringa, argv[1]);
	////printf("CLIENT-> Stringa dei comandi: %s\n",stringa);

	char* token = strtok(stringa, " ");

	while (token != NULL)
	{
		enqueueString(comandi,token);
	    token = strtok(NULL, " ");
	}

	int b=parser(comandi);
	if(b==-1)
	{
		perror("Errore!");
	}


	closeConnection(nomesocket);
	//free(nomesocket);
	return 0;
}
void ritardo()
{
	//se con il comando -t è stato specificato un tempo diverso da zero,
	//allora tra l' invio di una richiesta al server e l' invio
	//della successiva verranno attesi tali millisecondi
	if (ritardoFraRichieste != 0)
	{
		int ritardoInMicrosecondi=0;
		ritardoInMicrosecondi=ritardoFraRichieste*1000;
//		struct timespec ritardo;
//		ritardo.tv_sec = 0;
//		ritardo.tv_nsec = ritardoInNanosecondi;
//		nanosleep(&ritardo, NULL);
		usleep(ritardoInMicrosecondi);
	}
	else
	{
		if(abilitaStampe==1)
		{
//			printf("nessun ritardo!\n");
		}

	}
}


void enqueueString(struct struttura_coda *coda,char * stringa)
{
	enqueue(coda, stringa);
}


int parser(struct struttura_coda *comandi)
{

	int lettopiuuno=0;

	int tempo=0;
	short opzioneD=0;//flag utilizzati per sapere se l' opzione -D è stata utilizzata
	short opzioned=0;//flag utilizzati per sapere se l' opzione -d è stata utilizzata
	int openConnectionReturnValue;
	int numeroFileDaLeggere=0;
//	char daInviare[150];
	char * dirnameSecondarioLetture=NULL;
	char * dirnameSecondarioScritture=NULL;

	char * stringa=(char*)malloc(80*sizeof(char));
	struct struttura_coda *files = malloc(sizeof(struct struttura_coda));

	do
	{
		//elimino il primo comando dalla lista per poterlo eseguire
		if(lettopiuuno==0)
		{
			stringa=dequeue(comandi);
		}
		else
		{
			lettopiuuno=0;
		}

		if(stringa==NULL)
		{
			if(abilitaStampe==1)
			{
				//ATTENZIONE: dequeue da lista vuota
			}
			continue;
		}
		if(strcmp(stringa,"-h") == 0)
		{
			printf("Opzioni accettate da client:\n"
					"  -h,\tstampa tutte le opzioni accettate da client:\n"
					"  -f,\tSetta il nome socket a quello specificato dal linea di comando ed esegue la openConnection su esso\n"
					"  -w,\tEsegue una richiesta di scrittura al server di n file (con n passato da linea di comando) presenti nella cartella dirname. Se n=0 li scrive tutti\n"
					"  -W,\tEsegue una richiesta di scrittura al server dei file specificati\n"
					"  -D,\tSe si verifica un caso di capacity misses a seguito dell' opzione -w e/o -W, i file esplusi vengono salvati in questa cartella\n"
					"  -r,\tEsegue una richiesta di lettura al server dei file specificati\n"
					"  -R,\tLegge n file dal server (se n fosse uguale a zero, legge ogni file)\n"
					"  -d,\tSalva i file letti con l' opzione -r o -R nella cartella specificata\n"
					"  -t,\t Specifica il tempo in millisecondi che deve passare tra l' invio di due richieste successive al server\n"
					"  -l,\tEsegue il lock sui file specificati, se presenti\n"
					"  -u,\tEsegue unlock sui file specificati, se presenti ed è stata eseguita precedentemente l' operazione di lock su essi\n"
					"  -c,\tRimuove i file specificati dal server, se presenti\n"
					"  -p,\tStampa tutte le informazioni su ogni operazione eseguita\n"
				  );
			stringa=dequeue(comandi);
			if(stringa != NULL)
			{
				if(abilitaStampe==1)
				{
					printf("CLIENT->Errore! a seguito del comando -h non devono essere presenti altre opzioni\n ");
				}
				//op=0;
			}
			continue;
		}
		if(strcmp(stringa,"-f")==0)
		{
			struct timespec abstime;
			time_t instanteAttuale = time(NULL);//Gestire errore
			abstime.tv_sec = instanteAttuale + 20;

			stringa=dequeue(comandi);
			if(stringa[0]=='-')
			{
				if(abilitaStampe==1)
				{
					//printf("CLIENT-> Sintassi comando errata! Arresto in corso!");
				}
				continue;
			}
			strcpy(nomesocket,stringa);
			if(abilitaStampe==1)
			{
				////printf("CLIENT-> Letto nome nomesocket: %s\n",nomesocket);
			}
			ritardo();

			openConnectionReturnValue=openConnection(nomesocket,100,abstime);
			if(openConnectionReturnValue == -1)
			{
					perror("CLIENT-> Errore nell' apertura della connessione\n");
					return -1;
			}

			ritardo();

			continue;
		}
		if(strcmp(stringa,"-w")==0)
		{
			errno=0;
			int numFile2=0;
			short presenteErrore=0;

			stringa=dequeue(comandi);
			char* dirName=malloc(200*sizeof(char));
			if(stringa[0]=='-')
			{
				if(abilitaStampe==1)
				{
					printf("CLIENT-> Sintassi comando errata! Arresto in corso!\n");
					return -1;
				}
				continue;
			}

//			if((strncmp(dirnameSecondarioScritture,"",1)==0) && opzioneD==0)
//			{
//				if(abilitaStampe==1)
//				{
//					printf("CLIENT-> Si desidera utilizzare l' opzione -w senza l' utilizzo dell opzione -D. Arresto in corso!\n");
//					return -1;
//				}
//				continue;
//			}
			int lunghezzaStringa= strlen(stringa)+1;
			strncpy(dirName,stringa,lunghezzaStringa);
			printf("dirName:%s\n",dirName);
//			stringa=dequeue(comandi);
			char* token = strtok(dirName, ",");
			printf("token:%s\n",token);
//			strncpy(dirName,token,strlen(token)+1);
			if(abilitaStampe==1)
			{
				printf("CLIENT-> Letto dirname: %s\n",dirName);
			}
			while (token != NULL || presenteErrore==0)
			{
//				printf("token:%s\n",token);

//				enqueueString(comandi,token);
				token = strtok(NULL, ",");
				if(token == NULL)
				{
					numFile2=0;
					break;
				}

				printf("token:%s\n\n",token);
				char* token2 = strtok(token,"=");
				if(strncmp(token2,"n",2)==0)
				{
					token2 = strtok(NULL,"=");
					if(token2 == NULL)
					{
						numFile2=0;
						break;
					}
//					printf("token2= %s \n",token2);
					numFile2=atoi(token2);
					printf("CLIENT -> numero:%d\n",numFile2);
					if(errno!=0)
					{
						printf("errno:%d\n",errno);
						numFile2=0;
						break;
					}
					break;
//					token = strtok(NULL, ",");

				}
				else
				{
					if(abilitaStampe==1)
					{
						printf("CLIENT -> Sintassi comando sbagliata\n");
					}
					presenteErrore=1;
					break;
				}

//
//				token = strtok(NULL, ",");

			}
//			printf("presenteErrore=%d\n",presenteErrore);
			if(presenteErrore==1)
			{
				printf("Esco\n");
				continue;
			}
//			if(stringa != NULL && stringa[0]!='-' )
//			{
//				int a=atoi(stringa);
//				if(a<0)
//				{
//					//parametro errato
//					perror("CLIENT-> Parametro opzione -w errato\n");
//					return -1;
//				}
//				numFile2 = a;
//				if(abilitaStampe==1)
//				{
//					printf("CLIENT-> numero file: %d\n",numFile);
//				}
//			}
//			else
//			{
//				lettopiuuno=1;
//				numFile2 = 0;
//				if(abilitaStampe==1)
//				{
//					printf("CLIENT-> numero file: %d\n",numFile2);
//				}
//			}

			char *directoryPartenza=NULL;
			size_t sizeDirectoryCorrente=150;
			directoryPartenza=malloc(sizeof(char)*sizeDirectoryCorrente);
			directoryPartenza=getcwd(directoryPartenza,sizeDirectoryCorrente);
			printf("CLIENT: lavoro nella directory:%s\n",directoryPartenza);

			int i=0;
			char **arrayPath;
			//questa variabile se a zero indica che non è stato specificato il numero di file che
			//è necessario leggere dalla directory, quindi viene eseguita una lettura iniziale di tutti
			//i file per sapere quanti essi siano, in modo da poter allocare una struttura dati adeguata
			//Se tale bit fosse ad 1, indica che il numero di file da leggere è conosciuto, quinid la lettura deve essere
			short bitConteggio=1;

			int numFileLetti=0;
			//Conto quanti file sono effettivamente presenti all' interno della direcotry richiesta
			bitConteggio=0;

			int letturaDirectoryReturnValue=leggiNFileDaDirectory(&numFile2,dirName,arrayPath,i,bitConteggio,&numFileLetti);

			printf("NElla directory %s ci sono %d file \n",dirName,numFileLetti);

//			if(letturaDirectoryReturnValue != 0)
//			{
//				perror("Errore nella lettura dei file dalla directory\n");
//			}

			printf("numFileLetti:%d\n",numFileLetti);
			if(numFile2<=0 || numFile2>numFileLetti)
			{
				numFile2=numFileLetti;

			}
			bitConteggio=1;

//			printf("Sono nell' opzione -w: il dirname è:%s e n:%d",dirName,numFile2);

			arrayPath = malloc(numFile2 * sizeof(char *));
			for(i=0; i<numFile2; i++)
			{
				arrayPath[i] = malloc(200 * sizeof(char));
				strncpy(arrayPath[i],"",2);
			}

			i=0;
//
			int salvaNumFile=numFile2;


			//Dopo esserm memorizzato la directory corrente da cui partivo, utilizzo la funzione chdir per ritornarci.
			//Questo risulta essere necessario perchè al procedura leggiNFileDaDirectory mi ha modificato al directory in cui sto lavorando.
			int chdirReturnValue=0;
			chdirReturnValue=chdir(directoryPartenza);
			if(chdirReturnValue != 0)
			{
				//è stato settato errno
				perror("Errore nell' utilizzo di chdir\n");
				return -1;
			}


//			printf("dirname prima della seconda %s\n",dirName);
			letturaDirectoryReturnValue=leggiNFileDaDirectory(&numFile2,dirName,arrayPath,i,bitConteggio,&numFileLetti);

//
			chdirReturnValue=0;
			chdirReturnValue=chdir(directoryPartenza);
			if(chdirReturnValue != 0)
			{
				//è stato settato errno
				perror("Errore nell' utilizzo di chdir\n");
				return -1;
			}

			if(letturaDirectoryReturnValue != 0)
			{
				perror("Errore nella lettura dei file dalla directory\n");
			}
			for(i = 0; i < salvaNumFile; i++)
			{
				char* token;
				char * pathRelativo;
				char* rest = arrayPath[i];

				while ((token = strtok_r(rest, "/", &rest))!=NULL)
				{
					pathRelativo=token;
				}
//				printf("Lavoro sul file: %s\n",arrayPath[i]);
				openFile(pathRelativo,CREATELOCK);
				writeFile(pathRelativo,dirnameSecondarioScritture);
				closeFile(pathRelativo);
			}


			ritardo();
			continue;
		}
		if(strcmp(stringa,"-W")==0)
		{
			stringa=dequeue(comandi);
			//la stringa token conterrà mano mano tutti i singoli file ricevuti a linea di comando che dovranno essere inviati al server
			char* token = strtok(stringa,",");
			while (token != NULL)
			{
				openFile(token,CREATELOCK);
//				ritardo();

				void *buf = NULL;
				size_t *size=0;
				writeFile(token,dirnameSecondarioScritture);

//				void * buffer="questi sono byte aggiunti";
//				printf("strlen:%ld\nsizeof:%ld\n\n\n\n\n",strlen(buffer),sizeof(buffer));
//				appendToFile(token,buffer,sizeof(buffer),"");
//				int result=readFile(token,&buf,size);
//				ritardo();
				closeFile(token);
//				ritardo();
//				lockFile(token);
//				removeFile(token);
//				enqueueString(files,token);
				token = strtok(NULL, ",");
//				lockFile(token);

			}

//			readNFiles(1,"/home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi");
//			removeFile(token);
//			ritardo();
			continue;
		}
		if(strcmp(stringa,"-a")==0)
		{
			//l' opzione -a apre il file specificato con il tag O_CREATE, esegue l' operazione di lock, ed in seguito esegue l' operazione di append
			//al file appena creato inserendoci ciò che viene specificato da linea di comando
			stringa=dequeue(comandi);
			//la stringa token conterrà mano mano tutti i singoli file ricevuti a linea di comando che dovranno essere inviati al server
			char* fileACuiAppendereBytes = strtok(stringa,",");
			openFile(fileACuiAppendereBytes,CREATELOCK);
			writeFile(fileACuiAppendereBytes,dirnameSecondarioScritture);
			char* token = strtok(stringa,",");
			while (token != NULL)
			{
//				openFile(fileACuiAppendereBytes,O_CREATE);
//				lockFile(fileACuiAppendereBytes);
				printf("token:%s\n",token);
				void *buf = NULL;
				size_t *size=0;
				void * buffer=NULL;
				int returnValue=0;
//				writeFile(fileACuiAppendereBytes,dirnameSecondarioScritture);
				returnValue=recuperaFile(token, &buffer, &size);
//				printf("ReturnVAlue %d\n",returnValue);
				printf("buffer:%s\n",buffer);
				appendToFile(fileACuiAppendereBytes,buffer,size,dirnameSecondarioScritture);
		//		int result=readFile(token,&buf,size);

		//		ritardo();
		//				lockFile(token);
		//				removeFile(token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			closeFile(fileACuiAppendereBytes);

			ritardo();
			continue;
		}

		if(strcmp(stringa,"-D")==0)
		{
			stringa=dequeue(comandi);
			if(stringa[0]=='-')
			{
				if(abilitaStampe==1)
				{
					perror("CLIENT-> Sintassi comando errata! Arresto in corso!");
				}
				continue;
			}
			size_t lunghezzaDirnameScrittura=strlen(stringa)+1;
			dirnameSecondarioScritture=malloc(sizeof(char)*lunghezzaDirnameScrittura);
			strncpy(dirnameSecondarioScritture,stringa,lunghezzaDirnameScrittura);
			opzioneD=1;
			if(abilitaStampe==1)
			{
				printf("CLIENT-> Letto dirname scritture: %s\n",dirnameSecondarioScritture);
			}
			continue;
		}

		if(strcmp(stringa,"-r")==0)
		{
//			readNFiles(-5,"salvataggio/fileEspulsi");


			if((strncmp(dirnameSecondarioLetture,"",1)==0) && opzioned==0)
			{
				if(abilitaStampe==1)
				{
					printf("CLIENT-> Si desidera utilizzare l' opzione -w senza l' utilizzo dell opzione -D. Arresto in corso!\n");
					return -1;
				}
				continue;
			}
			stringa=dequeue(comandi);
			void *buf = NULL;
//			size_t size;
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
				//printf("%s\n", token);
				lockFile(token);
				int result=readFile(token,&buf,&size);
				unlockFile(token);
				if(result==0 && abilitaStampe==1)
				{
//					printf("CLIENT -> ricevuto buffer contenente: %s, di grandezza: %ld\n",(char*)buf,size);
					printf("CLIENT -> readFile eseguita correttamente\n");
				}
				else if(abilitaStampe==1)
				{
					printf("CLIENT-> la readFile ha riscontrato un errore\n");
				}
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
//
////			closeFile("debiti.txt");
//			ritardo();
//			if(abilitaStampe==1)
//			{
//				////printf("CLIENT-> file correttamente aggiunti alla lista!\n");
//			}
			continue;
		}

		if(strcmp(stringa,"-R")==0)
		{
//			if((strncmp(dirnameSecondarioLetture,"",1)==0) && opzioned==0)
//			{
//				if(abilitaStampe==1)
//				{
//					printf("CLIENT-> Si desidera utilizzare l' opzione -R senza l' utilizzo dell opzione -d. Arresto in corso!\n");
//					return -1;
//				}
//				continue;
//			}

			stringa=dequeue(comandi);
			lettopiuuno=1;
			errno=0;
			if(stringa[0]!='-')
			{
				char* token = strtok(stringa,"=");
				if(strncmp(token,"n",2)==0)
				{
					token = strtok(NULL,"=");
					printf("token= %s \n",token);
					numeroFileDaLeggere=atoi(token);
					printf("CLIENT -> numero:%d\n",numeroFileDaLeggere);
					if(errno!=0)
					{
						printf("errno:%d\n",errno);
						numeroFileDaLeggere=0;
					}

				}
				if(abilitaStampe==1)
				{
					printf("CLIENT->numero di file letto: %d\n",numeroFileDaLeggere);
				}
			}
			else
			{
				numeroFileDaLeggere = 0;
				if(abilitaStampe==1)
				{
					printf("CLIENT->numeroFileLEtto: %d\n",numeroFileDaLeggere);
				}
			}
			ritardo();
			readNFiles(numeroFileDaLeggere, dirnameSecondarioLetture);
			continue;
		}
		if(strcmp(stringa,"-d")==0)
		{
			stringa=dequeue(comandi);
			if(stringa[0]=='-')
			{
				if(abilitaStampe==1)
				{
					perror("CLIENT-> Sintassi comando -d errata! Arresto in corso!\n");
				}
				return -1;
			}
			size_t lunghezzaDirnameLettura=strlen(stringa)+1;
			dirnameSecondarioLetture=malloc(sizeof(char)*lunghezzaDirnameLettura);
			strncpy(dirnameSecondarioLetture,stringa,lunghezzaDirnameLettura);
			opzioned=1;
			if(abilitaStampe==1)
			{
				printf("CLIENT-> Letto dirname letture: %s\n",dirnameSecondarioLetture);
			}
			continue;
		}
		if(strcmp(stringa,"-t")==0)
		{
			stringa=dequeue(comandi);
			if(stringa[0]!='-')
			{
				tempo=atoi(stringa);
				if(tempo<0 && abilitaStampe==1)
				{
					//parametro errato
					perror("CLIENT-> Parametro opzione -t errato\n");
					return -1;
				}
				ritardoFraRichieste = tempo;
				if(abilitaStampe==1)
				{
					printf("CLIENT->tempo letto: %d\n",tempo);
				}
			}
			else
			{
				lettopiuuno=1;
				tempo = 0;
				ritardoFraRichieste=0;
				if(abilitaStampe==1)
				{
					printf("CLIENT->tempo letto: %d\n",tempo);
				}
			}
			continue;
		}
		if(strcmp(stringa,"-l")==0)
		{
			stringa=dequeue(comandi);
			char* token = strtok(stringa, ",");

			while (token != NULL)
			{
				ritardo();
				//printf("voglio lockare il file: %s\n", token);
				lockFile(token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			if(abilitaStampe==1)
			{
				printf("CLIENT-> file correttamente aggiunti alla lista!\n");
			}
			ritardo();
			continue;
		}
		if(strcmp(stringa,"-u")==0)
		{
			stringa=dequeue(comandi);
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
				ritardo();
				unlockFile(token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			if(abilitaStampe==1)
			{
				printf("CLIENT-> file correttamente aggiunti alla lista!\n");
			}
			ritardo();
			continue;
		}

		if(strcmp(stringa,"-c")==0)
		{
			stringa=dequeue(comandi);
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
				removeFile(token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			ritardo();


			continue;
		}

		if(strcmp(stringa,"-p")==0)
		{
			abilitaStampe=1;
			printf("CLIENT-> stampe abilitate!\n");
			continue;
		}

	}while(stringa != NULL);

	free(stringa);
	return 0;
}


