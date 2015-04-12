#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BACKLOG 10

/* We must define union semun ourselves. */
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};
/* Initialize a binary semaphore with a value of 1. */
int binary_semaphore_initialize (int semid) {
	union semun argument;
	unsigned short values[1];
	values[0] = 1;
	argument.array = values;
	return semctl (semid, 0, SETALL, argument);
}
int binary_semaphore_wait (int semid) {
	struct sembuf operations[1];
	/* Use the first (and only) semaphore. */
	operations[0].sem_num = 0;
	/* Decrement by 1. */
	operations[0].sem_op = -1;
	/* Permit undo’ing. */
	operations[0].sem_flg = SEM_UNDO;
	return semop (semid, operations, 1);
}
/* Post to a binary semaphore: increment its value by 1.
This returns immediately. */
int binary_semaphore_post (int semid) {
	struct sembuf operations[1];
	/* Use the first (and only) semaphore. */
	operations[0].sem_num = 0;
	/* Increment by 1. */
	operations[0].sem_op = 1;
	/* Permit undo’ing. */
	operations[0].sem_flg = SEM_UNDO;
	return semop (semid, operations, 1);
}
void error(char *msg)
{
	perror(msg);
	exit(1);
}
	//----------------------------------------------------------------------------------
int main(int argc, char* args[]) {
	if (argc!=2) {
		printf("usage: ./s port\n");
		exit(-1);
	}
	
	int sockfd, newfd, sin_size, n;
	struct sockaddr_in my_addr, their_addr;
	
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(atoi(args[1]));
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
	
	bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr);

	int yes=1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1) {
		perror("setsockopt");
		exit(1);
	}
	
	listen(sockfd, BACKLOG);
	
	//----------------------------------------------------------------------------------
	int segment_id;
	char* shared_memory;
	const int shared_segment_size = 0x6400;
	/* Allocating a shared memory segment.  */
	segment_id = shmget (IPC_PRIVATE, shared_segment_size,
		       IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	/* Attaching the shared memory segment.  */
	shared_memory = (char*) shmat (segment_id, 0, 0);
	
	int segment_id_2;
	char* shared_index;
	const int shared_segment_size_2 = 0x6400;
	/* Allocating a shared memory segment.  */
	segment_id_2 = shmget (IPC_PRIVATE, shared_segment_size_2,
		       IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	/* Attaching the shared memory segment.  */
	shared_index = (char*) shmat (segment_id_2, 0, 0);
	sprintf(shared_index,"ODD");
	//----------------------------------------------------------------------------------
	int numsems = 1;
	int semaphore1 = semget(IPC_PRIVATE,numsems,0666 | IPC_CREAT);
	binary_semaphore_initialize(semaphore1);
	int semaphore2 = semget(IPC_PRIVATE,numsems,0666 | IPC_CREAT);
	binary_semaphore_initialize(semaphore2);
	binary_semaphore_wait(semaphore2);
	//----------------------------------------------------------------------------------
	char buffer[256];
	//----------------------------------------------------------------------------------
	while (1) {
		printf("....waiting to connect to a client....\n");
		sin_size = sizeof their_addr;
		newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (newfd < 0) 
			error("ERROR accepting");
		printf(":::Connection Accepted:::\n");
		
		if(fork() == 0) {
			binary_semaphore_wait(semaphore1);
			
			printf("CONNECTED\n");
			close(sockfd);
			
			n = write(newfd,shared_index,256);
			if (n < 0) 
				error("ERROR writing to socket");

			if (!strcmp(shared_index,"EVEN")) {         //EVEN clients
				printf("---Connected to EVEN_client---\n");
				//reading client_1 info stored in the shared memory
				n = write(newfd,shared_memory,256);
				if (n < 0) 
					error("ERROR writing to socket");
				sprintf (shared_memory,"");
				sprintf(shared_index,"ODD");
				printf("/// Done with EVEN_client ///\n\n");
			}
			else if (!strcmp(shared_index,"ODD")){      //ODD clients
				printf("---Connected to ODD_client---\n");
				//writing client_1 info into the shared memory
				bzero(buffer, 256);
				n = read(newfd,buffer,256);
				if (n < 0) 
					error("ERROR reading from socket");
				sprintf (shared_memory,"%s|%s",(char *)inet_ntoa(their_addr.sin_addr),buffer);
				sprintf(shared_index,"EVEN");
				printf("/// Done with ODD_client ///\n\n");
			}
			else {
				printf("XXX---something is wrong---XXX\n");
			}
			binary_semaphore_post(semaphore2);
			exit(-1);
		}
		else {
			binary_semaphore_wait(semaphore2);
			close(newfd);
			binary_semaphore_post(semaphore1);
		}
	}
}
