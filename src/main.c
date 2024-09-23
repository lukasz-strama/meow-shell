#define MEOW_RL_BUFSIZE 1024
#define MEOW_TOK_BUFSIZE 64
#define MEOW_TOK_DELIM " \t\r\n\a"
#define MAX_ENTRIES 1024

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

void meow_loop(void);
char *meow_read_line(void);
char **meow_split_line(char *line);
int meow_launch(char **args);

int meow_cd(char **args);
int meow_help(char **args);
int meow_exit(char **args);
int meow_ls(char **args);

int meow_execute(char **args);

struct file_info {
    char name[256];
    char type;
    time_t mod_time;
    char permissions[11];
};

int compare(const void *a, const void *b) {
    struct file_info *fa = (struct file_info *)a;
    struct file_info *fb = (struct file_info *)b;

    if (fa->type != fb->type) {
        return (fa->type == 'd') ? -1 : 1;
    }
    return strcmp(fa->name, fb->name);
}

void format_time(time_t mod_time, char *buffer, size_t bufsize) {
    struct tm *timeinfo = localtime(&mod_time);
    strftime(buffer, bufsize, "%Y-%m-%d %H:%M", timeinfo);
}

char get_file_type(struct stat *file_stat) {
    if (S_ISDIR(file_stat->st_mode)) {
        return 'd';
    } else if (S_ISREG(file_stat->st_mode)) {
        return '-';
    }
    return '?';
}

char *get_file_extension(char *filename, char type) {
    if (type == 'd') {
        return "dir";
    }

    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "file";
    }
    return dot + 1;
}

void get_permissions(mode_t mode, char *perm_str) {
    perm_str[0] = (S_ISDIR(mode)) ? 'd' : '-';
    perm_str[1] = (mode & S_IRUSR) ? 'r' : '-';
    perm_str[2] = (mode & S_IWUSR) ? 'w' : '-';
    perm_str[3] = (mode & S_IXUSR) ? 'x' : '-';
    perm_str[4] = (mode & S_IRGRP) ? 'r' : '-';
    perm_str[5] = (mode & S_IWGRP) ? 'w' : '-';
    perm_str[6] = (mode & S_IXGRP) ? 'x' : '-';
    perm_str[7] = (mode & S_IROTH) ? 'r' : '-';
    perm_str[8] = (mode & S_IWOTH) ? 'w' : '-';
    perm_str[9] = (mode & S_IXOTH) ? 'x' : '-';
    perm_str[10] = '\0';
}

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
  "exit",
  "ls"
};

int (*builtin_func[]) (char **) = {
  &meow_cd,
  &meow_help,
  &meow_exit,
  &meow_ls
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

int meow_ls(char **args) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char path[1024];
    struct file_info files[MAX_ENTRIES];
    int count = 0;
    int max_name_len = 0;
    int max_type_len = 0;
    int show_permissions = 0;  // Flag to check if -p is passed

    // Check for -p flag
    for (int i = 1; args[i] != NULL; i++) {
        if (strcmp(args[i], "-p") == 0) {
            show_permissions = 1;
        } else {
            strcpy(path, args[i]);
        }
    }

    if (args[1] == NULL || strcmp(args[1], "-p") == 0) {
        strcpy(path, ".");  // Default to current directory
    }

    dir = opendir(path);
    if (dir == NULL) {
        perror("meow_ls");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_stat) == -1) {
            perror("meow_ls");
            continue;
        }

        strcpy(files[count].name, entry->d_name);
        files[count].type = get_file_type(&file_stat);
        files[count].mod_time = file_stat.st_mtime;

        // Get file permissions if -p flag is provided
        if (show_permissions) {
            get_permissions(file_stat.st_mode, files[count].permissions);
        }

        int name_len = strlen(entry->d_name);
        if (name_len > max_name_len) {
            max_name_len = name_len;
        }

        char *file_type = get_file_extension(entry->d_name, files[count].type);
        int type_len = strlen(file_type);
        if (type_len > max_type_len) {
            max_type_len = type_len;
        }

        count++;
    }

    closedir(dir);

    // Sort files
    qsort(files, count, sizeof(struct file_info), compare);

    // Print header
    if (show_permissions) {
        printf("| %-*s | %-*s | %-11s | %-16s |\n", max_name_len, "Name", max_type_len, "Type", "Permissions", "Modified Date");
        printf("|-%.*s-|-%.*s-|-------------|------------------|\n", max_name_len, "--------------------------------", max_type_len, "----------------");
    } else {
        printf("| %-*s | %-*s | %-16s |\n", max_name_len, "Name", max_type_len, "Type", "Modified Date");
        printf("|-%.*s-|-%.*s-|------------------|\n", max_name_len, "--------------------------------", max_type_len, "----------------");
    }

    // Print file information
    for (int i = 0; i < count; i++) {
        char time_buf[20];
        format_time(files[i].mod_time, time_buf, sizeof(time_buf));

        char *file_type = get_file_extension(files[i].name, files[i].type);

        if (show_permissions) {
            printf("| %-*s | %-*s | %-11s | %-16s |\n", max_name_len, files[i].name, max_type_len, file_type, files[i].permissions, time_buf);
        } else {
            printf("| %-*s | %-*s | %-16s |\n", max_name_len, files[i].name, max_type_len, file_type, time_buf);
        }
    }

    return 1;
}
