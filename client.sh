client=(
  '-p -f sockfile -t 0 '
  '-p -f sockfile -t 0 '
  '-p -f sockfile -t 0 '
  '-p -f sockfile -t 0 '
)

MY_ID=${ID}

while true 
do
    i=$(( RANDOM % ${#client[@]}))
    ./client ${client[i]} & 
done

exit 0
