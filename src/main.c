#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MEOW_RL_BUFSIZE 1024
#define MEOW_TOK_BUFSIZE 64
#define MEOW_TOK_DELIM " \t\r\n\a"
#define MAX_ENTRIES 1024
#define MAX_PATH 1024
#define MAX_NAME 256

struct FileInfo {
  char name[MAX_NAME];
  char type;
  time_t mod_time;
  char permissions[12];
};

void MeowLoop(void);
char* MeowReadLine(void);
char** MeowSplitLine(char* line);
int MeowLaunch(char** args);
int MeowExecute(char** args);
int MeowNumBuiltins(void);

int MeowCd(char** args);
int MeowHelp(char** args);
int MeowExit(char** args);
int MeowLs(char** args);

int CompareFileInfo(const void* a, const void* b);
void FormatTime(time_t mod_time, char* buffer, size_t bufsize);
char GetFileType(struct stat* file_stat);
const char* GetFileExtension(const char* filename, char type);
void GetPermissions(mode_t mode, char* perm_str);

char* builtin_str[] = {"cd", "help", "exit", "ls"};
int (*builtin_func[])(char**) = {&MeowCd, &MeowHelp, &MeowExit, &MeowLs};

int main(int argc, char** argv) {
  MeowLoop();
  return EXIT_SUCCESS;
}

void MeowLoop(void) {
  char* line;
  char** args;
  int status;

  do {
    printf("$ ");
    line = MeowReadLine();
    args = MeowSplitLine(line);
    status = MeowExecute(args);

    free(line);
    free(args);
  } while (status);
}

char* MeowReadLine(void) {
  char* line = NULL;
  size_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else {
      perror("MeowReadLine");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

char** MeowSplitLine(char* line) {
  int bufsize = MEOW_TOK_BUFSIZE;
  int position = 0;
  char** tokens = malloc(bufsize * sizeof(char*));
  char* token;

  if (!tokens) {
    fprintf(stderr, "MeowSplitLine: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, MEOW_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += MEOW_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "MeowSplitLine: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, MEOW_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int MeowLaunch(char** args) {
  pid_t pid = fork();
  int status;

  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("MeowLaunch");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("MeowLaunch");
  } else {
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int MeowNumBuiltins(void) {
  return sizeof(builtin_str) / sizeof(char*);
}

int MeowCd(char** args) {
  if (args[1] == NULL) {
    fprintf(stderr, "MeowCd: expected argument\n");
  } else if (chdir(args[1]) != 0) {
    perror("MeowCd");
  }
  return 1;
}

int MeowHelp(char** args) {
  printf("Meow Shell - 1.0.0\n");
  for (int i = 0; i < MeowNumBuiltins(); i++) {
    printf("- %s\n", builtin_str[i]);
  }
  return 1;
}

int MeowExit(char** args) {
  return 0;
}

int MeowExecute(char** args) {
  if (args[0] == NULL) {
    return 1;
  }

  for (int i = 0; i < MeowNumBuiltins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return MeowLaunch(args);
}

int MeowLs(char** args) {
  DIR* dir;
  struct dirent* entry;
  struct stat file_stat;
  char path[MAX_PATH] = ".";
  struct FileInfo files[MAX_ENTRIES];
  int count = 0;
  int max_name_len = 0;
  int max_type_len = 0;
  int show_permissions = 0;

  // Parse arguments
  for (int i = 1; args[i] != NULL; i++) {
    if (strcmp(args[i], "-p") == 0) {
      show_permissions = 1;
    } else {
      strncpy(path, args[i], MAX_PATH - 1);
      path[MAX_PATH - 1] = '\0';
    }
  }

  dir = opendir(path);
  if (dir == NULL) {
    perror("MeowLs");
    return 1;
  }

  // Read directory entries
  while ((entry = readdir(dir)) != NULL && count < MAX_ENTRIES) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    if (stat(full_path, &file_stat) == -1) {
      perror("MeowLs");
      continue;
    }

    strncpy(files[count].name, entry->d_name, MAX_NAME - 1);
    files[count].name[MAX_NAME - 1] = '\0';
    files[count].type = GetFileType(&file_stat);
    files[count].mod_time = file_stat.st_mtime;

    if (show_permissions) {
      GetPermissions(file_stat.st_mode, files[count].permissions);
    }

    int name_len = strlen(entry->d_name);
    if (name_len > max_name_len) {
      max_name_len = name_len;
    }

    const char* file_type = GetFileExtension(entry->d_name, files[count].type);
    int type_len = strlen(file_type);
    if (type_len > max_type_len) {
      max_type_len = type_len;
    }

    count++;
  }

  closedir(dir);

  // Sort files
  qsort(files, count, sizeof(struct FileInfo), CompareFileInfo);

  // Print header
  if (show_permissions) {
    printf("| %-*s | %-*s | %-11s | %-16s |\n", max_name_len, "Name",
           max_type_len, "Type", "Permissions", "Modified Date");
    printf("|-%.*s-|-%.*s-|-------------|------------------|\n", max_name_len,
           "--------------------------------", max_type_len,
           "----------------");
  } else {
    printf("| %-*s | %-*s | %-16s |\n", max_name_len, "Name", max_type_len,
           "Type", "Modified Date");
    printf("|-%.*s-|-%.*s-|------------------|\n", max_name_len,
           "--------------------------------", max_type_len,
           "----------------");
  }

  // Print file information
  for (int i = 0; i < count; i++) {
    char time_buf[20];
    FormatTime(files[i].mod_time, time_buf, sizeof(time_buf));

    const char* file_type = GetFileExtension(files[i].name, files[i].type);

    if (show_permissions) {
      printf("| %-*s | %-*s | %-11s | %-16s |\n", max_name_len, files[i].name,
             max_type_len, file_type, files[i].permissions, time_buf);
    } else {
      printf("| %-*s | %-*s | %-16s |\n", max_name_len, files[i].name,
             max_type_len, file_type, time_buf);
    }
  }

  return 1;
}

int CompareFileInfo(const void* a, const void* b) {
  const struct FileInfo* fa = (const struct FileInfo*)a;
  const struct FileInfo* fb = (const struct FileInfo*)b;

  if (fa->type != fb->type) {
    return (fa->type == 'd') ? -1 : 1;
  }
  return strcmp(fa->name, fb->name);
}

void FormatTime(time_t mod_time, char* buffer, size_t bufsize) {
  struct tm* timeinfo = localtime(&mod_time);
  strftime(buffer, bufsize, "%Y-%m-%d %H:%M", timeinfo);
}

char GetFileType(struct stat* file_stat) {
  if (S_ISDIR(file_stat->st_mode)) {
    return 'd';
  } else if (S_ISREG(file_stat->st_mode)) {
    return '-';
  }
  return '?';
}

const char* GetFileExtension(const char* filename, char type) {
  if (type == 'd') {
    return "dir";
  }

  const char* dot = strrchr(filename, '.');
  if (!dot || dot == filename) {
    return "file";
  }
  return dot + 1;
}

void GetPermissions(mode_t mode, char* perm_str) {
  const char chars[] = "rwxrwxrwx";

  perm_str[0] = S_ISDIR(mode)  ? 'd'
                : S_ISLNK(mode)  ? 'l'
                : S_ISFIFO(mode) ? 'p'
                : S_ISSOCK(mode) ? 's'
                : S_ISCHR(mode)  ? 'c'
                : S_ISBLK(mode)  ? 'b'
                                 : '-';

  for (size_t i = 0; i < 9; i++) {
    perm_str[i + 1] = (mode & (1 << (8 - i))) ? chars[i] : '-';
  }

  if (mode & S_ISUID) perm_str[3] = (perm_str[3] == 'x') ? 's' : 'S';
  if (mode & S_ISGID) perm_str[6] = (perm_str[6] == 'x') ? 's' : 'S';
  if (mode & S_ISVTX) perm_str[9] = (perm_str[9] == 'x') ? 't' : 'T';

  perm_str[10] = '\0';
}
