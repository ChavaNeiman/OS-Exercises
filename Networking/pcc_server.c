#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#define MIN_READABLE 32 
#define MAX_READABLE 126

unsigned long int pcc_total[MAX_READABLE - MIN_READABLE +1] = {0};
_Bool is_proccessing = 0, sigint=0;

void int_handler(){
	if(is_proccessing){
		sigint = 1;
	}
	else{
		for(int i=0; i<95;i++){
			printf("char ’%c’ : %lu times\n",i+MIN_READABLE,pcc_total[i]);
		}
	exit(0);
	}
}

int main(int argc, char *argv[]){

	unsigned long int server_port = atoi(argv[1]), u_file_size, total_read=0, count=0, file_size;
	int listenfd  = -1, connfd = -1;
	char letter[1];
	
	struct sigaction int_act;
	int_act.sa_handler = int_handler;
	sigaction(SIGINT, &int_act, NULL);
	
	if(argc!=2){
		perror("Not enough arguments\n") ;
        exit(1);
	}
	
	struct sockaddr_in serv_addr;
	struct sockaddr_in peer_addr;
	socklen_t addrsize = sizeof(struct sockaddr_in);
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, addrsize);
    
    serv_addr.sin_family = AF_INET;
  	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	serv_addr.sin_port = htons(server_port);
  	 
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {  
        perror("setsockopt fail");
        exit(1);  
    } 
	
	if( 0 != bind(listenfd,(struct sockaddr*) &serv_addr, addrsize)){
    	printf("Error: Bind Failed. %s \n", strerror(errno));
    	exit(1);
  	}
  	
  	if( 0 != listen(listenfd, 10)){
    	printf("Error: Listen Failed. %s \n", strerror(errno));
    	exit(1);
  	}
  	
  	while(1){
  		count=0;
    	connfd = accept(listenfd,(struct sockaddr*) &peer_addr, &addrsize);
		is_proccessing = 1;
	
    	if(connfd < 0){
    		if (strcmp(strerror(errno), "ETIMEDOUT") || strcmp(strerror(errno), " ECONNRESET") || strcmp(strerror(errno), "EPIPE")){
            		printf("Error : Accept Failed. %s \n", strerror(errno));
            		continue;
			}
			else{
       			printf("Error : Accept Failed. %s \n", strerror(errno));
       			exit(1);
    		}
    	}
    	
    	if (read(connfd, &file_size, sizeof(file_size)) < 0){
        	printf("Error: Unable to read data size. \n");
        	exit(1);
    	}
    	
    	u_file_size = ntohl(file_size);
    	total_read = 0;
    
    	while (total_read < u_file_size){
        	if(read(connfd, letter, sizeof(letter))<=0){
        		perror("Error: Unable to read data size. \n");
        		exit(1);
        	}
        	total_read ++;
        	if(letter[0]>=MIN_READABLE && letter[0]<=MAX_READABLE){
        		pcc_total[letter[0] - MIN_READABLE]++;
        		count++;
        	}
    	}
  	
    	unsigned long int ccount = htonl(count);
    	if(write(connfd, &ccount, sizeof(ccount))==-1){
    		printf("Error: Unable to write data. \n");
        	exit(1);
    	}
    	
    	is_proccessing = 0;
    	if(sigint)
    		raise(SIGINT);
    		
    	close(connfd);
	}

}