#!/bin/bash

#./server config.txt &
valgrind  --leak-check=full --track-origins=yes -s ./server config.txt &
PID_SERVER=$!
sleep 2


./client " -p -f sockfile -t 200 -D salvataggio/fileEspulsi -OL -W file80.txt " &


./client " -p -f sockfile -t 200 -D salvataggio/fileEspulsi -OL -a file10.txt,file11.txt " &


./client "-p -f sockfile -t 200 -d salvataggio/fileLetti -h" &


./client "-p -f sockfile -t 200 -D salvataggio/fileEspulsi -OL -W file3.txt" &

./client " -p -f sockfile -t 200 -D salvataggio/fileEspulsi -OL -w salvataggio/fileDaLeggere,n=3" &
./client " -p -f sockfile -t 200 -D salvataggio/fileEspulsi -OL -W paesaggio.jpeg,paesaggio1.jpeg,paesaggio2.jpeg" &
./client " -p -f sockfile -t 200 -D salvataggio/fileEspulsi -OL -W paesaggio3.jpeg,paesaggio4.jpeg" &
./client " -p -f sockfile -t 200 -D salvataggio/fileEspulsi -OL -W breve5.mp3" &

./client "-p -f sockfile -t 200 -d salvataggio/fileLetti -L -r file3.txt" &

./client "-p -f sockfile -t 200 -l file3.txt -u file3.txt" &

./client "-p -f sockfile -t 200 -d salvataggio/fileLetti -R n=10" &


./client "-p -f sockfile -t 200 -l file80.txt,file10.txt -c file80.txt,file10.txt" &



./client "-p -f sockfile -t 200 -l paesaggio.jpeg,paesaggio1.jpeg -c paesaggio.jpeg,paesaggio1.jpeg" &


sleep 2


kill -SIGHUP $PID_SERVER
wait $PID_SERVER

echo "-------------------------------------------------"
./statistiche.sh log.txt 
echo "-------------------------------------------------"

