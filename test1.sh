#!/bin/bash



./client " -p -f sockfile -t 200 -W file2.txt -r file2.txt" &

 ./client " -p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -d /home/luca/workspace2/FileStorageServer/salvataggio/fileLetti -W file12.txt -u file12.txt -r file12.txt -c file12.txt -W file25.txt,file26.txt -R 2 -l file25.txt -u file25.txt" &

./client " -p -f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file2.txt,file3.txt " &

valgrind  --leak-check=full -s ./server config.txt &

PID_SERVER=$!
sleep 3
kill -SIGHUP $PID_SERVER

#wait $PID_SERVER
