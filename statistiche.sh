#!/bin/bash

#Nel caso in cui il numero di argomenti risultasse diverso da 1
#stampa un messaggio di errore
if [ $# -ne 1 ]; then           
  echo "ATTENZIONE: $(basename $0) deve avere un argomento" 1>&2 
  exit -1                 # esce dallo script
fi


if [ ! -f $1 ]; then            # se il file non esiste oppure risulta regolare
  echo "ATTENZIONE: non riesco a trovare il file log.txt" 1>&2    
  exit -1      # esce dallo script
fi

openlock=0
lock=0
unlock=0
close=0
richiesteServite=0
read=0
write=0
stringa="Richiesta Servita dal thread: "
numThread=0

operazioniEseguitePerThread[25]
while IFS= read -r line 
do
  	#echo ${line}
	if [ ${line%% *} = "LOCK_FILE:" ]; then          #conto quante lockFile sono state eseguite
		lock=$((lock+1))
	fi	
	if [ ${line%% *} = "UNLOCK_FILE:" ]; then          #conto quante unLockFile sono state eseguite
		unlock=$((unlock+1))
	fi

	if [ ${line%% *} = "CLOSE:" ]; then          #conto quante close sono state eseguite
		close=$((close+1))
	fi
	if [ ${line%% *} = "Richiesta" ]; then          #conto quante richieste sono state eseguite dai worker
		richiesteServite=$((richiesteServite+1))
		numThread=${line/${stringa}}
		operazioniEseguitePerThread[numThread]=$((operazioniEseguitePerThread[numThread]+1))
		
	fi
	if [ ${line%% *} = "READ" ]; then          #conto quante read sono state eseguite
		read=$((read+1))
	fi
	if [ ${line%% *} = "write:" ]; then          #conto quante write sono state eseguite
		lock=$((lock+1))
	fi
done < $1

echo -e "numero thread: ${numThread}"
	echo -e "numero lock eseguite: ${lock}"
	echo -e "numero unlock eseguite: ${unlock}"
	echo -e "numero close eseguite: ${close}"
	echo -e "numero read eseguite: ${read}"
	echo -e "numero write eseguite: ${write}"
	echo -e "numero richieste eseguite: ${richiesteServite}"
	
	for ((i=0; i<=25; i+=2)); do
	echo "numero operazioni eseguite dal thread ${i}: ${operazioniEseguitePerThread[${i}]}"
done

#exec 3<&- #chiudo il file del file descriptor 3
