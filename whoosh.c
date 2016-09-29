// Shell: has functionality that would be expected of any shell
// Type an input to the "whoosh> " prompt:
// @author: Alex Curtis
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>

char** path;
void errorExit() {
  fprintf(stderr, "An error has occurred\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  int i;
  if (argc > 1) errorExit();
  path = malloc(sizeof(char*) * 1); // set initial path
  *path = malloc(sizeof(char) * 5);
  if (path == NULL || *path == NULL) errorExit();
  strcpy(*path, "/bin");

  while (1) {
    fprintf(stdout, "whoosh> ");
    fflush(stdout);
    char cmd[129]; // max command size 128
    cmd[128] = '\0';
    fgets(cmd, 130, stdin);
    
    // error: command is > 128 characters (flush stdin)
    if (cmd[128] != '\0' && cmd[128] != '\n') {
      fprintf(stderr, "An error has occurred\n");
      int c;
      while ((c = getchar()) != '\n' && c != EOF) { }
      continue;
    }

    // count number of arguments
    int nArgs = 1; // will always have null
    char prev = ' ';
    for (i = 0; i < 128; i++) {
      if (cmd[i] == ' ' && prev != ' ')	nArgs++;
      if (cmd[i] == '\n' && prev != ' ') nArgs++;
      if (cmd[i] == '\n' || cmd[i] == '\0') break;
      prev = cmd[i];
    }

    // put arguments into an array
    char** args = malloc(sizeof(char*) * nArgs);
    if (args == NULL) errorExit();
    args[0] = strtok(cmd, " \n\r");
    for (i = 1; i < nArgs; i++) args[i] = strtok(NULL, " \n\r");

    // handle built-in commands
    if (args[0] == NULL) continue; // empty command line
    if (strcmp(args[0], "exit") == 0) exit(0);
    if (strcmp(args[0], "pwd") == 0) printf("%s\n", getcwd(NULL, 0));
    if (strcmp(args[0], "cd") == 0) 
      if ((nArgs == 2 ? chdir(getenv("HOME")) : chdir(args[1])) != 0)
	fprintf(stderr, "An error has occurred\n");
    if (strcmp(args[0], "path") == 0) {
      for (i = 0; *(path + i) != NULL; i++) free(*(path + i));
      path = (char**)realloc(path, (nArgs - 1));
      for (i = 0; i < nArgs - 2; i++) {
	*(path + i) = malloc(strlen(args[i + 1]) + 1);
	if (*(path + i) == NULL) errorExit();
	strcpy(*(path + i), args[i + 1]);
      }
      *(path + (nArgs - 2)) = NULL;
    }
    if (strcmp(args[0], "path") == 0 || strcmp(args[0], "pwd") == 0 ||
	strcmp(args[0], "cd") == 0) continue;

    // for non-built in command: get path (if exists)
    char* fullPath = NULL;
    for (i = 0; *(path + i) != NULL; i++) {
      fullPath = malloc(sizeof(char) * PATH_MAX);
      if (fullPath == NULL) errorExit();
      strcpy(fullPath, *(path + i));
      strcat(fullPath, "/");
      strcat(fullPath, args[0]);
      if (access(fullPath, F_OK) != -1) break;
      free(fullPath);
      fullPath = NULL;
    }
    if (fullPath == NULL) {
      fprintf(stderr, "An error has occurred\n");
      continue;
    }
    args[0] = fullPath;

    // check for redirection, and modify args appropriately
    char* redirectFileName = NULL;
    int badRedirect = 0;
    for (i = 0; *(args + i) != NULL; i++) {
      if (strcmp(*(args + i), ">") == 0) {
	if (*(args + i + 1) == NULL || *(args + i + 2) != NULL) {
	  badRedirect = 1;	
	  break;
	}
	redirectFileName = malloc(sizeof(char) * 129);
	if (redirectFileName == NULL) errorExit();
	strcat(redirectFileName, *(args + i + 1));
	args = realloc(args, sizeof(char*) * (nArgs - 2));
	*(args + nArgs - 3) = NULL;
      }
    }
    if (badRedirect == 1) {
      fprintf(stderr, "An error has occurred\n");
      continue;
    }

    // run the external program (fork and wait for child!)
    int rc = fork();
    if (rc == 0) {
      if (redirectFileName != NULL) { // redirect if valid
	char* redirectStdErr = malloc(sizeof(char) * 129);
	if (redirectStdErr == NULL) errorExit();
	strcpy(redirectStdErr, redirectFileName);
	strcat(redirectFileName, ".out");
	strcat(redirectStdErr, ".err");
	close(STDOUT_FILENO);
	open(redirectFileName, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (access(redirectFileName, F_OK) != 0) { // checks for invalid path
	  fprintf(stderr, "An error has occurred\n");
	  continue;
	}
	close(STDERR_FILENO);
	open(redirectStdErr, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
      }
      execv(fullPath, args);
      errorExit();
    } else if (rc > 0) {
      wait(NULL); 
    } else {
      errorExit();
    }
    free(redirectFileName);    
    free(fullPath);
  }
  for (i = 0; *(path + i) != NULL; i++) free(*(path + i));
  free(path);
  return 0;
}
