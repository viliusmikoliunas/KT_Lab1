#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>

#define BUFFLEN 1024
#define MAXCLIENTS 10

static const int lives = 5;
#define maxWordLength 20
#define maxUsernameLength 50

char* GenerateUserString(char* word)
{
	char* userString = malloc(strlen(word));
	for(int i=0;i<strlen(word);i++)
	{
		*(userString+i) = '_';
	}
	return userString;
}

int GetRandomNumber(int max)
{
    time_t t;
    srand((unsigned) time(&t));
    return rand()%max;
}
char* GetRandomWord()
{
    FILE *fd;
    int wordLength = 50;

    char currentWord[wordLength];
    char words[100][wordLength];
    int wordCount=0;

    fd = fopen("WordBank.txt","r");
    while(fgets(currentWord,wordLength,fd)!=NULL)
    {
        strncpy(words[wordCount],currentWord,strlen(currentWord));
        wordCount++;
    }
    fclose(fd);
    //choosing random word
    static char *answer;
    answer = words[GetRandomNumber(wordCount)];
    return answer;
}

int findemptyuser(int c_sockets[]){
    int i;
    for (i = 0; i <  MAXCLIENTS; i++){
        if (c_sockets[i] == -1){
            return i;
        }
    }
    return -1;
}

struct hangmanGame{
	int lives[MAXCLIENTS];
	char words[MAXCLIENTS][maxWordLength];
	char userString[MAXCLIENTS][maxWordLength];
	char username[MAXCLIENTS][maxUsernameLength];
}hangman;

void SetupNewUser(int c_sockets[], int id)
{
	//recv(c_sockets[id],&hangman.username[id],maxUsernameLength,0);//get username
	memcpy(hangman.username[id],"TServ00",strlen("TServ00"));
	hangman.lives[id] = lives;
	
	char* tempWord = GetRandomWord();
	memcpy(hangman.words[id],tempWord,strlen(tempWord));
	
	char* tempUserString = GenerateUserString(tempWord);
	memcpy(hangman.userString[id],tempUserString,strlen(tempUserString));
}

int main(int argc, char *argv[]){
    unsigned int port;
    unsigned int clientaddrlen;
    int l_socket;
    int c_sockets[MAXCLIENTS];
    fd_set read_set;

    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;

    int maxfd = 0;
    int i;

    char buffer[BUFFLEN];
/*
    if (argc != 2){
        fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
        return -1;
    }
*/

    //port = atoi(argv[1]);
	port = atoi("7896");
	/*
    if ((port < 1) || (port > 65535)){
        fprintf(stderr, "ERROR #1: invalid port specified.\n");
        return -1;
    }*/

    if ((l_socket = socket(AF_INET, SOCK_STREAM,0)) < 0){
        fprintf(stderr, "ERROR #2: cannot create listening socket.\n");
        return -1;
    }

    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port);

    if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #3: bind listening socket.\n");
        return -1;
    }

    if (listen(l_socket, 5) <0){
        fprintf(stderr,"ERROR #4: error in listen().\n");
        return -1;
    }                           

    for (i = 0; i < MAXCLIENTS; i++){
        c_sockets[i] = -1;
    }


    for (;;){
        FD_ZERO(&read_set);
        for (i = 0; i < MAXCLIENTS; i++){
            if (c_sockets[i] != -1){
                FD_SET(c_sockets[i], &read_set);
                if (c_sockets[i] > maxfd){
                    maxfd = c_sockets[i];
                }
            }
        }

        FD_SET(l_socket, &read_set);
		maxfd = (l_socket > maxfd) ? l_socket : maxfd;
        
        select(maxfd+1, &read_set, NULL , NULL, NULL);

        if (FD_ISSET(l_socket, &read_set)){
            int client_id = findemptyuser(c_sockets);
            if (client_id != -1){
                clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                c_sockets[client_id] = accept(l_socket, 
                    (struct sockaddr*)&clientaddr, &clientaddrlen);
					
				SetupNewUser(c_sockets,client_id);
                printf("Connected: %s , Username:%s\n",inet_ntoa(clientaddr.sin_addr)
					,hangman.username[client_id]);
				
            }
        }
        for (i = 0; i < MAXCLIENTS; i++){
            if (c_sockets[i] != -1){
                if (FD_ISSET(c_sockets[i], &read_set)){
                    memset(&buffer,0,BUFFLEN);
                    int r_len = recv(c_sockets[i],&buffer,BUFFLEN,0);
					int w_len = send(c_sockets[i], buffer, strlen(buffer),0);
					
					if(w_len<=0)
					{
						close(c_sockets[i]);
						c_sockets[i] = -1;
						//printf("Disconnected: %s , Username:%s\n",inet_ntoa(clientaddr.sin_addr));
					}

                }
            }
        }
    }

    return 0;
}
