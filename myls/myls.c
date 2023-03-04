#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

struct node {
	void* item;
	struct node* next;
	struct node* prev;
};

struct list {
	struct node* head;
	struct node* tail;
};

struct printInfo {
	struct stat* fileStats;
	char* path;
	char* name;
};

void add(void* newItem, struct list* targetList) {
	struct node* newNode = malloc(sizeof(struct node));
	newNode->item = newItem;
	newNode->next = NULL;
	if (targetList->head == NULL) {
		targetList->head = newNode;
		targetList->tail = newNode;
		newNode->prev = NULL;
	} else {
		targetList->tail->next = newNode;
		newNode->prev = targetList->tail;
		targetList->tail = newNode;
	}
}

void freeList(struct list* targetList) {
	struct node* scan = targetList->head;
	struct node* next = scan;
	while (scan != NULL) {
		next = scan->next;
		free(scan->item);
		free(scan);
		scan = next;
	}
	free(targetList);
}

void freePrintList(struct list* targetList) {
	struct node* scan = targetList->head;
	struct node* next = scan;
	while (scan != NULL) {
		next = scan->next;
		free(((struct printInfo*)(scan->item))->fileStats);
		free(((struct printInfo*)(scan->item))->path);
		free(scan->item);
		free(scan);
		scan = next;
	}
	free(targetList);
}

int inodeSpace = 0;
int hardlinksSpace = 0;
int ownerNameSpace = 0;
int ownerGroupSpace = 0;
int fileSizeSpace = 0;

void resetSpaces() {
	inodeSpace = 0;
	hardlinksSpace = 0;
	ownerNameSpace = 0;
	ownerGroupSpace = 0;
	fileSizeSpace = 0;
}

void updateSpaces(struct stat* fileStats) {
	int count = 0;
	unsigned long temp = fileStats->st_ino;
	while (temp != 0) {
		++count;
		temp /= 10;
	}
	inodeSpace = count > inodeSpace ? count : inodeSpace;

	count = 0;
	temp = fileStats->st_nlink;
	while (temp != 0) {
		++count;
		temp /= 10;
	}
	hardlinksSpace = count > hardlinksSpace ? count : hardlinksSpace;

	count = 0;
	/*
	struct passwd* pw = getpwuid(fileStats->st_uid);
	char* str = pw->pw_name;
	*/
	char* str = getpwuid(fileStats->st_uid)->pw_name;
	while (*str != '\0') {
		++str;
		++count;
	}
	ownerNameSpace = count > ownerNameSpace ? count : ownerNameSpace;

	count = 0;
	/*
	struct group* grp = getgrgid(fileStats->st_gid);
	str = grp->gr_name;
	*/
	str = getgrgid(fileStats->st_gid)->gr_name;
	while (*str != '\0') {
		++str;
		++count;
	}
	ownerGroupSpace = count > ownerGroupSpace ? count : ownerGroupSpace;

	count = 0;
	temp = fileStats->st_size;
	while (temp != 0) {
		++count;
		temp /= 10;
	}
	fileSizeSpace = count > fileSizeSpace ? count : fileSizeSpace;
}

int iFlag = 0;
int lFlag = 0;
int RFlag = 0;

int processTokens(int argc, char* argv[], struct list* argList) {
	int count = 0;
	for (int i = 1; i < argc; i++) {
		char* arg = argv[i];
		if (*arg == '-') { //if - : option check
			char* scan = arg+1;
			while (*scan != '\0') {
				if (*scan == 'i') {
					iFlag = 1;
				} else if (*scan == 'l') {
					lFlag = 1;
				} else if (*scan == 'R') {
					RFlag = 1;
				} else {
					return -1;
				}
				scan++;
			}
		} else {
			add(arg, argList);
			++count;
		}
	}
	return count;
}

void backCopy(char* dest, char* src) {
	char* copyDest = dest-1;
	char* copySrc = src;
	while (*(copySrc+1) != '\0') {
		++copySrc;
	}
	while (copySrc >= src) {
		*copyDest = *copySrc;
		--copyDest;
		--copySrc;
	}
	*dest = ' ';
}

int printPrintList(struct list* printList) {
	int count = 0;
	char* output = malloc(sizeof(char)*400);
	struct node* scan = printList->head;
	char* temp = malloc(sizeof(char)*200);
	while (scan != NULL) {
		//printf("debug1\n");
		struct printInfo* info = (struct printInfo*)scan->item;
		struct stat* fileStats = info->fileStats;
		char* pos = output;
		for (int i = 0; i < 64; i++) {
			*(pos+i) = ' ';
		}
		//printf("debug1.1\n");
		if (iFlag == 1) {
			//inode number
			//printf("debug1.2 %p\n", fileStats);
			sprintf(temp, "%lu", fileStats->st_ino);
			//printf("debug1.3\n");
			pos += inodeSpace;
			backCopy(pos, temp);
			//printf("debug1.4\n");
			++pos;
		}
		//printf("debug2\n");
		if (lFlag == 1) {
			//permissions
			sprintf(pos++, (S_ISDIR(fileStats->st_mode)) ? "d" : "-");
			sprintf(pos++, (fileStats->st_mode & S_IRUSR) ? "r" : "-");
			sprintf(pos++, (fileStats->st_mode & S_IWUSR) ? "w" : "-");
		    sprintf(pos++, (fileStats->st_mode & S_IXUSR) ? "x" : "-");
		    sprintf(pos++, (fileStats->st_mode & S_IRGRP) ? "r" : "-");
		    sprintf(pos++, (fileStats->st_mode & S_IWGRP) ? "w" : "-");
		    sprintf(pos++, (fileStats->st_mode & S_IXGRP) ? "x" : "-");
		    sprintf(pos++, (fileStats->st_mode & S_IROTH) ? "r" : "-");
		    sprintf(pos++, (fileStats->st_mode & S_IWOTH) ? "w" : "-");
		    sprintf(pos++, (fileStats->st_mode & S_IXOTH) ? "x" : "-");
		    sprintf(pos++, " ");

		    //hardlinks
		    sprintf(temp, "%lu", fileStats->st_nlink);
			pos += hardlinksSpace;
			backCopy(pos, temp);
			++pos;

			//ownerID
			sprintf(temp, "%s", getpwuid(fileStats->st_uid)->pw_name);
			pos += ownerNameSpace;
			backCopy(pos, temp);
			++pos;

			//ownerGroup
			sprintf(temp, "%s", getgrgid(fileStats->st_gid)->gr_name);
			pos += ownerGroupSpace;
			backCopy(pos, temp);
			++pos;

			//fileSize
			//printf("file size: %lu, fileSizeSpace: %i\n", fileStats->st_size, fileSizeSpace);
			sprintf(temp, "%lu", fileStats->st_size);
			//printf("%s\n", temp);
			pos += fileSizeSpace;
			backCopy(pos, temp);
			
			++pos;

			//time
			asctime_r(localtime(&fileStats->st_mtim.tv_sec), temp);
			strncpy(pos, temp+4, 12);
			pos += 12;
			*pos++ = ' ';
		}
		//printf("debug3\n");
		if (S_ISLNK(fileStats->st_mode)) { //symlink
			sprintf(pos, "\033[96m%s\033[97m", info->name);
			if (lFlag == 1) {
				while (*pos != '\0') {
					++pos;
				}
				*pos++ = ' ';
				*pos++ = '-';
				*pos++ = '>';
				*pos++ = ' ';
				sprintf(temp, "%s/%s", info->path, info->name);
				char* temp2 = calloc(200, sizeof(char));
				readlink((char*)temp, temp2, 200);
				sprintf(pos, "\033[92m%s", temp2);
				free(temp2);
			}
		} else if (S_ISDIR(fileStats->st_mode)) {
			sprintf(pos, "\033[42m\033[94m%s", info->name);
		} else {
			sprintf(pos, "\033[92m%s", info->name);
		}
		printf("%s\033[0m\n", output);
		++count;
		scan = scan->next;
		//printf("debug4\n");
	}
	free(temp);
	free(output);
	return count;
}

void sort(struct list* list) {
	if (list->head == NULL) {
		return;
	}
	struct node* iscan = list->head->next;
	while (iscan != NULL) {
		struct node* jscan = iscan;
		while (jscan->prev != NULL && strcmp((char*)jscan->item, (char*)jscan->prev->item) < 0) {
			char* temp = jscan->prev->item;
			jscan->prev->item = jscan->item;
			jscan->item = temp;
			jscan = jscan->prev;
		}
	iscan = iscan->next;
	}
}

void sortPrintList(struct list* list) {
	if (list->head == NULL) {
		return;
	}
	struct node* iscan = list->head->next;
	while (iscan != NULL) {
		struct node* jscan = iscan;
		while (jscan->prev != NULL && strcmp((char*)((struct printInfo*)(jscan->item))->name, (char*)((struct printInfo*)(jscan->prev->item))->name) < 0) {
			char* temp = jscan->prev->item;
			jscan->prev->item = jscan->item;
			jscan->item = temp;
			jscan = jscan->prev;
		}
	iscan = iscan->next;
	}
}

int globalDirCount = 0;

int lsArg(char* path) {
	struct list* printList = calloc(1, sizeof(struct list));
	struct list* dirList = calloc(1, sizeof(struct list));
	DIR* dir = opendir(path);
	struct dirent* inDir = readdir(dir);
	//inDir = readdir(dir);
	//inDir = readdir(dir);

	resetSpaces();
	while (inDir != NULL) {
		if (inDir->d_name[0] != '.') {
			struct printInfo* printEntry = malloc(sizeof(struct printInfo));
			printEntry->path = malloc(sizeof(char)*PATH_MAX);
			printEntry->name = malloc(sizeof(char)*PATH_MAX);
			printEntry->fileStats = malloc(sizeof(struct stat));
			strcpy(printEntry->path, path);
			strcpy(printEntry->name, inDir->d_name);
			char* filepath = malloc(sizeof(char)*PATH_MAX);
			sprintf(filepath, "%s/%s", printEntry->path, printEntry->name);
			int res = lstat(filepath, printEntry->fileStats);
			if (res == -1) {
				printf("file accress error: %s | errno: %s\n", printEntry->path, strerror(errno));
			}
			add(printEntry, printList);
			if (RFlag == 1 && S_ISDIR(printEntry->fileStats->st_mode)) {
				add(filepath, dirList);
				++globalDirCount;
			}
			updateSpaces(printEntry->fileStats);
		}
		inDir = readdir(dir);
	}
	sortPrintList(printList);
	if (globalDirCount > 1 || RFlag == 1) {
		printf("%s:\n", path);
	}
	printPrintList(printList);
	if (dirList->head != NULL) {
		printf("\n");
	}

	if (RFlag == 1 && dirList->head != NULL) { // processing directories
		sort(dirList);
		struct node* it = dirList->head;
		while (it != NULL) {
			lsArg((char*)it->item);
			if (it->next != NULL) {
				printf("\n");
			}			
			it = it->next;
		}
	}
	closedir(dir);
	freePrintList(printList);
	freeList(dirList);
}

int main(int argc, char* argv[]) {
	//process tokens: options checked, paths queued
	struct list* argList = malloc(sizeof(struct list));
	argList->head = NULL;
	argList->tail = NULL;
	int res = processTokens(argc, argv, argList);
	if (res == -1) {
		printf("invalid option\n");
		return -1;
	}
	if (res == 0) { //ls current directory
		++globalDirCount;
		lsArg(".");
	} else { //ls arguments
		//insertion sort argList
		if (res > 1) {
			sort(argList);
			globalDirCount = 2;
		}
		//check args if file/directory
		struct node* argscan = argList->head;
		struct list* printList = malloc(sizeof(struct list));
		struct list* dirList = malloc(sizeof(struct list));
		while (argscan != NULL) {
			//build print list and directories to explore from args
			struct stat* fileStats = malloc(sizeof(struct stat));
			int res = lstat((char*)argscan->item, fileStats);
			if (res == -1) {
				printf("%s is not a valid file or directory\n", (char*)argscan->item);
			} else if (S_ISREG(fileStats->st_mode)) {
				struct printInfo* printEntry = malloc(sizeof(struct printInfo));
				printEntry->fileStats = fileStats;
				printEntry->name = (char*)argscan->item;
				updateSpaces(fileStats);
				add(printEntry, printList);
			} else {
				char* dirPath = malloc(sizeof(char)*PATH_MAX);
				strcpy(dirPath, (char*)argscan->item);
				add(dirPath, dirList);
				++globalDirCount;
			}
			argscan = argscan->next;
		}
		sortPrintList(printList);
		printPrintList(printList);
		resetSpaces();			
		if (printList->head != NULL && dirList->head != NULL) {
			printf("\n");
		}
		//ls dirList
		struct node* dirListScan = dirList->head;
		while (dirListScan != NULL) {
			lsArg((char*)dirListScan->item);
			if (dirListScan->next != NULL) {
				printf("\n");
			}
			dirListScan = dirListScan->next;
		}
		freeList(dirList);
	}
}