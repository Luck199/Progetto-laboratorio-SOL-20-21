comandiClients=(

 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W breve5.mp3,breve4.mp3" 
 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio.jpeg,paesaggio1.jpeg " 
 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio2.jpeg,paesaggio3.jpeg " 
 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio4.jpeg,paesaggio5.jpeg" 
 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -a file80.txt,file81.txt" 
 "-t 0 -f sockfile -l paesaggio5.jpeg,paesaggio2.jpeg -c paesaggio5.jpeg,paesaggio2.jpeg"


 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W paesaggio20.jpeg"
 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W breve.mp3,file6.txt" 
 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file4.txt,file5.txt" 

 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file6.txt,file7.txt" 
  "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file8.txt,file9.txt" 
   "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file10.txt,file11.txt" 
    "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file12.txt,file13.txt" 
     "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file14.txt,file15.txt" 
      "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file16.txt,file17.txt" 
       "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file18.txt,file19.txt" 
        "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file20.txt,file21.txt" 
         "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file22.txt,file23.txt" 
          "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file24.txt,file25.txt" 

          
          
 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -a file10.txt,file11.txt " 

 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W file42.txt,classica.mp3" 
 

 "-t 0 -f sockfile -d salvataggio/fileLetti -L -r file1.txt,file4.txt"
 "-t 0 -f sockfile -d salvataggio/fileLetti -OL -R n=10000,file4.txt" 
 #"-t 0 -f sockfile -l breve.mp3,file1.txt -c breve.mp3,file1.txt"
 "-t 0 -f sockfile -D salvataggio/fileEspulsi -OL -W breve3.mp3,file3.txt" 

)


MY_ID=${ID}


while true 
do
    
    i=$(( RANDOM % ${#comandiClients[@]}))
    ./client "${comandiClients[i]}"


done

exit 0




