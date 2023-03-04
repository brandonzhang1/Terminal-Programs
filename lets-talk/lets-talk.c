#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "list.h"

#define KEY 100
#define MAX_BUF_SIZE 4200

void freeItem(void* pItem) {
	free(pItem);
}

void wait(int* s) {
	(*s)--;
	while (*s <= 0) {
		sleep(0.5);
	}
}

void signal(int* s) {
	(*s)++;
}

void encryptMsg(char* buf) {
	char* scan = buf;
	while(*scan != '\0') {
		*scan = (*scan + KEY) % 256;
		++scan;
	}
}

void decryptMsg(char* buf) {
	char* scan = buf;
	while(*scan != '\0') {
		*scan = (*scan - KEY) % 256;
		++scan;
	}
}

int sendsockfd;
int recvsockfd;
struct sockaddr_in targetAddr;
struct sockaddr_in localAddr;

List* sentMessages;
List* receivedMessages;
char* sendBuf;
char* inputBuf;
char* recvBuf;
int freeNodesCount = LIST_MAX_NUM_NODES;
int statusCheck = 0;

int* threadRetVal;
int exit = 0;

void* statusTimeout() {
	sleep(1);
	if (statusCheck != 0) {
		printf("Offline\n");
	}
	statusCheck = 0;
}

void* receiveMessages() {
	while (1) {
		recvBuf = calloc(MAX_BUF_SIZE, sizeof(char));
		int len = recvfrom(recvsockfd, recvBuf, MAX_BUF_SIZE, 0, NULL, 0);
		if (len <= 0) {
			free(recvBuf);
		} else {
			//decode
			decryptMsg(recvBuf);
			if (strcmp(recvBuf, "!status\n") == 0) {
				free(recvBuf);
				char status[] = "Online\n";
				encryptMsg(status);
				int err = sendto(sendsockfd, status, 7, 0, (struct sockaddr*)&targetAddr, sizeof(targetAddr));
				if (err <= 0) {
					printf("Nothing was sent\n");
				}
			} else if (strcmp(recvBuf, "!exit\n") == 0) {
				//exit condition
				free(recvBuf);
				exit = 1;
				pthread_exit(threadRetVal);
			} else {
				if (strcmp(recvBuf, "Online\n") == 0) {
					statusCheck = 0;
				}
				//add to list
				List_add(receivedMessages, recvBuf);
			}
		}
	}
}

void* printMessages() {
	char* output;
	while (1) {
		//get next message
		while(List_count(receivedMessages) == 0) {
			if (exit == 1) {
				pthread_exit(threadRetVal);
			}
			sleep(0.5);
		}
		List_first(receivedMessages);
		output = (char*) List_remove(receivedMessages);
		printf("%s", output);
		fflush(stdout);
		free(output);
	}
}

void* sendMessages() {
	char* next;
	while (1) {
		//get next message if there is one
		while(List_count(sentMessages) == 0) {
			sleep(0.5);
		}
		List_first(sentMessages);
		next = (char*)List_remove(sentMessages);
		int len = strlen(next);
		if (len > MAX_BUF_SIZE-1) len = MAX_BUF_SIZE-1;
		strncpy(sendBuf, next, len+1);
		//encrypt
		encryptMsg(sendBuf);
		//send
		int err = sendto(sendsockfd, sendBuf, len, 0, (struct sockaddr*)&targetAddr, sizeof(targetAddr));
		if (err <= 0) {
			printf("Nothing was sent\n");
		}
		if (strcmp(next, "!status\n") == 0) {
			statusCheck = 1;
			pthread_t status;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			while(pthread_create(&status, &attr, statusTimeout, NULL) != 0) {}
			pthread_attr_destroy(&attr);
		}
		if (strcmp(next, "!exit\n") == 0) {
			err = sendto(sendsockfd, sendBuf, len, 0, (struct sockaddr*)&localAddr, sizeof(localAddr));
			if (err <= 0) {
				printf("Nothing was sent\n");
			}
			pthread_exit(threadRetVal);
		}
		free(next);
	}
}

void* inputMessages() {
	while (1) {
		inputBuf = malloc(sizeof(char)*MAX_BUF_SIZE);
		fgets(inputBuf, MAX_BUF_SIZE, stdin);
		List_append(sentMessages, inputBuf);
		if (strcmp(inputBuf, "!exit\n") == 0) {
			pthread_exit(threadRetVal);
		}
	}
}

//args: local port, target IP, target port
int main(int argc, char* argv[]) {
	int localPort = 0;
	int targetPort = 0;

	int localAddrlen = sizeof(localAddr);

	sendsockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendsockfd == -1) {
		printf("Send socket failed to create\n");
		return -1;
	}

	recvsockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (recvsockfd == -1) {
		printf("Receive socket failed to create\n");
		return -1;
	}

	//build target address struct for sending messages
	targetAddr.sin_family = AF_INET;
	if(argc != 4 || inet_pton(AF_INET, argv[2], &targetAddr.sin_addr)<=0) {
		printf("Usage:\n  ./lets-talk <local port> <remote host> <remote port>\nExamples:\n  ./lets-talk 3000 192.168.0.513 3001\n  ./lets-talk 3000 some-computer-name 3001\n");
		return 0;
	}
	sscanf(argv[3], "%i", &targetPort);
	targetAddr.sin_port = htons(targetPort);

	//build local address struct for binding socket
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = INADDR_ANY;
	sscanf(argv[1], "%i", &localPort);
	localAddr.sin_port = htons(localPort);
	int err = bind(recvsockfd, (struct sockaddr *)&localAddr, (socklen_t)localAddrlen);
	if (err == -1) {
		printf("Receive socket failed to bind\n");
		return -1;
	}

	//initialize working data structures
	sentMessages = List_create();
	receivedMessages = List_create();
	sendBuf = calloc(MAX_BUF_SIZE, sizeof(char));

	//start threads
	pthread_t send;
	pthread_t receive;
	pthread_t print;
	pthread_t input;
	err = pthread_create(&receive, NULL, receiveMessages, receivedMessages);
	if (err != 0) {
		printf("Receive thread failed to create\n");
		free(sendBuf);
		return -1;
	}
	err = pthread_create(&print, NULL, printMessages, receivedMessages);
	if (err != 0) {
		printf("Print thread failed to create\n");
		free(sendBuf);
		return -1;
	}
	err = pthread_create(&send, NULL, sendMessages, sentMessages);
	if (err != 0) {
		printf("Send thread failed to create\n");
		free(sendBuf);
		return -1;
	}
	err = pthread_create(&input, NULL, inputMessages, sentMessages);
	if (err != 0) {
		printf("Input thread failed to create\n");
		free(sendBuf);
		return -1;
	}

	//wait for receive thread to exit then proceed with program cleanup
	pthread_join(receive, (void**)&threadRetVal);
	pthread_join(send, (void**)&threadRetVal);
	pthread_join(print, (void**)&threadRetVal);
	pthread_join(input, (void**)&threadRetVal);
	//close sockets
	err = shutdown(sendsockfd, 1);
	if (err == EBADF) {
		printf("Send socket not a valid file descriptor\n");
	} else if (err == ENOTSOCK) {
		printf("Send socket not a socket\n");
	} else if (err == ENOTCONN) {
		printf("Send socket is not connected\n");
	}
	err = shutdown(recvsockfd, 0);
	if (err == EBADF) {
		printf("Receive socket not a valid file descriptor\n");
	} else if (err == ENOTSOCK) {
		printf("Receive socket not a socket\n");
	} else if (err == ENOTCONN) {
		printf("Receive socket is not connected\n");
	}
	//free structs
	free(sendBuf);
	free(inputBuf);
	//free lists and nodes
	List_free(sentMessages, freeItem);
	List_free(receivedMessages, freeItem);

	return 0;
}












