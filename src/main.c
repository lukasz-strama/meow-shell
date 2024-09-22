#define MEOW_RL_BUFSIZE 1024

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
