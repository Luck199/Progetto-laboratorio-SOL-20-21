#!/bin/bash

#./client "-p -f sockfile -t 100  -w ../FileStorageServer 0" &

#./client "-p -f sockfile -t 200 -D ../FileStorageServer/ciao -W file2.txt,file1.txt -R 2 " &

./client "-p -f sockfile -t 200  -D /home/luca/workspace2/FileStorageServer/salvataggio/ciao2 -W file2.txt,file1.txt" &
#./client "-p -f sockfile -t 100 -D /home/luca/workspace2/FileStorageServer/salvataggio/ciao2 -w /home/luca/workspace2/FileStorageServer/salvataggio 2" &
#./client "-p -f sockfile -t 200 -W test1.txt" &
#./client "-p -f sockfile  -W " &
#./client "-p -f sockfile  -W debiti.txt" &
#valgrind  --leak-check=full --show-leak-kinds=all -s --track-origins=yes ./server config.txt &
./server config.txt &
PID_SERVER=$!
sleep 3
kill -SIGQUIT $PID_SERVER

#wait $PID_SERVER
