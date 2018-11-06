/*Authors:Kristin Curry, Anton Krutyakov, Sean Nonnemacher
Program Name: Team3Server.c
Program Location: /users/student/curryk4/cs352-team/git
Date of Last Modification: 11/7/17
Compile: cc -lpthread -o server Team3Server.c
Run: ./server
Description: This program is the server for the two player game Reversi.  
   It handles two players connecting to the server and playing the game 
   on two different clients. Then handles all game play including checking 
   different directions and who has won.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#define PORTNUM 17300
#define MAXRCVLEN 500

/*
Codes that define the messages sent to the client in the first char
CONDITION CODES:
0 - valid move
1 - invalid move
2 - win message
3 - loss message
4 - Your turn
5 - tie message
6 - board message
7 - please send us your credentials
*/

typedef struct GAME{
   char **board;
   int size;
   int checkX;
   int checkY;
   char player;
}Game;

typedef struct PLAYERS{
   int player1;
   int player1msg;
   int p1ID;
   int player2;
   int player2msg;
   int p2ID;
}Players;

typedef struct USERS{
   char *username;
   char *password;
   int wins;
   int loses;
   int ties;
} Users;


int flip = 0;
int doFlip=0;
int maxGameThreads = 20;
int joinedPlayers = 0;
int startedGames = 0;
int subServers[20];

Users *playerRecords[100];
Players *players[20];

pthread_mutex_t subServerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t playerRecordsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readWrite = PTHREAD_MUTEX_INITIALIZER;
pthread_t *authenticatorThread;
pthread_t *subServerThread;


void addPlayer(int socketID, int playerID);
void startSubServer();
int checkGameOver(Game *gamePtr);
int checkWhoWon(Game *gamePtr);
bool checkValidMove(Game *gamePtr);
bool checkAvailableMove(Game *gamePtr);
void initializeBoard(Game *gamePtr);
void performMove(Game *gamePtr);
void *checkVertical(void *ptr);
void *checkHorizontal(void *ptr);
void *checkDiagonalRight(void *ptr);
void *checkDiagonalLeft(void *ptr);
void *subServer(void *players);
void insertNewUser(char *username, char *password);
void initializePlayerRecords();
void updateUser(int playerId, int result);
void *authenticator(void *i);
int verifyUser(char *username, char *password);
void initializePlayers();

//Accepts connections from two clients and creates subServer for each game
int main(){
   int tempPlayer = 0;
   initializePlayers();   
   initializePlayerRecords();
   int fd = 0, num = 0;
   int index = 0;
   fd = open("playerRecords.bin", O_RDONLY|O_CREAT, S_IRWXU);
   for(int i = 0; i < 100; i++){
      lseek(fd, i*sizeof(playerRecords), 0);
      num = read(fd, playerRecords[i], sizeof(playerRecords));
      printf("%s\n", playerRecords[i]->username);
   }
   close(fd);

   subServerThread = (pthread_t *)malloc(sizeof(pthread_t) * maxGameThreads);
   authenticatorThread = (pthread_t *)malloc(sizeof(pthread_t) * maxGameThreads);

   struct sockaddr_in dest;
   struct sockaddr_in serv; 

   int mysocket;
               
   socklen_t socksize = sizeof(struct sockaddr_in);
   memset(&serv, 0, sizeof(serv));           
   serv.sin_family = AF_INET;                
   serv.sin_addr.s_addr = htonl(INADDR_ANY); 
   serv.sin_port = htons(PORTNUM);           
   mysocket = socket(AF_INET, SOCK_STREAM, 0);
  
   bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));
   listen(mysocket, 20);
  
   while(joinedPlayers < maxGameThreads * 2){
      int tempPlayer[41];
      int top = 0;
      tempPlayer[top] = accept(mysocket, (struct sockaddr *)&dest, &socksize);
      printf("Player Connected\n");
      int *tempPlayerPtr = malloc(sizeof(*tempPlayerPtr));
      *tempPlayerPtr = tempPlayer[top];
      top++;
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_create(&authenticatorThread[top], &attr, authenticator, tempPlayerPtr);
   }
}

//Runs authentication logic and sends and receives messages to and from client
void *authenticator(void *i){
   int tempPlayer = *((int *) i);
   int playerMsg;
   free(i);
   char bufferUsername[MAXRCVLEN + 1];
   char bufferPassword[MAXRCVLEN + 1];
   int playerid = -1; 
   Players *playerPtr = (Players*)players;
   while(playerid == -1){
      send(tempPlayer, "7", 2, 0);
      int len1 = recv(tempPlayer, bufferUsername, MAXRCVLEN, 0);
      int len2 = recv(tempPlayer, bufferPassword, MAXRCVLEN, 0);
      playerid = verifyUser(bufferUsername, bufferPassword);
   }
   addPlayer(tempPlayer, playerid);
   pthread_exit(NULL);
}

//Runs the game logic and sends and receives messages to and from client
void *subServer(void *players){
   char username[5];
   Players *playerPtr = (Players*)players;
   Game *gamePtr = (Game *)malloc(sizeof(Game));
   char buffer[MAXRCVLEN + 1];
   char buffer1[MAXRCVLEN + 1];
   int won;
   int count = 0;
   gamePtr->player = 'X';
   
   initializeBoard(gamePtr);
   char *codedBoard = (char *) malloc(sizeof(char)*((gamePtr->size * gamePtr->size) + 2));

   do{
      if(checkGameOver(gamePtr) == 1){
         count++;
      }
      int length;
      codedBoard[0] = '6';
      int k = 1;
      for (int i = 0; i < gamePtr->size; i++){ //Encodes the message to be transmitted
         for(int j = 0; j < gamePtr->size; j++){
            codedBoard[k] = gamePtr->board[j][i]; 
            k++;
         }
      }
      send(playerPtr->player1, codedBoard , strlen(codedBoard), 0);
      send(playerPtr->player2, codedBoard , strlen(codedBoard), 0);      
      if(count%2 == 0){
         gamePtr->player = 'X';
         send(playerPtr->player1, "4", 2, 0);
         do{
            strcpy(buffer,"\0");
            int len1 = recv(playerPtr->player1, buffer, MAXRCVLEN, 0);
            printf("%sbuff\n", buffer);
            if(buffer[0]=='/'){
               send(playerPtr->player2, buffer, MAXRCVLEN, 0);
            }
         }while(buffer[0]=='/');
      }
      else if(count%2 == 1){
         gamePtr->player = 'O';
         send(playerPtr->player2, "4", 2, 0);
         do{
            int len1 = recv(playerPtr->player2, buffer, MAXRCVLEN, 0);
            printf("%s buff\n",buffer);
            if(buffer[0]=='/'){
               send(playerPtr->player1, buffer, MAXRCVLEN, 0);
            }
         }while(buffer[0]=='/');
      }
      gamePtr->checkX = buffer[0]-'0';
      gamePtr->checkY = buffer[2]-'0';
      while(!checkValidMove(gamePtr)){
         if(count%2 == 0){
            send(playerPtr->player1, "1", 2, 0);
            do{
               int len1 = recv(playerPtr->player1, buffer, MAXRCVLEN, 0);
               if(buffer[0]=='/'){
                  send(playerPtr->player2, buffer, MAXRCVLEN, 0);
               }
            }while(buffer[0]=='/');
         }
         else if(count%2 == 1){   
            send(playerPtr->player2, "1", 2, 0);
            do{
               int len1 = recv(playerPtr->player2, buffer, MAXRCVLEN, 0);
               if(buffer[0]=='/'){
                  send(playerPtr->player1, buffer, MAXRCVLEN, 0);
               }
            }while(buffer[0]=='/');
         }
         gamePtr->checkX = buffer[0]-'0';
         gamePtr->checkY = buffer[2]-'0';
      }
      performMove(gamePtr);
      count++;   
   }while(checkGameOver(gamePtr) != 2);

   won = checkWhoWon(gamePtr);
   codedBoard[0] = '6';
   int k = 1;
   for (int i = 0; i < gamePtr->size; i++){ //Encodes the message to be transmitted
      for(int j = 0; j < gamePtr->size; j++){
         codedBoard[k] = gamePtr->board[j][i];
         k++;
      }
   }
   if(won == 0){
      send(playerPtr->player1, codedBoard , MAXRCVLEN, 0);
      send(playerPtr->player2, codedBoard, MAXRCVLEN, 0);
      send(playerPtr->player1, "2", MAXRCVLEN, 0);
      send(playerPtr->player2, "3", MAXRCVLEN, 0);
      updateUser(playerPtr->p1ID, 0);
      updateUser(playerPtr->p2ID, 1);
   }
   else if(won == 1){
      send(playerPtr->player1, codedBoard , MAXRCVLEN, 0);
      send(playerPtr->player2, codedBoard , MAXRCVLEN, 0);
      send(playerPtr->player1, "3", MAXRCVLEN, 0);
      send(playerPtr->player2, "2", MAXRCVLEN, 0);
      updateUser(playerPtr->p1ID, 1);
      updateUser(playerPtr->p2ID, 0);
   }
   else{
      send(playerPtr->player1, "5", MAXRCVLEN, 0);
      send(playerPtr->player2, "5", MAXRCVLEN, 0);
      updateUser(playerPtr->p1ID, 2);
      updateUser(playerPtr->p2ID, 2);
   }

   char winschar[MAXRCVLEN +1];
   char loseschar[MAXRCVLEN +1];   
   char tieschar[MAXRCVLEN +1];

   for(int i = 0; i < 100; i++){
      sprintf(winschar, "%d", playerRecords[i]->wins);
      sprintf(loseschar, "%d", playerRecords[i]->loses);
      sprintf(tieschar, "%d", playerRecords[i]->ties);
      strcpy(username, playerRecords[i]->username);  
      strcat(strcat(strcat(strcat(strcat(strcat(username," "), winschar),"    "),loseschar), "    "), tieschar);
      printf("%s\n", username);
      send(playerPtr->player1, username, 30, 0);
      send(playerPtr->player2, username, 30, 0);
   }

   pthread_mutex_lock(&readWrite);
   int fd=0, num=0;
   fd = open("playerRecords.bin", O_RDWR|O_CREAT|O_TRUNC, S_IRWXU);
   for(int i = 0; i < 100; i++){
      lseek(fd, i*sizeof(playerRecords), 0);
      num = write(fd, playerRecords[i], sizeof(playerRecords));
   }
   close(fd);
   pthread_mutex_unlock(&readWrite);
   return (void *) 1;
   pthread_exit(NULL);
}

//Initializes the Player structs in players[]
void initializePlayers(){
   for(int i = 0; i < 20; i++){
      players[i]=(Players *)malloc(sizeof(Players));
   }
}

//Adds player to an open Player struct
void addPlayer(int socketID, int playerID){
   pthread_mutex_lock(&subServerMutex);
   joinedPlayers++;
   if(joinedPlayers == 1){
      players[startedGames]->player1 = socketID;
      players[startedGames]->p1ID = playerID;
   }
   else{
      players[startedGames]->player2 = socketID;
      players[startedGames]->p2ID = playerID;
   }  
   startSubServer();
}

//starts a subserver when two players have been authenticated
void startSubServer(){
      if(joinedPlayers == 2){
         pthread_attr_t attr;
         pthread_attr_init(&attr);
         pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
         pthread_create(&subServerThread[startedGames], &attr, subServer, (void*)players[startedGames]);
         startedGames++;
         joinedPlayers = 0;
      }
   pthread_mutex_unlock(&subServerMutex);
}

//Initializes Users struct in playerRecords[]
void initializePlayerRecords(){
   for(int i = 0; i < 100; i++){
      playerRecords[i] = (Users *)malloc(sizeof(Users));
      playerRecords[i]->username = (char *)malloc(sizeof(char)*30);
      strcpy(playerRecords[i]->username, "\0");
      playerRecords[i]->password = (char *)malloc(sizeof(char)*30);
      strcpy(playerRecords[i]->password,"\0");
      playerRecords[i]->wins = 0;
      playerRecords[i]->loses = 0;
      playerRecords[i]->ties = 0;
   }
   return;	
}

//Initializes game board, sets center pieces, sets rest of board as -
void initializeBoard(Game *gamePtr){
   gamePtr->size = 8;
   gamePtr->board = (char**) malloc(sizeof(char*)*gamePtr->size); 
   for( int i = 0; i < gamePtr->size; i++){
      gamePtr->board[i] = (char*) malloc(sizeof(char)*gamePtr->size);
   }
   for(int i = 0; i < 8; i++){
      for(int j = 0; j < 8; j++){
         (gamePtr->board)[i][j] = '-';
      }
   }
   (gamePtr->board)[3][3] = 'X';
   (gamePtr->board)[3][4] = 'O';
   (gamePtr->board)[4][3] = 'O';
   (gamePtr->board)[4][4] = 'X';
}

/*inserts a new user into the next empty element in the array*/
void insertNewUser(char *username, char *password){
   int i = 0;
   while(strcmp(playerRecords[i]->username, "\0") != 0){
      i++;
   }
   strcpy(playerRecords[i]->username,username);
   strcpy(playerRecords[i]->password,password);
   return;
}

/*Updates user information if they won, lost, or tied*/
void updateUser(int playerId, int result){
   pthread_mutex_lock(&playerRecordsMutex);
   printf("update\n");
   if(result == 0){
      playerRecords[playerId]->wins ++;
   }
   else if(result == 1){
      playerRecords[playerId]->loses ++;
   }
   else{
      playerRecords[playerId]->ties ++;
   }
   printf("%s %d\n",playerRecords[playerId]->username, playerRecords[playerId]->wins);
   pthread_mutex_unlock(&playerRecordsMutex);
   return;
}

/*sees if the user is already created, if they are, check if password is correct,
if not, new user is inserted into the list and send true that the new user has 
been verified.  If password is wrong, send false if password is right, send true 
that the user has been verified
*/
int verifyUser(char *username, char *password){
   pthread_mutex_lock(&playerRecordsMutex);
   int id = -1;
   int i = 0;
   while(i < 100 && strcmp(playerRecords[i]->username, username) != 0 && 
	strcmp(playerRecords[i]->username, "\0") != 0){
      i++;
   }
   if(strcmp(playerRecords[i]->username, username) == 0 && strcmp(playerRecords[i]->password, 
	password)== 0){
      id = i;
   }
   else if(strcmp(playerRecords[i]->username, username) != 0){
      insertNewUser(username, password);
      id = i;
   } 
   pthread_mutex_unlock(&playerRecordsMutex);
   return id;
}

/*Checks if board is full, if one player has a valid move left, and if both players 
have valid moves left
*/
int checkGameOver(Game *gamePtr){
   int result = 2;
   for(int i = 0; i < gamePtr->size; i++){
      for(int j = 0; j < gamePtr->size; j++){
         if(gamePtr->board[i][j] != '-'){
            result = 0;
         }
      }
   }
   if(result == 0){
      if(!checkAvailableMove(gamePtr)){
         result = 1;
         if(gamePtr->player == 'O'){
            gamePtr->player = 'X';
         }
         else{
            gamePtr->player = 'O';  
         }
      }
      if(!checkAvailableMove(gamePtr) && result == 1){
         result = 2;
      }
   }
   return result;
}

//Counts each marker and sees who has more on the board
int checkWhoWon(Game *gamePtr){
   int player1, player2;
   int won = -1;
   for(int i = 0; i < gamePtr->size; i++){
      for(int j = 0; j < gamePtr->size; j++){
         if(gamePtr->board[i][j] == 'X'){
            player1++;
         }
         else if(gamePtr->board[i][j] == 'O'){
            player2++;
         }
      }
   }
   if(player1 > player2){
      won = 0;
   }
   else if(player1 < player2){
      won = 1;
   }
   else{
      won = 2;
   }
   
   return won;
}

/*Checks if the move given by the player is valid by checking all of the different directions,
   if the move is within the bounds of the board, and if it is a free space.*/
bool checkValidMove(Game *gamePtr){
   bool result = true;
   doFlip=0;
   pthread_t threadH, threadV, threadD1, threadD2;

   if(gamePtr->board[gamePtr->checkX][gamePtr->checkY] != '-'  ||
      gamePtr->checkX > gamePtr->size  ||  gamePtr->checkY > gamePtr->size ||
      gamePtr->checkY<0 || gamePtr->checkX<0){
      result = false;
   }

   if(result == true){
      pthread_create(&threadH, NULL, checkHorizontal, gamePtr);
      pthread_create(&threadV, NULL, checkVertical, gamePtr);
      pthread_create(&threadD1, NULL, checkDiagonalRight, gamePtr);
      pthread_create(&threadD2, NULL, checkDiagonalLeft, gamePtr);
      pthread_join(threadH, NULL);
      pthread_join(threadV, NULL);
      pthread_join(threadD1, NULL);
      pthread_join(threadD2, NULL);
      if(flip == 0){
         result = false;
      }
      flip = 0;
   }
  return result;
}

//checks if there is an avalible move for the current player by checking each free spot
bool checkAvailableMove(Game *gamePtr){
   bool result = false;
   char player = gamePtr->player;
   pthread_t threadH, threadV, threadD1, threadD2;
  
   for(int i = 0; i < gamePtr->size; i++){
      for(int j = 0; j < gamePtr->size; j++){
         if(gamePtr->board[i][j] == '-'){
            gamePtr->board[i][j] = player;
            gamePtr->checkX = i;
            gamePtr->checkY = j;
            pthread_create(&threadH, NULL, checkHorizontal, gamePtr);
            pthread_create(&threadV, NULL, checkVertical, gamePtr);
            pthread_create(&threadD1, NULL, checkDiagonalRight, gamePtr);
            pthread_create(&threadD2, NULL, checkDiagonalLeft, gamePtr);
            pthread_join(threadH, NULL);   
            pthread_join(threadV, NULL);
            pthread_join(threadD1, NULL);
            pthread_join(threadD2, NULL);
            if(flip == 1){
               result = true;
            }
            flip = 0;
            gamePtr->board[i][j] = '-';
         }
      }
   }
   return result;
}

//Performs the move the player indicated
void performMove(Game *gamePtr){
   doFlip = 1;
   pthread_t threadH, threadV, threadD1, threadD2;
   gamePtr->board[gamePtr->checkX][gamePtr->checkY] = gamePtr->player;

   pthread_create(&threadH, NULL, checkHorizontal, gamePtr);
   pthread_create(&threadV, NULL, checkVertical, gamePtr);
   pthread_create(&threadD1, NULL, checkDiagonalRight, gamePtr);
   pthread_create(&threadD2, NULL, checkDiagonalLeft, gamePtr);
   pthread_join(threadH, NULL);
   pthread_join(threadV, NULL);
   pthread_join(threadD1, NULL);
   pthread_join(threadD2, NULL);

   doFlip = 0;
   flip = 0;
   if(gamePtr->player == 'X'){
      gamePtr->player = 'O';
   }
   else{
      gamePtr->player = 'X';
   }
   return;
}

//Checks horizontal direction both left and right if move makes flips
void *checkHorizontal(void *ptr){
   Game *gamePtr = (Game*)ptr;
   int row = gamePtr->checkX;
   int col = gamePtr->checkY;  
   char player = gamePtr->player;
   int check = col;
   char otherPlayer;
   if(player == 'X'){
      otherPlayer = 'O';
   }
   else{
      otherPlayer = 'X';
   }
   if(check != 0){
      check--;
      while(check >= 0 && gamePtr->board[row][check] == otherPlayer && 
         check-1 >= 0){
         check--;
      }
      if(gamePtr->board[row][check] == player && check < col-1){
         flip = 1;
         if(doFlip == 1){
            check++;
            while(gamePtr->board[row][check] == otherPlayer){
               gamePtr->board[row][check] = player;
               check++;
            }
         }
      }
   }
   check = col;
   if(check != gamePtr->size-1){
      check++;
      while(check < gamePtr->size && gamePtr->board[row][check] == otherPlayer && 
         check+1 < gamePtr->size){
         check++;
      }
      if(gamePtr->board[row][check] == player && check > col+1){
         flip = 1;
         if(doFlip == 1){
           check--;
           while(gamePtr->board[row][check] == otherPlayer){
              gamePtr->board[row][check] = player;
              check--;
           }
         }
      }
   }
   return NULL;
}

//Checks vertical up and down to see if the move makes flips
void *checkVertical(void *ptr){
   Game *gamePtr = (Game*)ptr;
   int row = gamePtr->checkX;
   int col = gamePtr->checkY;
   char player = gamePtr->player;
   int check = row;
   char otherPlayer;
   if(player == 'X'){
      otherPlayer = 'O';
   }
   else{
      otherPlayer = 'X';
   }
   
   if(check != 0){
      check--;
      while(check >= 0 && gamePtr->board[check][col] == otherPlayer && check-1 >= 0){
         check--;
      }
      if(gamePtr->board[check][col] == player && check < row-1){
         flip = 1;
         if(doFlip == 1){
            check++;
            while(gamePtr->board[check][col] == otherPlayer){
               gamePtr->board[check][col] = player;
               check++;
            }
         }
      }
   }
   check = row;  
   if(check != gamePtr->size-1){
      check++;
      while(check < gamePtr->size && gamePtr->board[check][col] == otherPlayer && 
         check+1 < gamePtr->size){
         check++;   
      }
      if(gamePtr->board[check][col] == player && check > row+1){
         flip = 1;
         if(doFlip == 1){
            check--;
            while(gamePtr->board[check][col] == otherPlayer){
               gamePtr->board[check][col] = player;
               check--;
            }
         }
      }
   }
   return NULL;
}

//Checks diagonal to the left to see if move makes flips
void *checkDiagonalLeft(void *ptr){
   Game *gamePtr = (Game*)ptr;
   int row = gamePtr->checkX;
   int col = gamePtr->checkY;
   char player = gamePtr->player;
   int checkX = row;
   int checkY = col;
   char otherPlayer;
   if(player == 'X'){
      otherPlayer = 'O';
   }   
   else{
      otherPlayer = 'X';
   }   
   if(checkX != 0 && checkY != 0){
      checkX--;
      checkY--;
      while(checkX >= 0 && checkY >= 0 && 
            gamePtr->board[checkX][checkY] == otherPlayer && checkX-1 >= 0 && 
            checkY-1 >= 0){
         checkX--;
         checkY--;
      }
      if(gamePtr->board[checkX][checkY] == player && checkX < row-1 && 
         checkY < col-1){
         flip = 1;
         if(doFlip == 1){
            checkX++;
            checkY++;
            while(gamePtr->board[checkX][checkY] == otherPlayer){
               gamePtr->board[checkX][checkY] = player;
               checkX++;
               checkY++;
            }
         }
      }
   }
   checkX = row;
   checkY = col;
   if(checkX != gamePtr->size-1 && checkY != gamePtr->size-1){
      checkX++;
      checkY++;
      while(checkX < gamePtr->size && checkY < gamePtr->size &&
         gamePtr->board[checkX][checkY] == otherPlayer && 
         checkX+1 < gamePtr->size && checkY+1 < gamePtr->size){
         checkX++;
         checkY++;  
      }
      if(gamePtr->board[checkX][checkY] == player && checkX > row+1  && 
           checkY > col+1){
         flip = 1;  
         if(doFlip == 1){
            checkX--;
            checkY--;
            while(gamePtr->board[checkX][checkY] == otherPlayer){
               gamePtr->board[checkX][checkY] = player;
               checkX--;
               checkY--;
            }
         }
      }
   }   
   return NULL;
}

//Check diagonal to the right to see if move makes flips
void *checkDiagonalRight(void *ptr){
   Game *gamePtr = (Game*)ptr;
   int row = gamePtr->checkX;
   int col = gamePtr->checkY;
   char player = gamePtr->player;
   int checkX = row;
   int checkY = col;
   char otherPlayer;
   if(player == 'X'){
      otherPlayer = 'O';
   }   
   else{
      otherPlayer = 'X';
   }
   if(checkX != gamePtr->size-1 && checkY != 0){
      checkX++;
      checkY--;
      while(checkX < gamePtr->size && checkY >= 0 && 
         gamePtr->board[checkX][checkY] == otherPlayer && checkX+1 < gamePtr->size 
         && checkY-1 >= 0){
         checkX++;
         checkY--;
      }
      if(gamePtr->board[checkX][checkY] == player && checkX > row+1 && 
         checkY < col-1){
         flip = 1;
         if(doFlip == 1){
            checkX--;
            checkY++;
            while(gamePtr->board[checkX][checkY] == otherPlayer){
               gamePtr->board[checkX][checkY] = player;
               checkX--;
               checkY++;
            }
         }
      }
   }   
   checkX = row;
   checkY = col;
   if(checkX != 0 && checkY != gamePtr->size-1){
      checkX--;
      checkY++;
      while(checkX >= 0 && checkY<gamePtr->size &&
         gamePtr->board[checkX][checkY] == otherPlayer && checkX-1 >= 0 && 
         checkY+1 < gamePtr->size){
         checkX--;
         checkY++;
      }
      if(gamePtr->board[checkX][checkY] == player && checkX < row-1  && 
            checkY > col+1){
         flip = 1;
         if(doFlip == 1){
            checkX++;
            checkY--;
            while(gamePtr->board[checkX][checkY] == otherPlayer){
               gamePtr->board[checkX][checkY] = player;
               checkX++;
               checkY--;
            }
         }
      }
   }
   return NULL;
}   

