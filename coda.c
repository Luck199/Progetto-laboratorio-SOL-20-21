#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include "coda.h"

//Gestione di una coda di stringhe

int is_empty_list(struct struttura_coda* head)
{
	return head->opzione == NULL;
}

int is_valid_list(struct struttura_coda* head)
{
	return head != NULL;
}

int enqueue(struct struttura_coda* head, char * opzione)
{

	if (!is_valid_list(head))
	{
		//printf("Errore, testa della lista uguale a null");
		return 0;
	}

	if(is_empty_list(head))
    {
		head->opzione=(char*)malloc(80*sizeof(char));
		strcpy(head->opzione,opzione);
		head->prec = NULL;
		head->next = NULL;
		return 1;
    }
    else
    {
    	struct struttura_coda *nodoCorrente=head;
        while(nodoCorrente->next != NULL)
        {
			nodoCorrente=nodoCorrente->next;
		}
        struct struttura_coda* nodo_successivo = malloc(sizeof(struct struttura_coda));
        nodo_successivo->opzione=malloc(80*sizeof(char));
        //nodo_successivo->opzione = opzione;
        strcpy(nodo_successivo->opzione,opzione);
        nodo_successivo->next = NULL;
		nodoCorrente->next=nodo_successivo;
		nodo_successivo->prec = nodoCorrente;


		return 1;
    }
}


char * dequeue(struct struttura_coda* head)
{
	if (!is_valid_list(head))
	{
		perror("Errore! head == NULL");
		exit(1);
	}
	if(is_empty_list(head))
	{
		////printf("LISTA VUOTA!");
		return NULL;
	}
	else
	{
		char *comando_scodato =(char*)malloc(80*sizeof(char));
		strcpy(comando_scodato,head->opzione);
		if (head->next == NULL)
		{
			/* se la lista � fatta da un solo elemento, allora la lista diventa una lista vuota */
			head->opzione=NULL;
		}
		else
		{
			/* se invece devo eliminare il primo elemento della lista, ma la lista � formata da pi� elementi.
			 * Allora il secondo elemento della lista diventa il primo
			 */
			struct struttura_coda *tmp = head->next;
			strcpy(head->opzione,tmp->opzione);
			head->prec = NULL;
			head->next = tmp->next;
			/* nota che il puntatore al primo elemento della lista non � cambiato. � cambiato solamente il suo contenuto
			 * (che adesso � il secondo cliente della lista). Quindi nel caso in cui la lista sia di 3 o pi� elementi devo anche aggiustare
			 * il puntatore prec del terzo elemento della lista, per farlo puntare a head (perch� prima puntava al secondo elemento della lista).
			 */
			if(head->next != NULL)
			{
				head->next->prec = head;
			}
			free(tmp);
		}
		////printf("ho eliminato il comando dalla lista, ritorno la stringa:%s\n",comando_scodato);
		//return comando_scodato;
		return comando_scodato;
	}
}

char * rimuovi(struct struttura_coda* head, char * opzione)
{
	struct struttura_coda* temp;
	struct struttura_coda* nodo_da_rimuovere = NULL;

	if (!is_valid_list(head))
	{
		perror("Errore! head == NULL");
		exit(1);
	}

    if(is_empty_list(head))
    {
		////printf("Tentativo di eliminazione testa da lista vuota\n");
    	return NULL;
	}
	else
	{
		temp=head;
		while(temp != NULL && temp->opzione != 0 && nodo_da_rimuovere == NULL)
        {
            if (temp != NULL && temp->opzione != 0)
            {
            	if ( (temp->opzione) == opzione )
            	{
            		nodo_da_rimuovere = temp;
            	}
            }
            temp=temp->next;
        }
        if(nodo_da_rimuovere != NULL) /* se ho trovato il nodo da rimuovere */
        {
        	if (nodo_da_rimuovere == head)
        	{ /* se il nodo da rimuovere � il primo della lista in pratica � come fare una dequeue */
        		////printf("rimuovo nodo in testa\n");
        		dequeue(head);
        		return opzione;
        		////printf("Il cliente con id: %d si è spostato di cassa\n", nodo_da_rimuovere->cliente->idCliente);
        	}
        	else
        	{
        		/* se il nodo da rimuovere � nel mezzo della lista, allora per rimuoverlo devo cortocircuitare il nodo precedente a quello successivo
        		 */
        		struct struttura_coda *nodo_prec = nodo_da_rimuovere->prec;
        		nodo_prec->next=nodo_da_rimuovere->next;
        		if (nodo_da_rimuovere->next != NULL)
        		{
        			nodo_da_rimuovere->next->prec = nodo_prec;
        		}
        		free(nodo_da_rimuovere);
        		return opzione;
        		////printf("Il cliente con id: %d si è spostato di cassa\n", nodo_da_rimuovere->cliente->idCliente);


        	}
        }
        else
        {
        	return NULL;
        }
	}

}


void StampaLista(struct struttura_coda* head)
{
	struct struttura_coda *temp = head;
	if (!is_valid_list(head))
	{
		// tirare errore se head = null
		//printf("Errore Stampa Lista");
		exit(1);
	}

    if(is_empty_list(head))
    {
		//printf("Lista vuota\n");
	}
	else
	{
		//printf("Lista:");
		while (temp != NULL)
		{
		  //printf(" %s", temp->opzione);
		  temp = temp->next;
		}
		//printf("\n");
	}
}

int size(struct struttura_coda* head)
{
	int size = 0;
	struct struttura_coda *temp = head;
	if (!is_valid_list(head))
	{
		// tirare errore se head = null
		//printf("Errore Size Coda\n");
		exit(1);
	}

    if(is_empty_list(head))
    {
    	size = 0;
	}
	else
	{
		while (temp != NULL)
		{
		  size+=1;
		  temp = temp->next;
		}
	}
    return size;
}


























//gestione di una coda di interi



int is_empty_list_Interi(struct codaInteri* head)
{
	return head->fileDescriptor == -1;
}

int is_valid_list_Interi(struct codaInteri* head)
{
	return head != NULL;
}

int enqueue_Interi(struct codaInteri* head, int fileDescriptor)
{
	if (!is_valid_list_Interi(head))
	{
		perror("Errore, testa della lista uguale a null");
		return -5;
	}

	if(is_empty_list_Interi(head))
    {
		head->fileDescriptor = fileDescriptor;
		head->prec = NULL;
		head->next = NULL;
		////printf("Aggiunto alla coda cliente %d\n", cliente->idCliente);
		return 1;
    }
    else
    {
    	struct codaInteri *nodoCorrente=head;
    	if(nodoCorrente->fileDescriptor==-1)
    	{
    		nodoCorrente->fileDescriptor=fileDescriptor;
    	}
    	else
    	{
    		while(nodoCorrente->next != NULL)
    		{
    			nodoCorrente=nodoCorrente->next;
    		}
    		////printf("MALLOC Nodo Coda\n");
    		struct codaInteri* nodo_successivo = malloc(sizeof(struct codaInteri));
    		nodo_successivo->fileDescriptor = fileDescriptor;
    		nodo_successivo->next = NULL;
    		nodoCorrente->next=nodo_successivo;
    		nodo_successivo->prec = nodoCorrente;

    	}
        		////printf("Aggiunto alla coda cliente %d\n", cliente->idCliente);
		return 1;
    }
}


int dequeue_Interi(struct codaInteri* head, int *errore)
{
	if (!is_valid_list_Interi(head))
	{
		perror("Errore! head == NULL");
		*errore=1;
		return -1;
	}
	if(is_empty_list_Interi(head))
	{
		return -1;
		*errore=1;
	}
	else
	{
		int cliente_scodato = head->fileDescriptor;
		if (head->next == NULL)
		{
			/* se la lista � fatta da un solo elemento, allora la lista diventa una lista vuota */
			head->fileDescriptor = -1;
		}
		else
		{
			/* se invece devo eliminare il primo elemento della lista, ma la lista � formata da pi� elementi.
			 * Allora il secondo elemento della lista diventa il primo
			 */
			struct codaInteri *tmp = head->next;
			head->fileDescriptor = tmp->fileDescriptor;
			head->prec = NULL;
			head->next = tmp->next;
			/* nota che il puntatore al primo elemento della lista non � cambiato. � cambiato solamente il suo contenuto
			 * (che adesso � il secondo cliente della lista). Quindi nel caso in cui la lista sia di 3 o pi� elementi devo anche aggiustare
			 * il puntatore prec del terzo elemento della lista, per farlo puntare a head (perch� prima puntava al secondo elemento della lista).
			 */
			if (head->next != NULL)
			{
				head->next->prec = head;
			}
			free(tmp);
		}
		////printf("Scodato cliente: %d\n", cliente_scodato->idCliente);
		return cliente_scodato;
	}
}

int rimuovi_Interi(struct codaInteri* head, int idCliente)
{
	struct codaInteri* temp;
	struct codaInteri* nodo_da_rimuovere = NULL;
int prova=0;
	if (!is_valid_list_Interi(head))
	{
		perror("Errore! head == NULL");
		exit(1);
	}

    if(is_empty_list_Interi(head))
    {
		////printf("Tentativo di eliminazione testa da lista vuota\n");
    	return -1;
	}
	else
	{
		temp=head;
		while(temp != NULL && temp->fileDescriptor != 0 && nodo_da_rimuovere == NULL)
        {
            if (temp != NULL && temp->fileDescriptor != 0)
            {
            	if ( (temp->fileDescriptor) == idCliente )
            	{
            		nodo_da_rimuovere = temp;
            	}
            }
            temp=temp->next;
        }
        if(nodo_da_rimuovere != NULL) /* se ho trovato il nodo da rimuovere */
        {
        	if (nodo_da_rimuovere == head)
        	{ /* se il nodo da rimuovere � il primo della lista in pratica � come fare una dequeue */
        		////printf("rimuovo nodo in testa\n");
        		dequeue_Interi(head,&prova);
        		return idCliente;
        		////printf("Il cliente con id: %d si è spostato di cassa\n", nodo_da_rimuovere->cliente->idCliente);
        	}
        	else
        	{
        		/* se il nodo da rimuovere � nel mezzo della lista, allora per rimuoverlo devo cortocircuitare il nodo precedente a quello successivo
        		 */
        		struct codaInteri *nodo_prec = nodo_da_rimuovere->prec;
        		nodo_prec->next=nodo_da_rimuovere->next;
        		if (nodo_da_rimuovere->next != NULL)
        		{
        			nodo_da_rimuovere->next->prec = nodo_prec;
        		}
        		free(nodo_da_rimuovere);
        		return idCliente;
        		////printf("Il cliente con id: %d si è spostato di cassa\n", nodo_da_rimuovere->cliente->idCliente);


        	}
        }
        else {
        	return -1;
        }
	}

}


void StampaLista_Interi(struct codaInteri* head)
{
	printf("\n\n");
	struct codaInteri *temp = head;
	if (!is_valid_list_Interi(head)){
		// tirare errore se head = null
		//printf("Errore Stampa Lista");
		exit(1);
	}

    if(is_empty_list_Interi(head))
    {
		printf("Lista vuota\n");
	}
	else
	{
		printf("Lista:");
		while (temp != NULL)
		{
		  printf(" %d", temp->fileDescriptor);
		  temp = temp->next;
		}
		printf("\n\n");
	}
}

int size_Interi(struct codaInteri* head)
{
	int size = 0;
	struct codaInteri *temp = head;
	if (!is_valid_list_Interi(head))
	{
		// tirare errore se head = null
		//printf("Errore Size Coda\n");
		exit(1);
	}

    if(is_empty_list_Interi(head))
    {
    	size = 0;
	}
	else
	{
		while (temp != NULL)
		{
		  size+=1;
		  temp = temp->next;
		}
	}
    return size;
}


