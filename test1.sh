#!/bin/bash

#./client "-p -f sockfile -t 100  -w ../FileStorageServer 0" &

#./client "-p -f sockfile -t 200 -D ../FileStorageServer/ciao -W file2.txt,file1.txt -R 2 " &

#./client "-p -f sockfile -t 200  -D /home/luca/workspace2/FileStorageServer/salvataggio/ciao2 -W file2.txt,file1.txt" &
#./client "-p -f sockfile -t 200  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file2.txt,file1.txt -d /home/luca/workspace2/FileStorageServer/salvataggio/fileLetti -R 2" &





./client "-p -f sockfile -t 200  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file2.txt,file1.txt  -w /home/luca/workspace2/FileStorageServer/salvataggio 2" &

#-w /home/luca/workspace2/FileStorageServer/salvataggio 2 -d /home/luca/workspace2/FileStorageServer/salvataggio/fileLetti -R 2 -r file1.txt


#./client " -p -f sockfile -t 200 -W file2.txt" &
#./client "-p -f sockfile -t 100 -D /home/luca/workspace2/FileStorageServer/salvataggio/ciao2 -w /home/luca/workspace2/FileStorageServer/salvataggio 2" &


#valgrind  --leak-check=full -s ./server config.txt &
./server config.txt &
PID_SERVER=$!
sleep 3
kill -SIGHUP $PID_SERVER

#wait $PID_SERVER
