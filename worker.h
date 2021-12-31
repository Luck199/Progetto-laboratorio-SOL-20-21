#ifndef WORKER_H_
#define WORKER_H_

void inizializza_worker();
void* vitaWorker(void*  idWorker);
void  riceviDati(int fdDaElaborare, void *dest, size_t size);

#endif
