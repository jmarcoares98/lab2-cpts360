//MARCO ARES
//11526185

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


char USER[32];
char* HOME;
char* token;
char tString[1024];
char cmd[64];
char input[16][16];
char* head[64], * tail[64];
char** ARGV;
int ARGC = 0, tl = 0, hd = 0;

int main(int argc, char* argv[], char* env[]) {
	int pid, status;
	int i = 0, k = 0;
	char line[123];
	char* home = getenv("HOME");	// gets home	
	char* path = getenv("PATH");	// gets path
	char* user = getenv("LOGNAME");

	// gets user name
	char* temp = strtok(user, ":");
	strcpy(USER, temp);

	printf("home = %s\n", home);
	printf("path = %s\n", path);

	tokenize(path);

	printf("\n*********** %s processing loop **********\n", user);

	while (1) {

		printf("%s %: ", USER);
		fgets(line, 128, stdin);
		line[strlen(line) - 1] = 0; // kill n\ at end of line[]
		
		createMyARGV(line);

		strcpy(tString, line);
		token = strtok(tString, " ");

		if (token == NULL) {
			continue;
		}

		strcpy(cmd, token);

		if (strcmp(cmd, "exit") == 0) {
			exit(0);
		}

		else if (strcmp(cmd, "cd") == 0) {
			token = strtok(NULL, " ");

			if (token == NULL) {
				chdir(HOME);
				printf("%d going home\n", getpid());
			}
			else {
				if (chdir(token))
					printf("%d cd to %s\n", getpid(), token);
				else
					printf("failed to cd\n");
			}
		}

		else {

			pid = fork();

			if (pid)
			{
				// parent sh
				printf("parent %s %d forks a child process %d\n", USER, getpid(), pid);
				printf("parent %s %d waits\n", USER, getpid());
				pid = wait(&status);

				printf("child %s %d died : exit status = %04x\n", USER, pid, status);
				continue;
			}

			else
			{
				//child sh
				printf("%d line = %s\n", getpid(), line);

				do_pipe(line);
			}

			exit(0);
		}
	}
}

//tokenize path
void tokenize(char* pathname)
{
	char line[123];
	char* s;
	int i = 0;

	printf("\ndecompose path into strings: \n");
	strcpy(line, pathname);

	// tokenize function!!
	s = strtok(line, ":");
	while (s)
	{
		strcpy(input[i], s);
		printf("%s  ", input[i]);
		s = strtok(0, ":");
		i++;
	}

	printf("\n");
}

//does the pipe if there is one
int do_pipe(char* cmdLine)
{
	int lpd[2], pid, hasPipe = 0;

	// divide input line into head, tail by rightmost pipe symbol
	hasPipe = scan();
	
	if (hasPipe) {
		for (int l = 0; l < hd; l++) {
			printf("HEAD: %s\n", head[l]);
		}

		for (int n = 0; n < tl; n++) {
			printf("TAIL: %s\n", tail[n]);
		}

		pipe(lpd); //create a pipe lpd

		pid = fork();
		if (pid) { // parent
			close(lpd[1]);
			dup2(lpd[0], 0);
			close(lpd[0]);
			do_command(tail);
		}

		else { // child
			close(lpd[0]);
			dup2(lpd[1], 1);
			close(lpd[1]);
			do_command(head);
		}
	}
	else 
		do_command(ARGV);
}

// executes the command line
int do_command(char* myargv[]) {
	char cmdPath[16] = "/bin/";

	strcat(cmdPath, myargv[0]);
	int loc = IOredirection(myargv);

	if (loc > 0) {
		myargv[loc] = NULL;
	}

	//to open editor
	if (strcmp(myargv[0], "gedit") == 0) {
		execlp("gedit", "gedit", myargv[1], NULL);
	}

	////print out argc and argv
	//for (int i = 0; i < ARGC - 1; i++) {
	//	printf("%d %s\n", i, myargv[i]);
	//}

	////printf out the paths
	//for (int j = 0; j <= 5; j++) {
	//	strcat(input[j], "/");
	//	strcat(input[j], myargv[0]);
	//	printf("name[0]=%s\ti=%d\tcmd=%s\n", myargv[0], j, input[j]);
	//}

	int r = execve(cmdPath, myargv, NULL);

	if (r == -1)
		printf("execve() failed: r = %d\n", r);
}

// scan head and tail, also check if there is pipe
int scan() {
	int i = 0, j = 0, k = 0;
	int lineLen = ARGC - 1;

	// divide cmdLine into head and tail by rightmost | symbol
	// cmdLine = cmd1 | cmd2 | ... | cmdn-1 | cmdn
	//           |<----------head --------->| tail |; return 1;
	// cmdLine = cmd1 ==> head = cmd1, tail = null;  return 0;

	//does not do multiple pipes
	for (i = 0; i < lineLen; i++) {
		if (strcmp(ARGV[i], "|") == 0) {
			j = 0;
			k = 0;
			while (ARGV[j]) {
				if (j > i) {
					tail[k] = ARGV[j];
					k++;
					tl++;
				}

				j++;
			}
			return 1;
		}

		head[i] = ARGV[i];
		hd++;

	}

	return 0;
}

int IOredirection(char *argv[]) {
	// cmd arg1 arg2 ... <  infile  // take inputs from infile
	// cmd arg1 arg2 ... >  outfile // send outputs to outfile
	// cmd arg1 arg2 ... >> outfile // APPEND outputs to outfile
	int i = 0;

	while (argv[i]) {
		//takes input from infile
		if (strcmp(argv[i], "<") == 0) {
			redirect(1, argv[i + 1]);
			return i;
		}

		//sends outputs to outfile
		else if (strcmp(argv[i], ">") == 0) {
			redirect(2, argv[i + 1]);
			return i;
		}

		//appends output to outfile
		else if (strcmp(argv[i], ">>") == 0) {
			redirect(3, argv[i + 1]);
			return i;
		}
		i++;
	}

	return i;
}

//to open files with IO directions
int redirect(int direction, char file[]) {
	if (direction == 1) {
		close(0);
		open(file, O_RDONLY);
	}

	else if (direction == 2) {
		close(1);
		open(file, O_WRONLY | O_CREAT, 0644); 
	}

	else if (direction == 3) {
		close(1);
		open(file, O_WRONLY | O_APPEND, 0644);
	}
}

//creating my argv for other functions
int createMyARGV(char* line) {
	char* tok = (char *)malloc(100);
	int i = 2, true = 0;

	//checks if its more than one in the line
	tok = strtok(line, " ");
	if (tok == NULL) {
		return;
	}
	//gets the first string

	ARGV = calloc(i, sizeof(char*));
	ARGV[0] =  tok;

	//checks if there are strings after first one
	tok = strtok(NULL, " ");
	ARGV[1] = tok;

	//continues checking
	while (tok != NULL) {
		i++;
		tok = strtok(NULL, " ");
		ARGV = realloc(ARGV, sizeof(char*) * i);
		ARGV[i - 1] = tok;
	}

	// define ARGC
	ARGC = i;

}