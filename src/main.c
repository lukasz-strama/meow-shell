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
  int bufsize = MEOW_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "meow: allocation error\n");
    exit(1);
  }

  while(1) {
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    if(position >= bufsize) {
      bufsize += MEOW_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);

      if (!buffer) {
        fprintf(stderr, "meow: allocation error\n");
        exit(1);
      }
    }
  }
}
