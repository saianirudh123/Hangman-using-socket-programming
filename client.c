#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#define BACKLOG 10

void error(char *msg)
{
	perror(msg);
	exit(0);
}

char* player1();
void player2(int oppSockfd, char wordStr[256]);
void player12(int oppSockfd, char wordStr[256]);

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr,"usage: %s hostname port\n", argv[0]);
		exit(0);
	}
	
	int sockfd, portno, n;
	char buffer[256];
	struct sockaddr_in serv_addr;
	struct hostent *server;

	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such server\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
	serv_addr.sin_port = htons(portno);
	if ( connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){ 
		error("ERROR connecting");
	}
	printf("\n---Connected to the server---\n");
	
	char oppIndex[256];
	bzero(buffer, 256);
	n = read(sockfd,buffer,256);
	if (n < 0) 
		error("ERROR reading from socket");
	bcopy(buffer,oppIndex,256);
	bzero(buffer,256);
	//---------------------------------------------------------------------------------------------------------------
	if (!strncmp(oppIndex,"ODD",3)){      //ODD clients
		printf("Give a port no through which ur opponent can connect with you: ");
		bzero(buffer,256);
		fgets(buffer,255,stdin);
		n = write(sockfd,buffer,strlen(buffer)-1);
		if (n < 0) 
			 error("ERROR writing to socket");
		close(sockfd);
		//-------------------END OF SERVER COMMUNICATION----------------------------
		int listeningSockfd, oppSockfd, sin_size, myportno;
		myportno = atoi(buffer);
		struct sockaddr_in my_addr, opp_addr;
	
		listeningSockfd = socket(PF_INET, SOCK_STREAM, 0);
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(myportno);
		my_addr.sin_addr.s_addr = INADDR_ANY;
		memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
		bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr);
		int yes=1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1) {
			perror("setsockopt");
			exit(1);
		}
		listen(listeningSockfd, BACKLOG);
		printf("\n---Waiting to connect to a opponent---\n");

		sin_size = sizeof opp_addr;
		oppSockfd = accept(listeningSockfd, (struct sockaddr *)&opp_addr, &sin_size);
		if (oppSockfd < 0) 
			error("ERROR accepting");
		printf("\nCONNECTED to opponent\n\n");
		//----------------CONNECTION ESTABLISHED----------------------------
		while (1) {
			char* wordStr;
			wordStr = (char*)malloc(sizeof(char));
			char temp[256],temp2[256];
			wordStr = player1();
			bcopy(wordStr,temp,256);
			bcopy(wordStr,temp2,256);
			printf("\n...waiting for the result...\n\n");
			n = write(oppSockfd,temp,256);
			if (n < 0) 
				 error("ERROR writing to socket");
			player12(oppSockfd, temp2);
			//--------------ONE TURN DONE------------------------
			printf("---Wait till your opponent gives a word to guess---\n");
			char tempWordStr[256];
			bzero(buffer,256);
			n = read(oppSockfd,buffer,256);
			if (n < 0) 
				 error("ERROR reading from socket");
			bcopy(buffer,tempWordStr,256);
			player2(oppSockfd, tempWordStr);
		}
	}
	//-----------------------------------------------------------------------------------------------------------------
	else if (!strncmp(oppIndex,"EVEN",4)) {         //EVEN clients
		bzero(buffer,256);
		n = read(sockfd,buffer,256);
		if (n < 0) 
			error("ERROR reading from socket");
		close(sockfd);
		//------------------------------------------------------------------------------
		int i,tempIdx,oppPortno;
		char ip_addr[256];
		char port_num[256];
		for (i=0;i<strlen(buffer);i++) {
			if (buffer[i] == '|') {
				ip_addr[i]='\0';
				tempIdx = i+1;
				break;
			}
			ip_addr[i] = buffer[i];
		}
		for (i=tempIdx;i<strlen(buffer);i++) {
			port_num[i-tempIdx] = buffer[i];
		}
		port_num[strlen(buffer)-(strlen(ip_addr)+1)] = '\0';
		oppPortno = atoi(port_num);
		//--------------------END OF SERVER COMMUNICATION-------------------------------
		printf("\n---Establishing connection with a player---\n");
		int oppSockfd;
		struct sockaddr_in opp_addr;
		struct hostent *opponent;

		oppSockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (oppSockfd < 0) 
			error("ERROR opening socket");
		opponent = gethostbyname(ip_addr);
		if (opponent == NULL) {
			fprintf(stderr,"ERROR, no such opponent\n");
			exit(0);
		}
		bzero((char *) &opp_addr, sizeof(opp_addr));
		opp_addr.sin_family = AF_INET;
		bcopy((char *)opponent->h_addr, 
		     (char *)&opp_addr.sin_addr.s_addr,
		     opponent->h_length);
		opp_addr.sin_port = htons(oppPortno);
		if ( connect(oppSockfd,(struct sockaddr *)&opp_addr,sizeof(opp_addr)) < 0){ 
			error("ERROR connecting");
		}
		printf("\nCONNECTED to opponent\n\n");
		//----------------CONNECTION ESTABLISHED----------------------------
		while (1) {
			printf("---Wait till your opponent gives a word to guess---\n");
			char tempWordStr[256];
			bzero(buffer,256);
			n = read(oppSockfd,buffer,256);
			if (n < 0) 
				 error("ERROR reading from socket");
			bcopy(buffer,tempWordStr,256);
			player2(oppSockfd, tempWordStr);
			//-----ONE TURN DONE----------------------------
			char* wordStr;
			wordStr = (char*)malloc(sizeof(char));
			char temp[256],temp2[256];
			wordStr = player1();
			bcopy(wordStr,temp,256);
			bcopy(wordStr,temp2,256);
			printf("\n...waiting for the result...\n\n");
			n = write(oppSockfd,temp,256);
			if (n < 0) 
				 error("ERROR writing to socket");
			player12(oppSockfd, temp2);
		}
	}
	else {
		printf("XXX---something is wrong---XXX\n");
	}
	return 0;
}

char* player1() {
	char word[256];
	char wordStr[256];
	char *actualWordStr = (char*)malloc(sizeof(char));
	printf("---Your turn to give a word to your opponent---\n");
	bzero(wordStr,256);
	printf("Give your word: ");
	scanf("%s",word);
	strcat(wordStr,word);
	strcat(wordStr,"|");
	printf("Your word in string format (for ex:- APPLE -> A_P_E): ");
	scanf("%s",word);
	strcat(wordStr,word);
	actualWordStr = &wordStr[0];
	return actualWordStr;
}

void player2(int oppSockfd, char wordStr[256]) {
	char man[7][15]={{' ',' ',' ',' ','_','_','_','\n'},{' ',' ',' ',' ',' ','|','\n'},{' ',' ',' ',' ',' ','0','\n'},{' ',' ',' ',' ',' ','|','\n'},{' ',' ',' ',' ','/','|','\\','\n'},{' ',' ',' ',' ',' ','|','\n'},{' ',' ',' ','_','/',' ','\\','_','\n'}};
	int i, j, temp, n;
	char actualWord[256],dupWord[256];
	printf("\n\tGo ahead...guess the word........\n");
	for (i=0;i<strlen(wordStr);i++) {
		if (wordStr[i]=='|') {
			actualWord[i] = '\0';
			temp = i+1;
			break;
		}
		actualWord[i] = wordStr[i];
	}
	for (i=temp;i<strlen(wordStr);i++) {
		dupWord[i-temp] = wordStr[i];
	}
	dupWord[strlen(wordStr)-(strlen(actualWord)+1)] = '\0';
	printf("\n\t\t");
	for (i=0;i<strlen(dupWord);i++) {
		printf("%c ",dupWord[i]);
	}
	printf("\n\n");
	
	char letter;
	char buffer[256];
	int count=0;
	while (count<7) {
		if (!strcmp(actualWord,dupWord)) {
			printf("You WON against your opponent!! :)\n\n");
			break;
		}
		printf("your letter: ");
		letter = getc(stdin);
		if (letter == '\n') letter = getc(stdin);
		
		buffer[0]=letter;
		buffer[1]='\0';
		n = write(oppSockfd,buffer,256);
		if (n < 0) 
			 error("ERROR writing to socket");
		
		int ch=0;
		for (i=0;i<strlen(actualWord);i++) {
			if (actualWord[i]==letter && dupWord[i]!=letter) {
				dupWord[i] = actualWord[i];
				ch=1;
			}
		}
		if(ch==0) { count++; }
		printf("\n\t\t");
		for (i=0;i<strlen(dupWord);i++) {
			printf("%c ",dupWord[i]);
		}
		printf("\n\n");
		for (i=0;i<count;i++){
			for (j=0;j<sizeof(man[i]);j++){
				printf("%c",man[i][j]);
			}
		}
		printf("\n\n");
	}
	if (count>=7) { 
		printf("You LOST against your opponent! :(\n\n"); 
	}
}

void player12(int oppSockfd, char wordStr[256]) {
	char man[7][15]={{' ',' ',' ',' ','_','_','_','\n'},{' ',' ',' ',' ',' ','|','\n'},{' ',' ',' ',' ',' ','0','\n'},{' ',' ',' ',' ',' ','|','\n'},{' ',' ',' ',' ','/','|','\\','\n'},{' ',' ',' ',' ',' ','|','\n'},{' ',' ',' ','_','/',' ','\\','_','\n'}};
	int i, j, temp;
	char actualWord[256],dupWord[256];
	printf("\n\tYour opponent is guessing the word........\n");
	for (i=0;i<strlen(wordStr);i++) {
		if (wordStr[i]=='|') {
			actualWord[i] = '\0';
			temp = i+1;
			break;
		}
		actualWord[i] = wordStr[i];
	}
	for (i=temp;i<strlen(wordStr);i++) {
		dupWord[i-temp] = wordStr[i];
	}
	dupWord[strlen(wordStr)-(strlen(actualWord)+1)] = '\0';
	printf("\n\t\t");
	for (i=0;i<strlen(dupWord);i++) {
		printf("%c ",dupWord[i]);
	}
	printf("\n\n");
	
	char letter;
	int count=0, n;
	char buffer[256];
	while (count<7) {
		if (!strcmp(actualWord,dupWord)) {
			printf("Your opponent WON against you! :(\n\n");
			break;
		}
		
		bzero(buffer,256);
		n = read(oppSockfd,buffer,256);
		if (n < 0) 
			 error("ERROR reading from socket");
		letter = buffer[0];
		printf("Letter choosen: %s\n",buffer);
		
		int ch=0;
		for (i=0;i<strlen(actualWord);i++) {
			if (actualWord[i]==letter && dupWord[i]!=letter) {
				dupWord[i] = actualWord[i];
				ch=1;
			}
		}
		if(ch==0) { count++; }
		printf("\n\t\t");
		for (i=0;i<strlen(dupWord);i++) {
			printf("%c ",dupWord[i]);
		}
		printf("\n\n");
		for (i=0;i<count;i++){
			for (j=0;j<sizeof(man[i]);j++){
				printf("%c",man[i][j]);
			}
		}
		printf("\n\n");
	}
	if (count>=7) { 
		printf("Your opponent LOST against you!! :)\n\n"); 
	}
}
