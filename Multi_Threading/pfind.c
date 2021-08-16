#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <unistd.h>
#include <signal.h>
#define Max_threads 100

typedef struct Node
{
	char* dir;
	struct Node* next;
}Node;

typedef struct Fifo_queue
{
	Node* head;
	Node* tail;
}Fifo_queue;

Fifo_queue queue = {NULL,NULL};
char* search_term;
size_t found_files = 0;
size_t num_of_threads;
size_t active_threads = 0;
size_t error_threads = 0;
pthread_t threads[Max_threads];
pthread_mutex_t lock;
pthread_cond_t not_empty;

Node* create_node(char* buff) 
{ 
    Node* node = malloc(sizeof(Node)); 
    node->dir = malloc(strlen(buff)); 
    strcpy(node->dir,buff);
    node->next = NULL; 
    return node; 
} 

_Bool queue_is_empty(){
	return !queue.head;
}

void enqueue(Node* node) 
{   
	pthread_mutex_lock(&lock);

    if (queue_is_empty()) { 
        queue.head = node;
        queue.tail = queue.head; 
    }
    else {
    	queue.tail->next=node;
    	queue.tail = queue.tail->next;
    }
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);
}

char* dequeue() 
{  
    pthread_mutex_lock(&lock);
    while (queue_is_empty())
    {
        pthread_cond_wait(&not_empty, &lock);
        pthread_testcancel();
    }
    pthread_testcancel();
	active_threads++;
    Node *temp1 = queue.head;
    queue.head = queue.head -> next;
    pthread_mutex_unlock(&lock);
    char *temp2 = temp1 -> dir;
    free(temp1);
    return temp2; 
} 

void free_queue()
{
	while(queue.head){
		Node *temp = queue.head->next;
		free(queue.head->dir);
		free(queue.head);
		queue.head=temp;
	}
}

void treat_file(char * path){
	if(!fnmatch(search_term,strrchr(path,'/')+1,0)){
		printf("%s\n",path);
		__sync_fetch_and_add(&found_files, 1);
	}
}

void treat_dir(char* directory)
{
	DIR *dir = opendir(directory);
	if (!dir){ 
		perror(directory);
		__sync_fetch_and_add(&error_threads, 1);
		if(error_threads == num_of_threads){
			printf("Done searching, found %ld files\n",found_files);
            pthread_mutex_destroy(&lock);
            free(search_term);
            free_queue();
            exit(1);
		}
		pthread_exit(NULL);
	}
	
	struct dirent *entry;
	struct stat dir_stat;
	while((entry = readdir(dir)))
	{
		char buff[strlen(directory)+strlen(entry->d_name)+2];
        sprintf(buff,"%s/%s",directory,entry->d_name);
		stat(buff, &dir_stat);
		
		if(strcmp(entry->d_name,"..") && strcmp(entry->d_name, "."))
		{
			if((dir_stat.st_mode & __S_IFMT) == __S_IFDIR)
				enqueue(create_node(buff));
				
			else
				treat_file(buff);
		 }	
	 }
	if(directory)
		free(directory);
	closedir(dir);
}

void ctrl_mem(void* _lock){
	pthread_mutex_t *p_mutex = (pthread_mutex_t*)_lock;
	pthread_mutex_unlock(p_mutex);
	return;
}

void* browse(void* num_of_thread){
	pthread_cleanup_push(ctrl_mem, &lock);
	while(!queue_is_empty()){
		treat_dir(dequeue());
		__sync_fetch_and_sub(&active_threads, 1);
		if (queue_is_empty() && !active_threads)
        {
            raise(SIGUSR1);
        }
	}
	pthread_cleanup_pop(1);
	return NULL;
}

void init_mutex()
{
    int rc = pthread_mutex_init(&lock, NULL);
    if( rc )
    {
        printf("ERROR in pthread_mutex_init(): %s\n", strerror(rc));
        exit(1);
    }
    pthread_cond_init(&not_empty,NULL);
}

void init_threads()
{
    for (size_t i = 0; i < num_of_threads; ++i)
    {
        int rc = pthread_create(&threads[i], NULL, browse, (void*)i);
        if (rc)
        {
            printf("ERROR in pthread_create(): %s\n", strerror(rc));
            exit(1);
        }
    }
}

void cancel_threads(){
	pthread_t tid = pthread_self();
	for(size_t i=0; i<num_of_threads; i++){
		if(tid!=threads[i]){
			pthread_cancel(threads[i]);
		}
	}
	free_queue();
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&not_empty);
	free(search_term);
}

void user_handler(){
	printf("Done searching, found %ld files\n",found_files);
	cancel_threads();
	pthread_exit(NULL);
}

void int_handler(){
	printf("Search stopped, found %ld files\n",found_files);
	cancel_threads();
	pthread_exit(NULL);
}

int main(int argc, char** argv)
{
	if(argc!=4){
		perror("Not enough arguments!\n");
		exit(1);
	}
	struct sigaction user_act;
	memset(&user_act, 0, sizeof(user_act));
	user_act.sa_handler=user_handler;
	sigaction(SIGUSR1, &user_act, NULL);
	
	struct sigaction int_act;
	memset(&int_act, 0, sizeof(int_act));
	int_act.sa_handler = int_handler;
	sigaction(SIGINT, &int_act, NULL);
	
	enqueue(create_node(argv[1]));
	search_term=(char*)malloc(strlen(argv[2])+3);
	sprintf(search_term,"%c%s%c",'*',argv[2],'*');
	num_of_threads = atoi(argv[3]);
	init_mutex();
	init_threads();
	
	for (size_t i = 0; i < num_of_threads; ++i)
    {
        pthread_join(threads[i], NULL);
    }
	return 0;
}
