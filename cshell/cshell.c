#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

struct list {
	struct node* head;
	struct node* tail;
	int size;
};

struct node {
	void* item;
	struct node* next;
};

struct logEntry {
	char* command;
	char* time;
	int retVal;
};

struct var {
	char* name;
	char* value;
};

void push(struct list* target, void* item) {
	if (!target->head) {
		target->head = malloc(sizeof(struct node));
		target->tail = target->head;
		target->head->item = item;
		target->head->next = NULL;
	} else {
		target->tail->next = malloc(sizeof(struct node));
		target->tail->next->item = item;
		target->tail->next->next = NULL;
		target->tail = target->tail->next;
	}
}

void enterLog(struct list* log, char* command, int retVal) {
	struct logEntry* newLogEntry = malloc(sizeof(struct logEntry));
	newLogEntry->command = malloc(sizeof(char)*50);
	strcpy(newLogEntry->command, command);
	newLogEntry->time = malloc(sizeof(char)*50);
	time_t timer = time(&timer);
	strcpy(newLogEntry->time, asctime(gmtime(&timer)));
	newLogEntry->retVal = retVal;
	push(log, newLogEntry);
}

struct node* parseInputLine(struct list* parse, char* input) {
	char* scan = input;
	struct node* end = parse->head;
	while (scan && *scan > 32) { //splitting the input into individual tokens stored in a list for further handling
		char* word = malloc(sizeof(char)*50);
		char* dump = word;
		while (*scan > 32) { //manually copying args into new strings
			*dump = *scan;
			dump++;
			scan++;
		}
		if (*scan == '\0') break;
		*dump = '\0';
		if (!end) { //reuse existing list nodes to store new command, expand if needed
			push(parse, word);
		} else {
			free(end->item);
			end->item = word;
			end = end->next;
		}
		scan++;
	}
	return end;
}
char* blank = "";
char* black = "\033[30m";
char* red = "\033[31m";
char* green = "\033[32m";
char* yellow = "\033[33m";
char* blue = "\033[34m";
char* magenta = "\033[35m";
char* cyan = "\033[36m";
char* white = "\033[37m";
char* brightblack = "\033[90m";
char* brightred = "\033[91m";
char* brightgreen = "\033[92m";
char* brightyellow = "\033[93m";
char* brightblue = "\033[94m";
char* brightmagenta = "\033[95m";
char* brightcyan = "\033[96m";
char* brightwhite = "\033[97m";

void interpretCommand(struct list* parse, struct list* vars, struct list* log, struct node* curr, struct node* end, char** theme) {
	char* command = (char*)parse->head->item;
	if (command[0] == '$') {
		// set up string for strcpy operations
		char* scan = command+1;
		char* varName = command+1;
		char* varVal;
		while (*scan != '=') {
			++scan;
		}
		*scan = '\0';
		varVal = scan+1;

		// check for pre-existing variable with same name
		struct node* varCheck = vars->head;
		while (varCheck && strcmp(((struct var*)varCheck->item)->name, varName) != 0 ) {
			++varCheck;
		}
		if (varCheck) { // var exists, reassign value
			strcpy(((struct var*)varCheck->item)->value, varVal);
		} else { // var doesn't exist, add to list
			struct var* newVar = malloc(sizeof(struct var));
			newVar->name = malloc(sizeof(char)*20);
			newVar->value = malloc(sizeof(char)*20);
			strcpy(newVar->name, varName);
			strcpy(newVar->value, varVal);
			push(vars, newVar);
		}
	enterLog(log, command, 0);

	} else if (strcmp(command, "log") == 0) { // built-in commands: log
		curr = log->head;
		while(curr) {
			printf("%s%s%s%s %i\n", *theme, ((struct logEntry*)curr->item)->time, *theme, ((struct logEntry*)curr->item)->command, ((struct logEntry*)curr->item)->retVal);
			curr = curr->next;
		}
		enterLog(log, command, 0);

	} else if (strcmp(command, "exit") == 0) { // exit
		exit(0);

	}/* else if (strcmp(command, "uppercase") == 0) {
		struct node* string = parse->head->next;
		char* buf = calloc(400, sizeof(char));
		char* temp = buf;
		while (string != end) {
			strcpy(temp, (char*)string->item);
			while(*temp != '\0') ++temp;
			*temp = ' ';
			++temp;
			string = string->next;
		}
		int res = uppercase(buf);
		if (res != 0) {
			enterLog(log, command, -1);	
		} else {
			enterLog(log, command, 0);
		}

	}*/ else if (strcmp(command, "print") == 0) { // print
		curr = parse->head->next;
		while (curr && curr != end) {
			if (((char*)curr->item)[0] == '$') {
				struct node* target = vars->head;
				while (target) {
					if (strcmp(((char*)curr->item)+1, ((struct var*)target->item)->name) == 0) {
						curr->item = ((struct var*)target->item)->value;
						break;
					}
					target = target->next;
				}
				if (!target) {
					printf("%s%s not assigned\n", *theme, ((char*)curr->item)+1);
					enterLog(log, command, -1);
				}
			}
			curr = curr->next;
		}
		curr = parse->head->next;
		while (curr && curr != end) {
			printf("%s%s\n", *theme, (char*)curr->item);
			curr = curr->next;
		}
		enterLog(log, command, 0);

	} else if (strcmp(command, "theme") == 0) { // theme
		char* arg = (char*)parse->head->next->item;
		if (!strcmp(arg, "black")) {
			*theme = black;
			enterLog(log, command, 0);
		}
		else if (!strcmp(arg, "red")) {
			*theme = red;
			enterLog(log, command, 0);
		}
		else if (!strcmp(arg, "green")) {
			*theme = green;
			enterLog(log, command, 0);
		}
		else if (!strcmp(arg, "yellow")) {
			*theme = yellow;
			enterLog(log, command, 0);
		}
		else if (!strcmp(arg, "blue")) {
			*theme = blue;
			enterLog(log, command, 0);
		}
		else if (!strcmp(arg, "magenta")) {
			*theme = magenta;
			enterLog(log, command, 0);
		}
		else if (!strcmp(arg, "cyan")) {
			*theme = cyan;
			enterLog(log, command, 0);
		}
		else if (!strcmp(arg, "white")) {
			*theme = white;
			enterLog(log, command, 0);
		}
		else if (!strcmp(arg, "bright")) {
			char* arg2 = (char*)parse->head->next->next->item;
			if (!strcmp(arg2, "black")) {
				*theme = brightblack;
				enterLog(log, command, 0);
			}
			else if (!strcmp(arg2, "red")) {
				*theme = brightred;
				enterLog(log, command, 0);
			}
			else if (!strcmp(arg2, "green")) {
				*theme = brightgreen;
				enterLog(log, command, 0);
			}
			else if (!strcmp(arg2, "yellow")) {
				*theme = brightyellow;
				enterLog(log, command, 0);
			}
			else if (!strcmp(arg2, "blue")) {
				*theme = brightblue;
				enterLog(log, command, 0);
			}
			else if (!strcmp(arg2, "magenta")) {
				*theme = brightmagenta;
				enterLog(log, command, 0);
			}
			else if (!strcmp(arg2, "cyan")) {
				*theme = brightcyan;
				enterLog(log, command, 0);
			}
			else if (!strcmp(arg2, "white")) {
				*theme = brightwhite;
				enterLog(log, command, 0);
			} else {
				enterLog(log, command, -1);
			}
		} else {
			enterLog(log, command, -1);
		}
		
	} else { // non-built in command
		//replace $VAR args with values, if not exist print error and return
		curr = parse->head;
		int argsCount = 0;
		while (curr && curr != end) {
			if (((char*)curr->item)[0] == '$') {
				struct node* target = vars->head;
				while (target) {
					if (strcmp(((char*)curr->item)+1, ((struct var*)target->item)->name) == 0) {
						curr->item = ((struct var*)target->item)->value;
						break;
					}
					target = target->next;
				}
				if (!target) {
					printf("%s%s not assigned\n", *theme, ((char*)curr->item)+1);
					return;
				}
			}
			curr = curr->next;
			++argsCount;
		}

		//build arg array from list of inputs
		curr = parse->head;
		char** argsList = malloc(sizeof(char*)*argsCount + 1);
		argsList[argsCount] = (char*)NULL;
		int it = 0;
		while (it < argsCount) {
			argsList[it] = ((char*)curr->item);
			curr = curr->next;
			++it;
		}

		int thePipe[2];
		int errPipe[2];
		if (pipe(thePipe) == -1 || pipe(errPipe) == -1) {
			printf("%sError Creating Pipe. Try again.\n", *theme);
		} else {
			pid_t cpid;
			cpid = fork();
			if (cpid == -1) {
				printf("%sFork Error. Try again.\n", *theme);
			} else if (cpid == 0) { // child does stuff
				close(thePipe[0]);
				close(errPipe[0]);
				dup2(thePipe[1], STDOUT_FILENO);
				dup2(errPipe[1], STDERR_FILENO);
				printf("%s", *theme);
				int err = execvp(argsList[0], argsList);
				if (err == -1) {
					exit(-1);
				}
			} else { //parent does stuff
				close(thePipe[1]);
				close(errPipe[1]);
				int statusRet;
				wait(&statusRet);
				if (statusRet != 0) { // error has happened
					char* errbuf = calloc(1000, sizeof(char));
					int errread = read(errPipe[0], errbuf, 999);
					int off = 0;
					while(*(errbuf+off) > 31) {++off;}
					printf("%sError: Command: %s, return code:%i\n", *theme, argsList[0], statusRet);
					write(STDOUT_FILENO, errbuf, off);
					//printf("\n");
					free(errbuf);
					enterLog(log, command, -1);
				} else { //all is good go ahead and print output
					char* buf = calloc(4000, sizeof(char));
					int numread = read(thePipe[0], buf, 3999);
					int off = 0;
					int lineStart = 0;
					char* output = calloc(1000, sizeof(char));

					while (off < numread) {
						if (*(buf+off) < 32) {
							*(buf+off) = '\0';
							strcpy(output, buf+lineStart);
							printf("%s%s\n", *theme, output);
							lineStart = off+1;
						}
						++off;
					}
					free(buf);
					free(output);
					enterLog(log, command, statusRet);
				}
				close(thePipe[0]);
				close(errPipe[0]);
			}
		}
	}
}



int main(int argc, char* argv[]) {
	struct list* parse = malloc(sizeof(struct list));
	struct list* vars = malloc(sizeof(struct list));
	struct list* log = malloc(sizeof(struct list));
	struct node* curr = NULL; // used for iterating through input args
	struct node* end = NULL; // used for setting an endpoint for iterating through input args
	char input[200];
	char* theme = blank;


	if (argc == 1) { // interactive mode
		while (1) {
			printf("%scshell$ \033[37m", theme);
			fgets(input, 199, stdin);
			end = parseInputLine(parse, input);
			if (end != parse->head) {
				interpretCommand(parse, vars, log, curr, end, &theme);
			}
		}
	} else if (argc == 2){ // script mode
		FILE* script = fopen(argv[1], "r");
		if (!script) {
			printf("%sFile %s does not exist.\n", theme, argv[1]);
			return 0;
		}
		while(1) {
			char* lineCopy = input;
			char read = fgetc(script);
			while(read > 31) {
				*lineCopy = read;
				++lineCopy;
				read = fgetc(script);
			}
			if (read == EOF) return 0;
			*lineCopy++ = 13;
			*lineCopy++ = 10;
			*lineCopy = '\0';
			end = parseInputLine(parse, input);
			if (end != parse->head) {
				interpretCommand(parse, vars, log, curr, end, &theme);
			}
		}
		fclose(script);
		printf("%sEnd of script\n", theme);
	} else { // too many args
		printf("%sToo many arguments, cshell takes up to 1 script as argument.\n", theme);
	}
	return 0;
}
