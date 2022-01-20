comandiClients=(

   #"  -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -d /home/luca/workspace2/FileStorageServer/salvataggio/fileLetti -W file102.txt -u file102.txt -r file102.txt -c file102.txt -W file101.txt,prova.txt -R 2 -l config.txt -u config.txt"
 	#" -f sockfile -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -d /home/luca/workspace2/FileStorageServer/salvataggio/fileLetti -W file1.txt -r file1.txt -R 2 -c file1.txt -w /home/luca/workspace2/FileStorageServer/salvataggio 2" 
	"-f sockfile   -W file1.txt,file2.txt,file3.txt " 
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file4.txt,file5.txt,file6.txt,file7.txt" 
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file8.txt,file9.txt,file10.txt,file11.txt" 
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file12.txt,file13.txt,file14.txt,file15.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file16.txt,file17.txt,file18.txt,file19.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file20.txt,file21.txt,file22.txt,file23.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file24.txt,file25.txt,file26.txt,file27.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file28.txt,file29.txt,file30.txt,file31.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file32.txt,file33.txt,file34.txt,file35.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file36.txt,file37.txt,file38.txt,file39.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file40.txt,file41.txt,file42.txt,file43.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file44.txt,file45.txt,file46.txt,file47.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file48.txt,file49.txt,file50.txt,file51.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file51.txt,file52.txt,file53.txt,file54.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file55.txt,file56.txt,file57.txt,file58.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file59.txt,file60.txt,file61.txt,file62.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file63.txt,file64.txt,file65.txt,file66.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file67.txt,file68.txt,file69.txt,file70.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file71.txt,file72.txt,file73.txt,file74.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file75.txt,file76.txt,file77.txt,file78.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file79.txt,file80.txt,file81.txt,file82.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file83.txt,file84.txt,file85.txt,file86.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file87.txt,file88.txt,file89.txt,file90.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file91.txt,file92.txt,file93.txt,file94.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file95.txt,file96.txt,file97.txt,file98.txt"
	#"-f sockfile  -D /home/luca/workspace2/FileStorageServer/salvataggio/fileEspulsi -W file99.txt,file100.txt"

)



while true 
do
    
    i=$(( RANDOM % ${#comandiClients[@]}))
    ./client "${comandiClients[i]}"
    sleep 4

done

exit 0
