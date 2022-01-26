#!/bin/bash

 ./server config3.txt &
SERVER_PID=$!



sleep 2

CLIENTS_PID=$!

ID=0

CLS=()
for i in {1..10}; do
    ID=${i}
    export ID
    ./creazioneClient.sh &
   # sleep 1
    CLS+=($!)
done
echo ${CLS[@]}
sleep 15
echo "STOPPING SERVER"
kill -SIGINT $SERVER_PID
#wait $SERVER_PID

for i in "${CLS[@]}"; do
    kill -9 ${i}
    wait ${i}
done

echo "-------------------------------------------------"
./statistiche.sh log.txt 
echo "-------------------------------------------------"

exit 0

