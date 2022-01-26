#!/bin/bash

valgrind  --leak-check=full --show-leak-kinds=all ./server config2.txt &
#./server config2.txt &
PID_SERVER=$!

sleep 2


./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W breve5.mp3" &
#./client "-p -f sockfile -t 0 -D salvataggio/fileEspulsi -OL -W paesaggio.jpeg,paesaggio1.jpeg " &
#./client "-p -f sockfile -t 0 -D salvataggio/fileEspulsi -OL -W paesaggio2.jpeg,paesaggio3.jpeg " &
#./client "-p -f sockfile -t 0 -D salvataggio/fileEspulsi -OL -W paesaggio4.jpeg,paesaggio5.jpeg" &
#./client " -p -t 0 -f sockfile -D salvataggio/fileEspulsi -OL -a file80.txt,file81.txt" &

#./client "-p -t 0 -f sockfile  -D salvataggio/fileEspulsi -OL -W file1.txt" &

#./client "-p -t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file40.txt" &

#./client "-p -t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio6.jpeg,paesaggio7.jpeg,paesaggio8.jpeg,paesaggio9.jpg" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio10.jpeg,paesaggio11.jpeg,paesaggio12.jpeg,paesaggio13.jpeg" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio12.jpeg,paesaggio13.jpeg" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio14.jpeg,paesaggio15.jpeg,paesaggio16.jpeg,paesaggio17.jpeg" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio16.jpeg,paesaggio17.jpeg" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio18.jpeg,paesaggio19.jpeg,paesaggio20.jpeg,paesaggio21.jpeg" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W breve.mp3" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio12.jpeg,paesaggio13.jpeg,paesaggio14.jpeg" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio17.jpeg,paesaggio18.jpeg" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio19.jpeg" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio15.jpeg,paesaggio16.jpeg" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W file4.txt,file5.txt,file6.txt" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W file31.txt,file32.txt,file33.txt" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W breve4.mp3" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W file34.txt,file35.txt,file36.txt,file37.txt" &


./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W file42.txt,file43.txt,file44.txt" &

./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W file45.txt,file46.txt" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -OL -W breve.mp3" &


#./client "-p -f sockfile -d salvataggio/fileLetti -R n=6" &





sleep 5


kill -SIGHUP $PID_SERVER
#wait $PID_SERVER

echo "-------------------------------------------------"
./statistiche.sh log.txt 
echo "-------------------------------------------------"

