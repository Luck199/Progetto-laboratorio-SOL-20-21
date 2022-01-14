#!/bin/bash
./client "-p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file1.txt,file2.txt,file3.txt,file14.txt,file15.txt,file26.txt,file27.txt" &
./client "-p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file4.txt,file5.txt,file6.txt,file7.txt,file13.txt,file16.txt" &
./client "-p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file8.txt,file9.txt,file10.txt,file11.txt,file12.txt,file18.txt,file19.txt,file20.txt" &
./client "-p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file31.txt,file32.txt,file33.txt,file34.txt,file35.txt,file36.txt,file37.txt" &
./client "-p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file42.txt,file43.txt,file44.txt,file45.txt,file46.txt" &
./client "-p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file50.txt,file51.txt,file52.txt,file53.txt,file54.txt" &
./client "-p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file55.txt,file56.txt,file57.txt,file58.txt,file59.txt" &
./client "-p -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file60.txt,file61.txt,file62.txt,file63.txt,file64.txt  -d /home/luca/workspace2/FileStorageServer/salvataggio/fileLetti -R 2" &
./server config2.txt &
PID_SERVER=$!
sleep 3
kill -SIGHUP $PID_SERVER

#wait $PID_SERVER
