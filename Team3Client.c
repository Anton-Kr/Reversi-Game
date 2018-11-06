/*Authors:Kristin Curry, Anton Krutyakov, Sean Nonnemacher
Program Name: Team3Client.c
Program Location: /users/student/curryk4/cs352-team/git
Date of Last Modification: 11/7/17
Compile: cc -o client Team3Client.c
Run: ./client
Description: This program is the client that sends the moves the player indicates to the server
   and receives messages from the server.  Each client is a player in the Reversi game.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#define __POSIX
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXRCVLEN 500
#define PORTNUM 17300

/*
CONDITION CODES:
0 - valid move
1 - invalid move
2 - win message
3 - loss message
4 - Your turn
5- tie message
6 - board message
7 - enter credentials
*/ 
pthread_t *listenerThread;
pthread_t *scannerThread;
pthread_t *senderThread;
void printBoard(char *string);
char senderbuff[1];
char gamebuff[1];
sem_t sender_empty;
sem_t sender_full;
sem_t game_empty;
sem_t game_full;

int main(int argc, char *argv[])
{
   listenerThread = (pthread_t *)malloc(sizeof(pthread_t) * maxGameThreads);
   scannerThread = (pthread_t *)malloc(sizeof(pthread_t) * maxGameThreads); 
   senderThread = (pthread_t *)malloc(sizeof(pthread_t) * maxGameThreads);
   char buffer[MAXRCVLEN + 1]; /* +1 so we can add null terminator */
   char buffer1[MAXRCVLEN + 1];
   char message1[MAXRCVLEN + 1];
   char message2[MAXRCVLEN + 1];
   buffer[0] = '7';
   int len, gamesocket;
   struct sockaddr_in dest; 
   char string[MAXRCVLEN+1];
  
   memset(&dest, 0, sizeof(dest));                /* zero the struct */
   dest.sin_family = AF_INET; 
   dest.sin_port = htons(PORTNUM);                /* set destination port number */
   inet_pton(AF_INET,"134.198.169.2",&(dest.sin_addr));
   int connectstatus = -1;
   int connectstatus1 = -1;
   gamesocket = socket(AF_INET, SOCK_STREAM, 0);
   msgsocket = socket(AF_INET, SOCK_STREAM, 0);
   while(connectstatus == -1){
      connectstatus = connect(gamesocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
   }
   while(connectstatus1 == -1){
      connectstatus1 = connect(msgsocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
   }
   
<<<<<<< HEAD
   int *msgsocketPtr =  malloc(sizeof(*msgsocketPtr));
   *msgsocketPtr = msgsocket;
   
   pthread_create(&listenerThread[top], &attr, listener, msgsocketPtr);
   pthread_create(&scannerThread[top], &attr, scanner, NULL);
   pthread_create(&senderThread[top], &attr, sender, msgsocketPtr);
   
   while(buffer[0] != '3' && buffer[0] !='2' && buffer[0] !='5'){
      recv(gamesocket, buffer, MAXRCVLEN, 0);
      if(buffer[0] == '1'){ 
         printf("Your move was not valid. Please enter a valid move. \n");
         sem_wait(&game_empty);
         send(gamesocket, buffer, 4, 0);
         send(gamesocket, message2, 4, 0);
      }
      else if(buffer[0] == '4'){ //Main "enter your move" prompt
         printf("Please enter your move. \n");
         sem_wait(&game_empty);
         send(gamesocket, message1, 4, 0);
         send(gamesocket, message2, 4, 0);
=======
   while(buffer[0] != '3' && buffer[0] != '2' && buffer[0] != '5'){
      recv(mysocket, buffer, MAXRCVLEN, 0);
      if(buffer[0] == '1'){ 
         printf("Your move was not valid. Please enter a valid move. \n");
         do{
            scanf("%127s",message1);
            if(strcmp(message1, "\0") != 0){
               send(mysocket, message1, MAXRCVLEN, 0);
            }
         }while(message1[0] == '/' || strcmp(message1, "\0") == 0);
      }
      else if(buffer[0] == '4'){ //Main "enter your move" prompt
         printf("Please enter your move. \n");
         do{
            do{
               scanf("%s", message1);
               strcat(string, strcat(" ", message1));
            }while(strcmp(message1, "\\"));
            if(strcmp(message1, "\0") != 0){
               printf("%d\n", strcmp(message1, "\0"));
               send(mysocket, message1, MAXRCVLEN, 0);
            }
         }while(message1[0] == '/' || strcmp(message1, "\0") == 0);
>>>>>>> 541ebec3054f163efebdb5447a6d229b2bac7e30
      }
      else if(buffer[0] == '6'){
         printf("Printing board \n");
         printBoard(buffer);
      }
      else if(buffer[0] == '7'){
         printf("Please enter your username and password \n");
<<<<<<< HEAD
		   sem_wait(&game_empty);
         send(gamesocket, message1, MAXRCVLEN, 0);
         send(gamesocket, message2, MAXRCVLEN, 0);
=======
	 scanf("%s %s", message1, message2);
         send(mysocket, message1, MAXRCVLEN, 0);
         send(mysocket, message2, MAXRCVLEN, 0);
         strcpy(message1, "\0");
      }
      else if(buffer[0]=='/'){
         printf("%s\n",buffer);
>>>>>>> 541ebec3054f163efebdb5447a6d229b2bac7e30
      }
   }

   if(buffer[0] == '3'){
      printf("You lost \n");
   }
   else if(buffer[0] == '2'){
      printf("You won \n");
   }
   else if(buffer[0] == '5'){
      printf("Tie!");
   }
   
   printf("L E A D E R B O A R D \n");
   printf("NAME    WINS    LOSSES    TIES \n");
  
<<<<<<< HEAD
   for(int i = 0; i<100; i++){
      len = recv(gamesocket, buffer1, 30, 0);
=======
   for(int i = 0; i < 100; i++){
      len = recv(mysocket, buffer1, 30, 0);
>>>>>>> 541ebec3054f163efebdb5447a6d229b2bac7e30
      //buffer1[5] = '\0'; 
      if(buffer1[1] != '0' && buffer1[3] != '0' && buffer1[5] != '0'){
         printf("%s\n", buffer1);
      }
   }
   buffer[len] = '\0'; 
   close(gamesocket);
   return EXIT_SUCCESS;
}
void *scanner(void *i){
   char *buffer;
   buffer = malloc[MAXRCVLEN+1];
   scanf("%[^\n]", buffer);
   if(buffer[0] == '/'){
      
   }
   else{
      
      
   }
   
   
}
void printBoard(char *string){
   printf("  ");
   for(int i = 0; i < 8; i++){
      printf("%d ",i); 
   }
   printf("\n");
   for(int i = 0; i < 8; i++){
      printf("%d ", i);
      for(int j = 0; j < 8; j++){
        printf("%c ",string[(i*8)+j+1]);
      }
      printf("\n");
   }
   return;
}
