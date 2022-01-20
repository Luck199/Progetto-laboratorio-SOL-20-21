#!/bin/bash

#./server config.txt &
valgrind  --leak-check=full ./server config.txt &
PID_SERVER=$!
#./client " -p -f sockfile -t 200 -W file1.txt" &
#./client " -p -f sockfile -t 200 -w salvataggio/fileLetti,cuccurù" &
#./client " -p -f sockfile -D salvataggio/fileEspulsi -t 0 -d salvataggio/fileLetti -W file1.txt,file2.txt,file3.txt  -R n=6 -l file1.txt,file2.txt,file3.txt -c file1.txt,file2.txt,file3.txt   " &
#./client " -p -f sockfile -D salvataggio/fileEspulsi -t 0 -w salvataggio/fileEspulsi,n=8" &
./client " -p -f sockfile -D salvataggio/fileEspulsi -a file1.txt,file2.txt "&
#./client " -p -f sockfile -D salvataggio/fileEspulsi -t 0 -W file74.txt,file81.txt,file78.txt" &
#./client " -p -f sockfile -D salvataggio/fileEspulsi -t 200 -W " &
#./client " -p -f sockfile -t 200 -W file3.txt" &

#./client " -p -f sockfile -t 200 -r fill" &
#./client " -p -f sockfile -t 200 -W file3.txt" &
#./client " -p -f sockfile -t 200 -W file2.txt" &

# ./client " -p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -d /home/luca/workspace2/FileStorageServer/salvataggio/fileLetti -W file12.txt -u file12.txt -r file12.txt -c file12.txt -W file25.txt,file26.txt -R 2 -l file25.txt -u file25.txt" &

#./client " -p -f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file2.txt,file3.txt " &




sleep 2


kill -SIGHUP $PID_SERVER
wait $PID_SERVER
#echo "\n\n\n\n"
echo "-------------------------------------------------"
./statistiche.sh log.txt 
echo "-------------------------------------------------"
#wait $PID_SERVER
