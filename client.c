#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>          
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <dirent.h>

void error(const char *msg){
    perror(msg);
    exit(0);
}

void createFile(int sockfd){	//Creating file on client-site
  	printf("Creating a file...");
	
	//Setting directory
	char content[256];
	char path[256] = "/home/vemont93/Desktop/TCPIP/";
	char file[256] = "/client/";
	printf("\nPath: %s", path);
	
	//Create directory if it does not exist	
	struct stat st = {0};
	if(stat(path, &st) == -1){
	  mkdir(path, 0700);
	}
	
	//Create file name
	char filename[256];
	printf("\nPlease enter file name: ");
	fgets(filename, 256, stdin);
	
	if(filename != NULL){
		strcat(path, filename);
		printf("File location: %s", path);
		//Create file
		FILE *fp;
		fp = fopen(path, "w+");
		if(fp == NULL){
		  printf("\nERROR: File create failed");
		  perror("fopen");	
		  exit(0);	
		}
		else{	//Client insert content
		  printf("Please insert content for the file: ");
		  fgets(content, 256, stdin);
		  printf("Content: %s", content);
		 
		  fprintf(fp, "%s", content);	//Write content into the file
		  fclose(fp);
		  printf("\nFile created successfully!");
		}
	}
	else{
		printf("\nERROR: Filename cannot be NULL");		
		printf("\nERROR: Please try again later");
		exit(0);
	}
}

void downloadFile(int sockfd){	//Client download file from Server
	printf("Downloading file from Server... ");
	
	int n;
	int buflen;

	//Setting directory
	char revBuff[256];
	char path[256] = "/home/vemont93/Desktop/TCPIP/";
	char file[256] = "/client/";
	
	
	//Create directory if it does not exist
	struct stat st = {0};
	if(stat(path, &st) == -1){
	  mkdir(path, 0700);
	}

	//Getting available file from Server
	char tempo[256];
	bzero(tempo,256);
	n = read(sockfd, (char*)&buflen, sizeof(buflen));
	if (n < 0) error("ERROR reading from socket");
	buflen = htonl(buflen);
	n = read(sockfd,tempo,buflen);
	if (n < 0) error("ERROR reading from socket");
	printf("\nAvailable file: \n");
	printf("%s", tempo);

	printf("Please enter the file name to download the file: ");
	char selectFile[256];
	bzero(selectFile,256);
	fgets(selectFile,255,stdin);
    char input[256];
	
	//Sending file name that Client wants to download to Server
	int datalen = strlen(selectFile);
	int tmp = htonl(datalen);
	n = write(sockfd, (char*)&tmp, sizeof(tmp));
	if(n < 0) error("ERROR writing to socket");
	n = write(sockfd,selectFile,datalen);
	if (n < 0) error("ERROR writing to socket");
	
	char filename[256];
	printf("Save file name as: ");
	fgets(filename, 256, stdin);

	if(filename != NULL){
		strcat(path, filename);
		printf("File location: %s", path);

		FILE *fr = fopen(path, "ab");
		if(fr == NULL){
		  printf("File cannot be opened");
		  perror("fopen");
		  exit(0);
		}
		else{	//Receiving file from Server 
		  bzero(revBuff, 256);
		  int fr_block_sz = 0;
		  while((fr_block_sz = recv(sockfd, revBuff, 256, 0)) > 0){
		  	int write_sz = fwrite(revBuff, sizeof(char), fr_block_sz, fr);
			if(write_sz < fr_block_sz){
			  error("File write failed on server.\n");
			}
			bzero(revBuff, 256);
			if(fr_block_sz == 0 || fr_block_sz != 256){
			  break;			
			}
		  }
		  printf("\nFile downloaded successfully");
		  fclose(fr);
		}
	}
	else{
		printf("\nERROR: Filename cannot be NULL");		
		printf("\nERROR: Please try again later");
		exit(0);
	}
}

void sendFile(int sockfd){	//Client send file to Server
    
    printf("Sending a file to server...");
    char buff[256];
    int n;
    
    //Setting directory
    char dir[256] = "/home/vemont93/Desktop/TCPIP/";
    char hostname[256];
    gethostname(hostname, 255);
    char file[256] = "Client/";
    
    
    //Create directory if it does not exist
    struct stat st = {0};
    if(stat(dir, &st) == -1){
        mkdir(dir, 0700);
    }
    
    //Printing files that is available from the directory
    printf("\nAvailable file: \n");
    DIR *directory;
    struct dirent *ent;
    if((directory = opendir(dir)) != NULL){
        while((ent = readdir(directory)) != NULL){
            printf("%s", ent->d_name);
        }
        closedir(directory);
    }
    else{
        perror("ERROR");
        exit(0);
    }
    
    //Selecting file to be sent to Server
    char tempo[256];
    printf("\nPlease enter the file name that you wish to send out: ");
    fgets(tempo, 256, stdin);
    char filename[256];
    strcpy(filename, tempo);
    
    if(filename != NULL){
        
        //Sending the file name to Server
        int datalen = strlen(tempo);
        int tmp = htonl(datalen);
        n = write(sockfd, (char*)&tmp, sizeof(tmp));
        if(n < 0) error("ERROR writing to socket");
        n = write(sockfd,tempo,datalen);
        if (n < 0) error("ERROR writing to socket");
        
        char split[2] = "\n";
        strtok(tempo, split);
        
        strcat(dir, filename);
        printf("Sending %s to Server... ", tempo);
        printf("\nDir: %s", dir);
        
        FILE *fs = fopen(dir, "rb");	//Read file
        if(fs == NULL){
            printf("\nERROR: File not found.\n");
            perror("fopen");
            exit(0);
        }
        else{	//Sending file to Server
            bzero(buff, 256);
            int fs_block_sz;
            while((fs_block_sz = fread(buff, sizeof(char), 256, fs)) > 0){
                if(send(sockfd, buff, fs_block_sz, 0) < 0){
                    fprintf(stderr, "ERROR: Failed to send file. %d", errno);
                    break;
                }
                bzero(buff, 256);
            }
            printf("\nFile sent successful !\n");
            fclose(fs);
        }
    }
    else{
        printf("\nERROR: Filename cannot be NULL");		
        printf("\nERROR: Please try again later");
        exit(0);
    }
    
}

void deleteFile(int sockfd){	//Deleting file on client-site
	printf("Deleting a file...");
	
	//Setting directory
	char content[256];
	char path[256] = "/home/vemont93/Desktop/TCPIP/";
	char file[256] = "/client/";
	printf("\nPath: %s", path);
	
	//Create directory if it does not exist	
	struct stat st = {0};
	if(stat(path, &st) == -1){
	  mkdir(path, 0700);
	}

	//Printing files that is available from the directory
	printf("\nAvailable file: \n");
	DIR *directory;
	struct dirent *ent;
	if((directory = opendir(path)) != NULL){
	  while((ent = readdir(directory)) != NULL){
		printf("%s", ent->d_name);
	  }
	  closedir(directory);
	}
	else{
	  perror("ERROR");
	  exit(0);
	}

	//Getting file name to be deleted
	char filename[256];
	printf("\nPlease enter the file name that you want to delete: ");
	fgets(filename, 256, stdin);

	
	if(filename != NULL){

		strcat(path, filename);
		FILE *fp;
		
		//Check if file available
		fp = fopen(path, "r");
		if(fp == NULL){
		  printf("\nERROR: File cannot be created");
		  perror("fopen");	
		  exit(0);	
		}
		else{	//Deleting file
		  int status = remove(path);
		  if(status == 0){
			printf("\nFile deleted successfully!");
			fclose(fp);
		  }else{
			printf("\nERROR: unable to delete the file");
			exit(0);
		  }
		}
	}
}

int main(int argc, char *argv[])	//Connecting to Server (SOCKET)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;


    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    printf("\n\nYou are now connected to the Server!");

    int count = 0;
    while(count == 0){	//Getting Client's choice
	int proceed = 0;
	printf("\n\n1.Create 2.Download 3.Send 4.Delete 5.Exit : ");
	printf("\nPlease insert your choice: ");
	bzero(buffer,256);
	fgets(buffer,255,stdin);
    	char input[256];
	strcpy(input, buffer);
	
	//Sending Client's choice to Server
	int datalen = strlen(buffer);
	int tmp = htonl(datalen);
	n = write(sockfd, (char*)&tmp, sizeof(tmp));
	if(n < 0) error("ERROR writing to socket");
	n = write(sockfd,buffer,datalen);
	if (n < 0) error("ERROR writing to socket");
	

	if((strcmp(input, "1\n")) == 0){	//Create file on client-site
	   createFile(sockfd);
	   count = 0;
	}
	else if((strcmp(input, "2\n")) == 0){	//Client download file from Server
	   downloadFile(sockfd);
	   count = 0;
	}
	else if((strcmp(input, "3\n")) == 0){	//Client send file to Server
	   sendFile(sockfd);
	   count = 0;
	}
	else if((strcmp(input, "4\n")) == 0){	//Delete file on client-site
	   deleteFile(sockfd);
	   count = 0;
	}
	else if((strcmp(input, "5\n")) == 0){	//Client disconnect from Server
	   count = 1;
	   proceed = 1;
	}
	else{
	   printf("\nWrong input, please try again.");	//Invalid input from Client
	   count = 0;
	}
    }
	
    close(sockfd);
    printf("\nYou have disconnected from the Server.\n\n");
    return 0;
}


