#!/bin/bash


./server config3.txt &
SERVER_PID=$!



PID_CLIENTS=$!

ID=0
IDCLIENT=()
for i in {1..10}; do
    ID=${i}
    export ID
    ./creazioneClient.sh &
    IDCLIENT+=($!)
done


sleep 30

kill -SIGINT $SERVER_PID


wait $PID_CLIENTS

for i in "${IDCLIENT[@]}"; do
    kill -9 ${i}
    wait ${i}
done

./statistiche.sh log.txt
exit 0

