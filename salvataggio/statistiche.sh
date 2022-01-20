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
numRimpiazzi=0
sommaWrite=0
read=0
write=0
maxConnessioni=0
sommaRead=0
stringaWrite="WRITE_FILE_di_byte: "
stringaRead="READ_FILE_di_byte: "
stringa="Richiesta Servita dal thread: "
stringaRimpiazzi="NumRimpiazzi: "
stringaMaxConnessioni="NumMaxConnessioniContemporanee: "
numThread=0
appoggioWrite=0
sommaWrite=0
maxMem=0
maxConnessioni=0
stringaMem="maxMemoriaRaggiunta: "
readmedia=0
sizemedia=0
numMaxFilePresenti=0
stringaNumFile="numMaxFilePresenti: "
writemedia=0
#operazioniEseguitePerThread[8]
while IFS= read -r line 
do
  	#echo ${line}
  	if [ ${line%% *} = "numMaxFilePresenti:" ]; then          #conto quante lockFile sono state eseguite
		numMaxFilePresenti=${line/${stringaNumFile}}
	fi
  	if [ ${line%% *} = "OPEN_FILE_LOCK:" ]; then          #conto quante lockFile sono state eseguite
		openlock=$((openlock+1))
	fi
	if [ ${line%% *} = "LOCK_FILE:" ]; then          #conto quante lockFile sono state eseguite
		lock=$((lock+1))
	fi	
	if [ ${line%% *} = "UNLOCK_FILE:" ]; then          #conto quante unLockFile sono state eseguite
		unlock=$((unlock+1))
	fi

	if [ ${line%% *} = "CLOSE_FILE:" ]; then          #conto quante close sono state eseguite
		close=$((close+1))
	fi
	if [ ${line%% *} = "Richiesta" ]; then          #conto quante richieste sono state eseguite dai worker
		richiesteServite=$((richiesteServite+1))
		numThread=${line/${stringa}}
		operazioniEseguitePerThread[numThread]=$((operazioniEseguitePerThread[numThread]+1))
	fi
	if [ ${line%% *} = "READ_FILE_di_byte:" ]; then          #conto quante read sono state eseguite
		read=$((read+1))
		appoggioRead=${line/${stringaRead}}
		sommaRead=$(($sommaRead + $appoggioRead))
	fi
	if [ ${line%% *} = "WRITE_FILE_di_byte:" ]; then          #conto quante write sono state eseguite
		write=$((write+1))
		appoggioWrite=${line/${stringaWrite}}
		sommaWrite=$(($sommaWrite + $appoggioWrite))

		
		#sommaWrite=${line/${stringaWrite}}
	fi
	
	if [ ${line%% *} = "NumFileMax:" ]; then          #leggo quanti rimpiazzi sono stati eseguiti
		numRimpiazzi=${line/${stringaRimpiazzi}}
	fi

	if [ ${line%% *} = "NumRimpiazzi:" ]; then          #leggo quanti rimpiazzi sono stati eseguiti
		numRimpiazzi=${line/${stringaRimpiazzi}}
	fi
	if [ ${line%% *} = "maxMemoriaRaggiunta:" ]; then          #leggo quanti rimpiazzi sono stati eseguiti
		maxMem=${line/${stringaMem}}
	fi

	if [ ${line%% *} = "NumMaxConnessioniContemporanee:" ]; then          #leggo il massimo numero di connessioni contemporanee
		maxConnessioni=${line/${stringaMaxConnessioni}}
	fi

done < $1
if [ "$read" -ne 0 ]; then 
readmedia=$(($sommaRead/$read))
fi

if [ "$write" -ne 0 ]; then 
writemedia=$(($sommaWrite/$write))
fi
	echo -e "numero di read eseguite:${read}"
	echo -e "size media delle letture: ${readmedia}"
	echo -e "numero write eseguite: ${write}"
	echo -e "size media write : ${writemedia}"
	echo -e "numero lock eseguite: ${lock}"
	echo -e "numero unlock eseguite: ${unlock}"
	echo -e "numero open con flag lock eseguite: ${openlock}"
	echo -e "numero close file eseguite: ${close}"
	echo -e "massima memoria raggiunta: ${maxMem}"

	echo -e "numero massimo di file memorizzati: ${numMaxFilePresenti}"
	echo -e "numero rimpiazzi eseguiti: ${numRimpiazzi}"
	for ((i=0; i<8; i++)); do
		echo "numero operazioni eseguite dal thread ${i}: ${operazioniEseguitePerThread[${i}]}"
	done
	echo -e "numero max connessioni contemporanee: ${maxConnessioni}"
#done

#exec 3<&- #chiudo il file del file descriptor 3
