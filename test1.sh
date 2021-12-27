#!/bin/bash
./client "-p -f sockfile " &
./client "-p -f sockfile " &


#./server config.txt &
valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all -s ./server config.txt &
PID_SERVER=$!
sleep 3
kill -SIGHUP $PID_SERVER
#wait $PID_SERVER
