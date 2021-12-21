#ifndef _coda_
#define _coda_

extern struct struttura_coda
{
	char * opzione;
	struct struttura_coda * prec;
    struct struttura_coda * next;
} struttura_coda;


extern struct codaInteri
{
	int fileDescriptor;
	struct codaInteri * prec;
    struct codaInteri * next;
} codaInteri;

extern int is_empty_list(struct struttura_coda* head);
extern int enqueue(struct struttura_coda* head, char * opzione);
extern char * dequeue(struct struttura_coda* head);
extern char * rimuovi(struct struttura_coda* head, char * opzione);
extern void StampaLista(struct struttura_coda *head);
extern int size(struct struttura_coda *head);


extern int is_empty_list_Interi(struct codaInteri* head);
extern int is_valid_list_Interi(struct codaInteri* head);
extern int enqueue_Interi(struct codaInteri* head, int fileDescriptor);
extern int dequeue_Interi(struct codaInteri* head);
extern int rimuovi_Interi(struct codaInteri* head, int idCliente);
extern void StampaLista_Interi(struct codaInteri* head);
extern int size_Interi(struct codaInteri* head);





#endif
