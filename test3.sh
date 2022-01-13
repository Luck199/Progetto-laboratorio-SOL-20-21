#!/bin/bash

#./client "-p -f sockfile -t 0  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file2.txt,file1.txt" &


./server config3.txt &
PID_SERVER=$!

#wait $PID_SERVER

CLIENTS_PID=$!

ID=0

CLS=()
for i in {1..10}; do
    ID=${i}
    export ID
    ./client.sh &
    CLS+=($!)
done

sleep 5

kill -SIGQUIT $SERVER_PID
wait $CLIENTS_PID

for i in "${CLS[@]}"; do
    kill -9 ${i}
    wait ${i}
done

./statistiche.sh
