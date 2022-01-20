#!/bin/bash

 ./server config3.txt &
SERVER_PID=$!



sleep 3

CLIENTS_PID=$!

ID=0

CLS=()
for i in {1..10}; do
    ID=${i}
    export ID
    ./creazioneClient.sh &
    CLS+=($!)
done

sleep 10

kill -SIGINT $SERVER_PID
#wait $CLIENTS_PID

for i in "${CLS[@]}"; do
    kill -9 ${i}
    wait ${i}
done
./statistiche.sh log.txt
exit 0

