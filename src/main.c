#define MEOW_RL_BUFSIZE 1024
#define MEOW_TOK_BUFSIZE 64
#define MEOW_TOK_DELIM " \t\r\n\a"

#include <stdio.h>

void meow_loop(void);
char *meow_read_line(void);

int main(int argc, char **argv) {

  // load config files if any

  // run command loop

  // perform exit/cleanup

  return 0;
}

void meow_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
  }
  while (status);
}

char *meow_read_line(void)
{
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

char **lsh_split_line(char *line)
{
  int bufsize = MEOW_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
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
        fprintf(stderr, "lsh: allocation error\n");
        exit(1);
      }
    }

    token = strtok(NULL, MEOW_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}
