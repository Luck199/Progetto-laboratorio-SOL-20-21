#!/bin/bash

./client "-p -f sockfile -W file2.txt -W file1.txt" &

#./client "-p -f sockfile " &./client "-p -f sockfile " &


#valgrind  --leak-check=full --show-leak-kinds=all -s --track-origins=yes ./server config.txt &
./server config.txt &
PID_SERVER=$!
sleep 2
kill -SIGHUP $PID_SERVER

#wait $PID_SERVER
