#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "web-util.h"
#include <semaphore.h>
#include <string.h>

/* 
 *  Description: Shared Buffer & overload flags
 *  Author: Asaad W. Daadouch
 *  Version: 1.0 16/10/2022
 */
struct Shared_Buffer_requsted {
    sem_t list_empty;
    sem_t list_full;
    sem_t mutex;
    int buffer_size;
    int produce;
    int consume;
    int *buffer;
};
// 0 = block(BLCK), 1 = Drop_Tail(DRPT), 2 = Drop_Head(DRPH)
 int overload_type;  // 2: drop head, 1: drop tail, 0: block.
 int THREAD_NUM;

/*
*  Description: workerThreads fucntion serve the data that
*  was taken after that close, after that
*  it will increment the counter in the buffer if its empty
*  if the counter is full it will decremnt it in the buffer
*  args: data
*  return:  null  
*  Author: Asaad W. Daadouch
*  Version: 1.0 16/10/2022
*/
 
static void *workerThreads(void *data) {
	
	struct Shared_Buffer_requsted *ptr = (struct Shared_Buffer_requsted *)data;
    int data_Req, hit;
	
	for(hit=1; ; hit++)
	{    
		sem_wait(&ptr->list_full);         			            // Decrement the number of full spaces
        sem_wait(&ptr->mutex);        			                // Lock the consumer value so only this operation can change it
        data_Req = ptr->buffer[ptr->consume];                   // Assign it to variable requested_Data
        ptr->consume = (ptr->consume + 1) % ptr->buffer_size;   // Update the consumer pointer
        sem_post(&ptr->mutex);        		                    // Unlock the consumer 
		pthread_detach(pthread_self());                         //Detach self.
		usleep(1);                                              //1 microsecond sleep for monitoring the multithreading result
		serve(data_Req, hit);     		                        // serve the request
		close(data_Req);  				                        // close socket
        sem_post(&ptr->list_empty);  			                // Increment the number of empty spaces
    }	
    return NULL;
}
 
 /*
 *  Description: Mainthread
 *  here in this function will use the overload methods
 *  if the buffer if full will iverright the old one byy usuing one of the
 *  hanlding methods. 
 *  Author: Asaad W. Daadouch
 *  Version: 1.0 16/10/2022
 *  return: NULL
 */
// 0 = block(BLCK), 1 = Drop_Tail(DRPT), 2 = Drop_Head(DRPH)
void MainThread(int socket_Requestes, struct Shared_Buffer_requsted *ptr) 
{
    int i;
    sem_getvalue(&ptr->list_full,&i);       				             
    if(i == ptr->buffer_size){                						 
        if (overload_type == 2) 				 				         
		{
			sem_wait(&ptr->mutex);         			    		 
			close(ptr->buffer[ptr->consume]);  			   			 
			ptr->buffer[ptr->consume] = socket_Requestes;         
			ptr->produce = (ptr->consume + 1) % ptr->buffer_size;      		 
			ptr->consume= (ptr->consume + 1) % ptr->buffer_size;      			 
			sem_post(&ptr->mutex);         						 
		}
		else if(overload_type == 1){ 								 
			sem_wait(&ptr->mutex);         						 
			ptr->produce = (ptr->consume + ptr->buffer_size - 1) % ptr->buffer_size; 
			close(ptr->buffer[ptr->produce]);
			ptr->buffer[ptr->produce] = socket_Requestes;       	 
			sem_post(&ptr->mutex);         						 
		}
		else if(overload_type == 0){									 
			sem_wait(&ptr->mutex);         			 			 
		}
    
	} else {													
        sem_wait(&ptr->list_empty);         							
        sem_wait(&ptr->mutex);        							
        ptr->buffer[ptr->produce] = socket_Requestes; 			
        ptr->produce = (ptr->produce + 1) % ptr->buffer_size; 					
        sem_post(&ptr->mutex);         							
        sem_post(&ptr->list_full);         							
    }
    return;
}

/* =======================================================================
 * main
 * Check the aguments, create sockets, set overload flag, initalize queue 
 * and buffer, create worker threads.
 * For each hit, a new socket fd is added to the buffer.
 * arguments:   int argc, char argv. 
 * =======================================================================
 */
int main(int argc, char **argv) 
{
    int i, port, listenfd, socketfd, hit, buffer_size_arg,counter;
    socklen_t length;
    static struct sockaddr_in cli_addr;  // static = initialised to zeros
    static struct sockaddr_in serv_addr; // static = initialised to zeros
    struct Shared_Buffer_requsted *ptr;
	struct sigaction action;
	
    /* =======================================================================
     * Check for command-line errors, and print a help message if necessary.
     * =======================================================================
     */
    if( argc < 5  || argc > 6 || !strcmp(argv[1], "-?") ) {
     (void)printf("hint: web-server Port-Number Top-Directory\t\tversion %d\n\n"
    "\tweb-server is a small and very safe mini web server\n"
    "\tweb-server only servers out file/web pages with extensions named below\n"
    "\tand only from the named directory or its sub-directories.\n"
    "\tThere is no fancy features = safe and secure.\n\n"
    "\tExample: web-server 8181 /home/webdir 64&\n\n"
    "\tOnly Supports:", VERSION);

        
        for(i=0;extensions[i].ext != 0;i++)
            (void)printf(" %s",extensions[i].ext);
 
     (void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
    "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n");
    
    }

    if (argc == 5){
            overload_type = BLCK;
            printf("\nOverload method (BLCK = 0, DRPT = 1, DRPH = 2) : %i\n", overload_type);
            printf("Number of Buffers: %s\n", argv[4]);
            printf("Number of Threads : %s\n", argv[3]);
            char s[100];
            printf("Current Directory: %s\n",  getcwd(s, 100));
            printf("Port Number : %s\n", argv[1]); 
        }

    if (argc == 4){
        buffer_size_arg = 10;
        overload_type = BLCK;
        printf("\nOverload method (BLCK = 0, DRPT = 1, DRPH = 2) : %i\n", overload_type);
        printf("Number of Buffers: %d\n", buffer_size_arg);
        printf("Number of Threads : %s\n", argv[3]);
        char s[100];
        printf("Current Directory: %s\n",  getcwd(s, 100));
        printf("Port Number : %s\n", argv[1]); 

    }

    if (argc == 3){
        long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
        THREAD_NUM = number_of_processors;
        buffer_size_arg = 10;
        overload_type = BLCK;
        printf("\nOverload method (BLCK = 0, DRPT = 1, DRPH = 2) : %i\n", overload_type);
        printf("Number of Buffers: %d\n", buffer_size_arg);
        printf("Number of Threads : %d\n", THREAD_NUM);
        char s[100];
        printf("Current Directory: %s\n",  getcwd(s, 100));
        printf("Port Number : %s\n", argv[1]); 


    }

    if (argc == 2){
        char s[100];
        argv[2] = getcwd(s, 100);
        long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
        THREAD_NUM = number_of_processors;
        buffer_size_arg = 10;
        overload_type = BLCK;
        printf("\nOverload method (BLCK = 0, DRPT = 1, DRPH = 2) : %i\n", overload_type);
        printf("Number of Buffers: %d\n", buffer_size_arg);
        printf("Number of Threads : %d\n", THREAD_NUM);
        printf("Current Directory: %s\n",  getcwd(s, 100));
        printf("Port Number : %s\n", argv[1]); 

    }
    
    if (argc == 1){
        port = 8085;
        char s[100];
        argv[2] = getcwd(s, 100);
        long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
        THREAD_NUM = number_of_processors;
        buffer_size_arg = 10;
        overload_type = BLCK;
        printf("\nOverload method (BLCK = 0, DRPT = 1, DRPH = 2) : %i\n", overload_type);
        printf("Number of Buffers: %d\n", buffer_size_arg);
        printf("Number of Threads : %d\n", THREAD_NUM);
        printf("Current Directory: %s\n",  getcwd(s, 100));
        printf("Port Number : %d\n", port); 
        

    }
        // printf("\n\n\n\n\n\nOverload method (BLCK = 0, DRPT = 1, DRPH = 2) : %i\n", overload_type);
        // printf("Number of Buffers: %d\n", buffer_size_arg);
        // printf("Number of Threads : %d\n", THREAD_NUM);
        // printf("Current Directory: %s\n",  argv[2]);
        // printf("Port Number : %d\n", port); 


    /* =======================================================================
     * Check the legality of the specified top directory of the web site.
     * =======================================================================
     */
    if( !strncmp(argv[2],"/"   ,2 ) || !strncmp(argv[2],"/etc", 5 ) ||
        !strncmp(argv[2],"/bin",5 ) || !strncmp(argv[2],"/lib", 5 ) ||
        !strncmp(argv[2],"/tmp",5 ) || !strncmp(argv[2],"/usr", 5 ) ||
        !strncmp(argv[2],"/dev",5 ) || !strncmp(argv[2],"/sbin",6) ){
        (void)printf("ERROR: Bad top directory %s, see web-server -?\n",argv[2]);
        exit(3);
    }
 
    /* =======================================================================
     * See if we can change the current directory to the one specified.
     * =======================================================================
     */
    if(chdir(argv[2]) == -1){ 
        (void)printf("ERROR: Can't Change to directory %s\n",argv[2]);
        exit(4);
    }
 
    /* =======================================================================
     * Detach the process from the controlling terminal, after all input args
     * checked out successfuly; then create a child process to continue as a
     * daemon, whose job is to create a communication socket to listen for
     * http requests.
     * =======================================================================
     * 
	 * Become deamon + unstopable and no zombies children (= no wait())
	 * =======================================================================
     */
	if(fork() != 0)  return 0;         // parent terminates & returns OK to shell
    (void)signal(SIGCLD, SIG_IGN);     // ignore child death
    (void)signal(SIGHUP, SIG_IGN);     // ignore terminal hangups
 
    for(i=0;i<32;i++) (void)close(i);  // close open files
    (void)setpgrp();                   // break away from process group
    if (argc > 2){
        logs(MSG6,atoi(argv[1]),"web-server starting at port","pid",getpid());
    }
    logs(MSG6,port,"web-server starting at port","pid",getpid());
    
 
    /* =======================================================================
     * Setup the network socket
     * =======================================================================
     */
    if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0) {
        logs(ERROR,1,"system call","socket",0);
        exit(3);
    }

    if (argc > 1){
        port = atoi(argv[1]);

        if(port < 0 || port >60000) {
            logs(ERROR,2,"Invalid port number (try 1->60000)",argv[1],0);
            exit(3);
        }
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
 
    if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
        logs(ERROR,3,"system call","bind",0);
        exit(3);
    }
 
    if(listen(listenfd,64) <0) {
        
        exit(3);
    }

    if ( argc > 5){

        if (!strcmp(BLCK_STR, argv[5])){
            overload_type = BLCK;
            logs(MSG6,4,"Overload_type","Value=",overload_type);
        } else if (!strcmp(DRPT_STR, argv[5])){
            overload_type = DRPT;
            logs(MSG6,4,"Overload_type","Value=",overload_type);
        } else if (!strcmp(DRPH_STR, argv[5])){
            overload_type = DRPH;
            logs(MSG6,4,"Overload_type","Value=",overload_type);
        } else {
            overload_type = BLCK;
            logs(MSG6,4,"Overload_type","Value=",overload_type);
        }
    }    
    logs(MSG6,4,"Overload_type","Value=",overload_type);
    logs(MSG6,4,"Buffer_size","Value=",buffer_size_arg);


    if (argc > 4){
        buffer_size_arg = atoi(argv[4]);
    }
    if (argc > 3){
        THREAD_NUM = atoi(argv[3]);
    }
	pthread_t thread_id[THREAD_NUM];                                                       // create IDs array with the desired thread numbers
	
	ptr = (struct Shared_Buffer_requsted *)malloc(sizeof(struct Shared_Buffer_requsted));   //allocate memory space for queue struct 
     if(ptr == NULL){
        perror("malloc()");
        (void)printf("ERROR: Can't create q.\n");                                           // Print error message if errors occurred
        exit(EXIT_FAILURE);                                                                 // Exit if any errors happend
    }
    
    ptr->buffer = (int *)malloc(buffer_size_arg * sizeof(int));                              //allocate memory memory space for queue Buffer
    if(ptr->buffer == NULL){
        free(ptr);                                                                           // free memory space if error occurred
        (void)printf("ERROR: Can't initalize q size.\n");                                    // Print error message if errors occurred
        exit(EXIT_FAILURE);                                                                 // Exit if any errors happend
    }
     
    ptr->buffer_size = buffer_size_arg;                                                     // Set buffer size after safely create the queue
     
    // initialize the empty semaphore to the desired buffer size
    // All buffers are empty in the beginning
    if(sem_init(&ptr->list_empty, 0, buffer_size_arg) != 0) {
        (void)printf("ERROR: Can't initalize empty semaphore.\n");                          // Print error message if errors occurred
        exit(EXIT_FAILURE);                                                                 // Exit if any errors happend
    }
 
    // initialize the full semaphore to the desired buffer size
    // No buffers is full in the beginning
    if(sem_init(&ptr->list_full, 0, 0) != 0) {
        (void)printf("ERROR: Can't initalize full semaphore.\n");                           // Print error message if errors occurred  
        exit(EXIT_FAILURE);                                                                 // Exit if any errors happend
    } 
 
    // initialize the mutex binary semaphore to 1
    if(sem_init(&ptr->mutex, 0, 1) != 0) {
        (void)printf("ERROR: Can't initalize mutex semaphore.\n");                          // Print error message if errors occurred
        exit(EXIT_FAILURE);                                                                 // Exit if any errors happend
    }
 
    ptr->produce = 0; // initalize producer pointer to zero
    ptr->consume = 0; // initalize consumer pointer to zero
    
    
		for (counter=0; counter < THREAD_NUM; counter++){
			pthread_create(&thread_id[counter],NULL,workerThreads,ptr);
		} 
  
    /* =======================================================================
     * Loop forever, listening to connection attempts and accept them.
     * For each accepted connection, add it to buffer and return. 
     * =======================================================================
     */
      for(hit=1; ;hit++) {
        length = sizeof(cli_addr);
        if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0){
            logs(ERROR,5,"system call","accept",0);
        }else {
        MainThread(socketfd, ptr);
        }
    }
	/*========================================================================
	 * SIGTERM signal handling function
	 *========================================================================
	 */
		memset(&action, 0, sizeof(struct sigaction));
		//action.sa_handler = term; 
		sigaction(SIGTERM, &action, NULL);
	
}


 