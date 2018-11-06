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
#include <time.h>
  
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

void printBoard(char *string);

int main(int argc, char *argv[])
{
   char buffer[MAXRCVLEN + 1]; /* +1 so we can add null terminator */
   char buffer1[MAXRCVLEN+1];
   char message1[MAXRCVLEN +1];
   char message2[MAXRCVLEN +1];
   srand(time(NULL));
   buffer[0] = '7';
   int len, mysocket;
   struct sockaddr_in dest; 
   int timeout = 0;
   memset(&dest, 0, sizeof(dest));                /* zero the struct */
   dest.sin_family = AF_INET; 
   dest.sin_port = htons(PORTNUM);                /* set destination port number */
   inet_pton(AF_INET,"134.198.169.2",&(dest.sin_addr));
   int connectstatus = -1;
   mysocket = socket(AF_INET, SOCK_STREAM, 0);
      
   while(connectstatus == -1){
      connectstatus = connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
   }
   
   while(buffer[0] != '3' && buffer[0] !='2' && buffer[0] !='5' && timeout < 200){
      recv(mysocket, buffer, MAXRCVLEN, 0);
      if(buffer[0] == '1'){ 
      /*
         printf("Your move was not valid. Please enter a valid move. \n");
         scanf("%s %s", message1, message2);
         */
         printf("TRANSMIT \n");
         int x = rand() % 8;
         int y = rand() % 8;
         sprintf(message1, "%d", x);
         sprintf(message2, "%d", y);
         timeout++;
         send(mysocket, message1, 4, 0);
         send(mysocket, message2, 4, 0);
      }
      else if(buffer[0] == '4'){ //Main "enter your move" prompt
         printf("TRANSMIT \n");
         int x = rand() % 8;
         int y = rand() % 8;
         sprintf(message1, "%d", x);
         sprintf(message2, "%d", y);
         timeout++;
         send(mysocket, message1, 4, 0);
         send(mysocket, message2, 4, 0);
      }
      else if(buffer[0] == '6'){
         printf("Printing board \n");
         printBoard(buffer);
      }
	  else if(buffer[0] == '7'){
         printf("Please enter your username and password \n");
		   scanf("%s %s", message1, message2);
         send(mysocket, message1, MAXRCVLEN, 0);
         send(mysocket, message2, MAXRCVLEN, 0);
      }
      timeout = 0;
   }

   if(buffer[0]=='3'){
      printf("You lost \n");
   }
   else if(buffer[0]=='2'){
      printf("You won \n");
   }
   else if(buffer[0]=='5'){
      printf("Tie!");
   }
   

   printf("L E A D E R B O A R D \n");
   printf("NAME    WINS    LOSSES    TIES \n");

   for(int i = 0; i<100; i++){
      len = recv(mysocket, buffer1, 30, 0);
      //buffer1[5] = '\0';
      if(buffer[0]!='0' && buffer[2]!='0' && buffer[4]!='0'){
         printf("%s\n", buffer1);
      }
   }
   close(mysocket);
   return EXIT_SUCCESS;
}

void printBoard(char *string){
   printf("  ");
   for(int i=0; i<8; i++){
      printf("%d ",i); 
   }
   printf("\n");
   for(int i=0; i<8; i++){
      printf("%d ", i);
      for(int j=0; j<8; j++){
        printf("%c ",string[(i*8)+j+1]);
      }
      printf("\n");
   }
   return;
}
