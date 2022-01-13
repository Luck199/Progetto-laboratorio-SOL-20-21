#!/bin/bash
./client "-p -f sockfile -t 200  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file1.txt,file2.txt,file3.txt" &
./client "-p -f sockfile -t 200  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file4.txt,file5.txt,file6.txt,file7.txt" &
./client "-p -f sockfile -t 200  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file8.txt,file9.txt,file10.txt,file11.txt" &
./server config2.txt &
PID_SERVER=$!
sleep 3
kill -SIGHUP $PID_SERVER

#wait $PID_SERVER
