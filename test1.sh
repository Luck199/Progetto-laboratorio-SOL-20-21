#!/bin/bash

./client "-p -f sockfile " &
./client "-p -f sockfile " &




#./client "-p -f sockfile " &./client "-p -f sockfile " &



#valgrind  --leak-check=full ./server config.txt &
./server config.txt &
PID_SERVER=$!
sleep 3
kill -SIGINT $PID_SERVER
#wait $PID_SERVER
