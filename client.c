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
//int isCurrentDirOrParentDir(char *nomeDirectory);
//int leggiNFileDaDirectory(int *numFile2,const char *dirName, char** arrayPath, int posizioneArray);

//int leggiNFileDaDirectory(int *numFile, char *dirname, char** arrayPath, int posizioneArray,int bitConteggio);


int parser(struct struttura_coda *comandi);
void ritardo();
struct sockaddr_un sa;
char * dirname;
char * nomesocket;
int numFile=0;
int numFileCheLeggo=0;
int ritardoFraRichieste=0;

int main(int argc, char **argv)
{
	nomesocket=(char*)malloc(socketLUNG*sizeof(char));

	dirname=(char*)malloc(80*sizeof(char));
	struct struttura_coda *comandi = malloc(sizeof(struct struttura_coda));
	struct struttura_coda *files = malloc(sizeof(struct struttura_coda));
	comandi->opzione=NULL;
	comandi->next = NULL;
	comandi->prec = NULL;

	files->opzione=NULL;
	files->next = NULL;
	files->prec = NULL;
	char * stringa=(char*)malloc(200*sizeof(char));
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
		//printf("Ritardo diverso da zero!\n");
		struct timespec ritardo;
		ritardo.tv_sec = ritardoFraRichieste / 1000;
		ritardo.tv_nsec = (ritardoFraRichieste % 1000) * 1000000L;
		nanosleep(&ritardo, NULL);
	}
	else
	{
		//printf("nessun ritardo!\n");
	}
}


void enqueueString(struct struttura_coda *coda,char * stringa)
{
	enqueue(coda, stringa);
}


int parser(struct struttura_coda *comandi)
{
	int lettopiuuno=0;
	int abilitaStampe;
	int tempo=0;
	short opzioneD=0;//flag utilizzati per sapere se l' opzione -D è stata utilizzata
	short opzioned=0;//flag utilizzati per sapere se l' opzione -d è stata utilizzata
	int openConnectionReturnValue;
	int numeroFileDaLeggere=0;
	char daInviare[150];
	char dirnameSecondarioLetture[200]="";
	char dirnameSecondarioScritture[200]="";

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
//			(void)unlink(nomesocket);
			ritardo();
			openConnectionReturnValue=openConnection(nomesocket,100,abstime);
			if(openConnectionReturnValue == -1)
			{
					perror("CLIENT-> Errore nell' apertura della connessione\n");
					return -1;
			}
			//strncpy(daInviare,"WRITE_FILE;",150);
			ritardo();

			//openFile("file1.txt",O_CREATE);
			//openFile("file2.txt",CREATELOCK);


//
//			appendToFile("file1.txt",buffer,sizeof(buffer),"cartella");
//			lockFile("file1.txt");
			//openFile("file2.txt",O_LOCK);
			continue;
		}
		if(strcmp(stringa,"-w")==0)
		{
			int numFile2=0;
			stringa=dequeue(comandi);
			char* dirName=malloc(200*sizeof(char));
			if(stringa[0]=='-')
			{
				if(abilitaStampe==1)
				{
					////printf("CLIENT-> Sintassi comando errata! Arresto in corso!\n");
					return -1;
				}
				continue;
			}
			if((strncmp(dirnameSecondarioScritture,"",1)==0) && opzioneD==0)
			{
				if(abilitaStampe==1)
				{
					printf("CLIENT-> Si desidera utilizzare l' opzione -w senza l' utilizzo dell opzione -D. Arresto in corso!\n");
					return -1;
				}
				continue;
			}
			int lunghezzaStringa= strlen(stringa)+1;
			strncpy(dirName,stringa,lunghezzaStringa);
			if(abilitaStampe==1)
			{
				////printf("CLIENT-> Letto dirname: %s\n",dirname);
			}
			stringa=dequeue(comandi);

			if(stringa != NULL && stringa[0]!='-' )
			{
				int a=atoi(stringa);
				if(a<0)
				{
					//parametro errato
					perror("CLIENT-> Parametro opzione -w errato\n");
					return -1;
				}
				numFile2 = a;
				if(abilitaStampe==1)
				{
					////printf("CLIENT-> numero file: %d\n",numFile);
				}
			}
			else
			{
				lettopiuuno=1;
				numFile2 = 0;
				if(abilitaStampe==1)
				{
					////printf("CLIENT-> numero file: %d\n",numFile);
				}
			}



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
			leggiNFileDaDirectory(&numFile,dirName,arrayPath,i,bitConteggio,&numFileLetti);

//				if(letturaDirectoryReturnValue != 0)
//				{
//					perror("Errore nella lettura dei file dalla directory\n");
//				}



			if(numFile2<0 || numFile2>numFileLetti)
			{
				numFile2=numFileLetti;

			}
			bitConteggio=1;


			arrayPath = malloc(numFile2 * sizeof(char *));
			for(i=0; i<numFile2; i++)
			{
				arrayPath[i] = malloc(200 * sizeof(char));
				strncpy(arrayPath[i],"",2);
			}

			i=0;

			int letturaDirectoryReturnValue=0;
//			printf("dirname:%s\n",dirName);
//			printf("richiedo -w nella cartella %s , di %d file, i vale %d\n",dirName,numFile2,i);
			int salvaNumFile=numFile2;
			letturaDirectoryReturnValue=leggiNFileDaDirectory(&numFile2,dirName,arrayPath,i,bitConteggio,&numFileLetti);
//			printf("il dirname secondario è il seguente:%s\n",dirnameSecondarioScritture);

			if(letturaDirectoryReturnValue != 0)
			{
				perror("Errore nella lettura dei file dalla directory\n");
			}
			for(i = 0; i < salvaNumFile; i++)
			{
//				printf("arrayPath[%d]:%s\n",i,arrayPath[i]);
				openFile(arrayPath[i],O_CREATE);
				//writeFile(arrayPath[i],dirnameSecondarioScritture);
				closeFile(arrayPath[i]);
			}

			//strncpy(daInviare,"WRITE_FILE;",150);
			ritardo();




			continue;
		}
		if(strcmp(stringa,"-W")==0)
		{
			if((strncmp(dirnameSecondarioScritture,"",1)==0) && opzioneD==0)
			{
				if(abilitaStampe==1)
				{
					printf("CLIENT-> Si desidera utilizzare l' opzione -w senza l' utilizzo dell opzione -D. Arresto in corso!\n");
					return -1;
				}
				continue;
			}
			stringa=dequeue(comandi);
			//la stringa token conterrà mano mano tutti i singoli file ricevuti a linea di comando che dovranno essere inviati al server
			char* token = strtok(stringa,",");
			while (token != NULL)
			{
				//openFile(token,O_CREATE);

//				void * buf="viva il carnevale";
//				size_t size=sizeof(buf);
				openFile(token,CREATELOCK);
				ritardo();
//				printf("gli passo questa cartella:%s\n",dirnameSecondarioScritture);
				writeFile(token,dirnameSecondarioScritture);
				//appendToFile(token,buf,size,"");
				closeFile(token);

				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			strncpy(daInviare,"WRITE_FILE;ti/puzza/il/culo.txt\n",150);
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
			strcpy(dirnameSecondarioScritture,stringa);
			opzioneD=1;
			if(abilitaStampe==1)
			{
				//printf("CLIENT-> Letto dirname: %s\n",dirname);
			}
			continue;
		}

		if(strcmp(stringa,"-r")==0)
		{
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
			size_t size;
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
				printf("%s\n", token);
				int result=readFile(token,&buf,&size);
				//printf("MENU CLIENT -> %s\n\n\n",*buf);

//				dirname=relativoToAssoluto(dirname);
//				printf("dirname assoluto: %s\n",dirname);
				if(result==0)
				{
					//printf("CLIENT -> ricevuto buffer contenente: %s, di grandezza: %ld\n",(char*)buf,size);
				}
				else
				{
					printf("CLIENT-> la readFile ha riscontrato un errore\n");
				}
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}

//			closeFile("debiti.txt");
			//StampaLista(files);
			ritardo();
			//readFile();
			if(abilitaStampe==1)
			{
				////printf("CLIENT-> file correttamente aggiunti alla lista!\n");
			}
			continue;
		}
		if(strcmp(stringa,"-R")==0)
		{
			if((strncmp(dirnameSecondarioScritture,"",1)==0) && opzioned==0)
			{
				if(abilitaStampe==1)
				{
					printf("CLIENT-> Si desidera utilizzare l' opzione -w senza l' utilizzo dell opzione -D. Arresto in corso!\n");
					return -1;
				}
				continue;
			}
			stringa=dequeue(comandi);
			lettopiuuno=1;
			if(stringa[0]!='-')
			{
				int a=atoi(stringa);
				if(a<0)
				{
					//parametro errato
					perror("CLIENT-> Parametro opzione -R errato\n");
					return -1;
				}
				numeroFileDaLeggere = a;
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
					////printf("CLIENT->tempo letto: %d\n",numeroFileDaLeggere);
				}
			}
			ritardo();
			readNFiles(numeroFileDaLeggere, "cartella");
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
			strcpy(dirnameSecondarioLetture,stringa);
			opzioned=1;
			if(abilitaStampe==1)
			{
				printf("CLIENT-> Letto dirname secondario: %s\n",dirnameSecondarioLetture);
			}
			continue;
		}
		if(strcmp(stringa,"-t")==0)
		{
			stringa=dequeue(comandi);
			if(stringa[0]!='-')
			{
				tempo=atoi(stringa);
				if(tempo<0)
				{
					//parametro errato
					perror("CLIENT-> Parametro opzione -t errato\n");
					return -1;
				}
				ritardoFraRichieste = tempo;
				if(abilitaStampe==1)
				{
					////printf("CLIENT->tempo letto: %d\n",timeToWaitBetweenConnections);
				}
			}
			else
			{
				lettopiuuno=1;
				tempo = 0;
				if(abilitaStampe==1)
				{
					////printf("CLIENT->tempo letto: %d\n",timeToWaitBetweenConnections);
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
				////printf("CLIENT-> file correttamente aggiunti alla lista!\n");
			}
			ritardo();
			//printf("CLIENT-> INVIO RICHIESTA LOCK\n");


//			lockFile(daInviare);
			continue;
		}
		if(strcmp(stringa,"-u")==0)
		{
			stringa=dequeue(comandi);
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
				//printf("%s\n", token);
				ritardo();
				unlockFile(token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			if(abilitaStampe==1)
			{
				////printf("CLIENT-> file correttamente aggiunti alla lista!\n");
			}
			ritardo();
			//printf("CLIENT-> INVIO RICHIESTA UNLOCK\n");
			//unlockFile(daInviare);
			continue;
		}

		if(strcmp(stringa,"-c")==0)
		{
			stringa=dequeue(comandi);
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
//				printf("%s\n", token);

				removeFile(token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			ritardo();
			//printf("CLIENT-> INVIO RICHIESTA REMOVE_FILE\n");
			strncpy(daInviare,"REMOVE_FILE;ti/puzza/il/culo.txt\n",150);


			//removeFile(daInviare);
			continue;
		}

		if(strcmp(stringa,"-p")==0)
		{
			abilitaStampe=1;
			////printf("CLIENT-> stampe abilitate!\n");
			continue;
		}

	}while(stringa != NULL);

	free(stringa);
	return 0;
}




////Funzione che legge N file da una directory
////nel caso in cui siano presenti sottoDirectory, le visita ricorsivamente fino al raggiungimento di NumFile
//int leggiNFileDaDirectory(int *numFile, char *dirname, char** arrayPath, int posizioneArray,int bitConteggio)
//{
//	if(numFile == NULL)
//	{
//		perror("ERRORE è stato passato alla funzione un valore non valido");
//		return -1;
//	}
//
//
//	int leggiTuttiIFile = *numFile <= 0;
//	printf("directory:%s\n",dirname);
//	//apro la directory
//	DIR *dir = opendir(dirname);
//	if(dir==NULL)
//	{
//		perror("ERRORE nella funzione openDir\n");
//	}
//
//	// Eseguo operazione cd nella directory selezionata
//	int chDirReturnValue=0;
//	chDirReturnValue=chdir(dirname);
//	if(chDirReturnValue==-1 && errno!=0)
//	{
//		perror("ERRORE nella funzione chdir\n");
//	}
//
//
//	struct dirent *file = NULL;
//	//Leggo ogni entry presente nella directory fino a che non ho letto tutti i file oppure ho raggiunto il limite
//	while ((leggiTuttiIFile || *numFile) && (file = readdir(dir)) != NULL)
//	{
//		char *filename = file->d_name;
//		struct stat s;
//		stat(filename, &s);
//
//		//  stat(filename, &s);
//
//
//		int isFileCurrentDir = isCurrentDirOrParentDir(filename);
//		//**************************
//		//SCRIVERE IN RELAZIONE CHE UTILIZZO STAT CHE NON é POSIX!!!
//		//******************************
//		int isDirectory = S_ISDIR(s.st_mode);
//		int isFileRegolare = S_ISREG(s.st_mode);
//
//		//Tramite questi tre if, se ho selezionato un file
//		//speciale lo salto, non considerandolo nel conteggio
//		if (isFileCurrentDir == 1 || isFileCurrentDir == 2)
//		{
//			continue;
//		}
//		else if (!isDirectory && !isFileRegolare)
//		{
//			//se entro dentro questo if significato che
//			//grazie all' utilizzo della struttura stat,
//			//sono riuscito ad identificare che il file considerato in questo momento non
//			//risulta un file regolare e nemmeno una directory
//			continue;
//		}
//
//
//		else if (isDirectory)
//		{
//			posizioneArray++;
//			int chiamataRicorsivaReturnValue = leggiNFileDaDirectory(numFile,filename, arrayPath,posizioneArray,bitConteggio);
//			if (chiamataRicorsivaReturnValue == -1)
//			{
//				int closeDirReturnValue=0;
//				closeDirReturnValue=closedir(dir);
//				if(closeDirReturnValue==-1 && errno!=0)
//				{
//					perror("ERRORE nella funzione closedir\n");
//					return -1;
//				}
//			}
//			else
//			{
//				// cd in the current dir again
//
//				int chdirReturnValue=0;
//
//				chdirReturnValue=chdir("..");
//				if(chdirReturnValue==-1 && errno!=0)
//				{
//					perror("ERRORE nella funzione closedir\n");
//					return -1;
//				}
//
//			}
//		}
//		else if (isFileRegolare)
//		{
//			numFileCheLeggo++;
//			if(bitConteggio != 0)
//			{
//				char *path=NULL ;//SIstemare il discorso della free
//				path=malloc(sizeof(char)*300);
//				path=relativoToAssoluto(filename);
////
////
//				printf("path:%s\n",path);
////				if(strcmp(arrayPath[posizioneArray],"")==0)
////				{
////					strcpy(arrayPath[posizioneArray],path);
////				}
////				else
////				{
////					posizioneArray++;
////					strcpy(arrayPath[posizioneArray],path);
////				}
//
//			}
//
//			//free(path);
//
//
//			if (!leggiTuttiIFile)
//			{
//				(*numFile)--;
//			}
//
//
//		}
//		closedir(dir);
//		return 0;
//
//	}
//}


