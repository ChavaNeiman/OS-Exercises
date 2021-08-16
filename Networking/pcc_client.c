#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

unsigned long int readFile(FILE* fptr){
    unsigned long int total_read = 0;

    while (getc(fptr) != EOF){
        ++total_read;
    } 
    return total_read;
}

int main(int argc, char *argv[]){
	int  sockfd = -1;
  	char recv_buff[1024], buff;
  	uint16_t server_port;
  	char* server_ip = NULL, *file_name = NULL;
  	unsigned long int bytes_read = 0, bytes_written = 0,file_len, printable_bytes, len;
  	void* _printable_bytes;
	
	if(argc!=4){
		perror("Not enough arguments\n");
        exit(1);
	}
	
	server_ip = argv[1];
    server_port = atoi(argv[2]);
    file_name = argv[3];
    
    FILE* fptr = fopen(file_name, "r");
    if (!fptr){
  		printf("Cannot open file %s\n", file_name);
        exit(1);
    }
    
	struct sockaddr_in my_addr;
	
	memset(recv_buff, 0,sizeof(recv_buff));
  	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	printf("Error : Could not create socket \n");
    	exit(1);
  	}
  	
  	memset(&my_addr, 0, sizeof(my_addr));
  	my_addr.sin_family = AF_INET;
  	my_addr.sin_port = htons(server_port); 
  	my_addr.sin_addr.s_addr = inet_addr(server_ip); 
  	
  	if(connect(sockfd,(struct sockaddr*) &my_addr, sizeof(my_addr)) < 0){
    	printf("Error: Connect Failed. %s \n", strerror(errno));
    	exit(1);
  	}
  	
  	len = readFile(fptr);
  	file_len = htonl(len);
  	
  	if(write(sockfd, &file_len, sizeof(file_len))==-1){
  		printf("Error: Unable to write data. \n");
        exit(1);
  	}
  	
  	rewind(fptr);
  	
    bytes_written = 0;
  	while(bytes_written < len){
  		buff = getc(fptr);
  		if(write(sockfd, &buff, sizeof(buff))==-1){
  			printf("Error: Unable to write data. \n");
        	exit(1);
        }
        else{
        	bytes_written++;
        }
  	}
  	fclose(fptr);
  	
    if((bytes_read = read(sockfd, &_printable_bytes, sizeof(_printable_bytes)))<= 0 ){
     	printf("Error: Unable to read data. \n");
        exit(1);
    }
 
    printable_bytes = ntohl((unsigned long int)_printable_bytes);
  	printf("# of printable characters: %lu\n", printable_bytes);
  	
  	close(sockfd);
  	return 0;
}