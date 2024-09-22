#define MEOW_RL_BUFSIZE 1024
#define MEOW_TOK_BUFSIZE 64
#define MEOW_TOK_DELIM " \t\r\n\a"

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void meow_loop(void);
char *meow_read_line(void);
char **meow_split_line(char *line);
int meow_launch(char **args);

int meow_cd(char **args);
int meow_help(char **args);
int meow_exit(char **args);

int meow_execute(char **args);

int main(int argc, char **argv) {

  // load config files if any

  // run command loop
  meow_loop();

  // perform exit/cleanup

  return 0;
}

void meow_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf("$ ");
    line = meow_read_line();
    args = meow_split_line(line);
    status = meow_execute(args);

    free(line);
    free(args);
  }
  while (status);
}

char *meow_read_line(void) {
  char *line = NULL;
  size_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1){
    if (feof(stdin)) {
      exit(1);
    } else  {
      perror("readline");
      exit(0);
    }
  }

  return line;
}

char **meow_split_line(char *line) {
  int bufsize = MEOW_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "meow: allocation error\n");
    exit(1);
  }

  token = strtok(line, MEOW_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += MEOW_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "meow: allocation error\n");
        exit(1);
      }
    }

    token = strtok(NULL, MEOW_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int meow_launch(char **args) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("meow");
    }
    exit(1);
  } else if (pid < 0) {
    perror("meow");
  } else {
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &meow_cd,
  &meow_help,
  &meow_exit
};

int meow_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int meow_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "meow: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("meow");
    }
  }
  return 1;
}

int meow_help(char **args) {
  int i;
  printf("Meow Shell - 1.0.0\n");

  for (i = 0; i < meow_num_builtins(); i++) {
    printf("- %s\n", builtin_str[i]);
  }
  return 1;
}

int meow_exit(char **args) {
  return 0;
}

int meow_execute(char **args) {
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < meow_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return meow_launch(args);
}
