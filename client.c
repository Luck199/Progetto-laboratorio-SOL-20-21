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
#include "utility.h"
#include "apiServer.h"
#include "client.h"
#include "coda.h"

#define SOCKNAME "./sockfile"
#define socketLUNG 100
#define N 100

void enqueueString(struct struttura_coda *coda,char * stringa);
int parser(struct struttura_coda *comandi);
struct sockaddr_un sa;
char * dirname;
char * nomesocket;


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
	//int op;
	char * stringa=(char*)malloc(80*sizeof(char));
	strcpy(stringa, argv[1]);
	printf("CLIENT-> Stringa dei comandi: %s\n",stringa);

	char* token = strtok(stringa, " ");

	while (token != NULL)
	{
		//printf("%s\n", token);
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

void enqueueString(struct struttura_coda *coda,char * stringa)
{
	enqueue(coda, stringa);
}


int parser(struct struttura_coda *comandi)
{
	int lettopiuuno=0;
	int abilitaStampe;
	int openConnectionReturnValue;
	int timeToWaitBetweenConnections=0;
	int numeroFileDaLeggere=0;
	char daInviare[150];
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
		if(strcmp(stringa,"-h")==0)
		{
			printf("Opzioni accettate da client:\n"
					"  -h,\tstampa tutte le Opzioni accettate da client:\n"
					"  -O,\tSets the dir for openFile eviction. If no dir is setted, the file/s will be lost"
					"  -a,\tAccepts a list of files. The first will be a local source from where remotely append on the others"
					"  -f,\tSets the nomesocket file path up to the specified nomesocketPath\n"
					"  -w,\tSends to the server n files from the specified dirname directory. If n=0, sends every file in dirname\n"
					"  -W,\tSends to the server the specified files\n"
					"  -D,\tIf capacity misses occur on the server, save the files it gets to us in the specified dirname directory\n"
					"  -r,\tLegge i file specificati dal server\n"
					"  -R,\tLegge n file dal server (se n fosse uguale a zero, legge ogni file\n"
					"  -d,\tSalva i file letti con l' opzione -r o -R nella cartella specificata\n"
					"  -t,\t Specifica il tempo in millisecondi che intercorre tra l' invio di due richieste successive al server\n"
					"  -l,\tEsegue il lock sui file specificati\n"
					"  -u,\tSblocca il file specificato\n"
					"  -s,\tChiude il file specificato\n"
					"  -o,\tApre il file specificato senza l' utilizzo di flags\n"
					"  -e,\tApre il file specificato utilizzando il flag O_CREATE\n"
					"  -n,\tApre il file specificato utilizzando il flag O_LOCK\n"
					"  -c,\t Rimuove i file specificati dal server, se presenti\n"
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
					printf("CLIENT-> Sintassi comando errata! Arresto in corso!");
				}
				continue;
			}
			strcpy(nomesocket,stringa);
			if(abilitaStampe==1)
			{
				printf("CLIENT-> Letto nome nomesocket: %s\n",nomesocket);
			}
			(void)unlink(nomesocket);
			openConnectionReturnValue=openConnection(nomesocket,100,abstime);
			if(openConnectionReturnValue==-1)
			{
					perror("CLIENT-> Errore nell' apertura della connessione\n");
					return -1;
			}

			strncpy(daInviare,"Sono un client, mi collego\n",150);
			openFile(daInviare,0);
			continue;
		}
		if(strcmp(stringa,"-w")==0)
		{
			stringa=dequeue(comandi);
			if(stringa[0]=='-')
			{
				if(abilitaStampe==1)
				{
					printf("CLIENT-> Sintassi comando errata! Arresto in corso!");
				}
				continue;
			}
			strcpy(dirname,stringa);
			if(abilitaStampe==1)
			{
				printf("CLIENT-> Letto dirname: %s\n",dirname);
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
				timeToWaitBetweenConnections = a;
				if(abilitaStampe==1)
				{
					printf("CLIENT-> numero file: %d\n",timeToWaitBetweenConnections);
				}
			}
			else
			{
				lettopiuuno=1;
				timeToWaitBetweenConnections = 0;
				if(abilitaStampe==1)
				{
					printf("CLIENT-> numero file: %d\n",timeToWaitBetweenConnections);
				}
			}
			continue;
		}
		if(strcmp(stringa,"-W")==0)
		{
			stringa=dequeue(comandi);
			char* token = strtok(stringa,",");
			while (token != NULL)
			{
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
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
			strcpy(dirname,stringa);
			if(abilitaStampe==1)
			{
				printf("CLIENT-> Letto dirname: %s\n",dirname);
			}
			continue;
		}

		if(strcmp(stringa,"-r")==0)
		{

			stringa=dequeue(comandi);
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
				//printf("%s\n", token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			//StampaLista(files);
			if(abilitaStampe==1)
			{
				printf("CLIENT-> file correttamente aggiunti alla lista!\n");
			}
			continue;
		}
		if(strcmp(stringa,"-R")==0)
		{
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
					printf("CLIENT->tempo letto: %d\n",numeroFileDaLeggere);
				}
			}
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
			strcpy(nomesocket,stringa);
			if(abilitaStampe==1)
			{
				printf("CLIENT-> Letto dirname secondario: %s\n",nomesocket);
			}
			continue;
		}
		if(strcmp(stringa,"-t")==0)
		{
			stringa=dequeue(comandi);
			if(stringa[0]!='-')
			{
				int a=atoi(stringa);
				if(a<0)
				{
					//parametro errato
					perror("CLIENT-> Parametro opzione -t errato\n");
					return -1;
				}
				timeToWaitBetweenConnections = a;
				if(abilitaStampe==1)
				{
					printf("CLIENT->tempo letto: %d\n",timeToWaitBetweenConnections);
				}
			}
			else
			{
				lettopiuuno=1;
				timeToWaitBetweenConnections = 0;
				if(abilitaStampe==1)
				{
					printf("CLIENT->tempo letto: %d\n",timeToWaitBetweenConnections);
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
				//printf("%s\n", token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			if(abilitaStampe==1)
			{
				printf("CLIENT-> file correttamente aggiunti alla lista!\n");
			}
			continue;
		}
		if(strcmp(stringa,"-u")==0)
		{
			stringa=dequeue(comandi);
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
				//printf("%s\n", token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
			if(abilitaStampe==1)
			{
				printf("CLIENT-> file correttamente aggiunti alla lista!\n");
			}
			continue;
		}

		if(strcmp(stringa,"-c")==0)
		{
			stringa=dequeue(comandi);
			char* token = strtok(stringa, ",");
			while (token != NULL)
			{
				//printf("%s\n", token);
				enqueueString(files,token);
				token = strtok(NULL, ",");
			}
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







