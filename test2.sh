#!/bin/bash

valgrind  --leak-check=full --show-leak-kinds=all -s ./server config2.txt &
#./server config2.txt &
PID_SERVER=$!



./client "-p -f sockfile -D salvataggio/fileEspulsi -W file1.txt,file2.txt,file3.txt,file4.txt,ibra.jpg" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -W file9.txt,file5.txt,file6.txt,file7.txt,file8.txt" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -W ibra3.jpg,ibra4.jpg,ibra2.jpg,ibra14.jpg,ibra15.jpg" &
#./client "-p -f sockfile  -D salvataggio/fileEspulsi -W file1.txt" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -W file4.txt,file5.txt,file6.txt,file7.txt,file8.txt" &
#./client "-p -f sockfile -D salvataggio/fileEspulsi -W ibra3.jpg" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -W ibra5.jpg,ibra10.jpg,ibra11.jpg,ibra12.jpg,ibra13.jpg" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -W ibra6.jpg,ibra8.jpg,ibra9.jpg" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -W file31.txt,file32.txt,file33.txt,file34.txt,file35.txt,file36.txt,file37.txt" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -W file42.txt,file43.txt,file44.txt,file45.txt,file46.txt" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -W file50.txt,file51.txt,file52.txt,file53.txt,file54.txt" &
./client "-p -f sockfile -D salvataggio/fileEspulsi -W file55.txt,file56.txt,file57.txt,file58.txt,file59.txt" &
#./client "-p -f sockfile -D /salvataggio/fileEspulsi -W file60.txt,file61.txt,file62.txt,file63.txt,file64.txt,brevi.mp3" &
#./client "-p -f sockfile -D /salvataggio/fileEspulsi -W musica.mp3" &



sleep 3
kill -SIGHUP $PID_SERVER

#wait $PID_SERVER
