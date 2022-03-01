#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char file_name[100];
  int lines_num;
  int file_kind;

  printf("Enter file file_name (without file_kind): ");
  scanf("%s", file_name);

  printf("Enter file file_kind (1: TXT, 2: BIN): ");
  scanf("%d", &file_kind);

  switch (file_kind) {
  case 1:
    strcat(file_name, ".txt");
    break;
  case 2:
    strcat(file_name, ".bin");
    break;
  default:
    printf("Error: invalid file kind.");
    exit(EXIT_FAILURE);
  }

  printf("Enter file lines number: ");
  scanf("%d", &lines_num);
  getchar();

  char *lines[lines_num];

  for (int i = 0; i < lines_num; i++) {
    char *line = new char[1024];
    printf(" Enter line no. %d: ", i + 1);
    scanf("%1023[^\n]", line);
    getchar();

    lines[i] = line;
  }

  FILE *f;
  switch (file_kind) {
  case 1:
    if (!(f = fopen(file_name, "w"))) {
      printf("Error: creating file failure.");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < lines_num; i++) {
      fputs(lines[i], f);
      if (i < lines_num - 1)
        fputc('\n', f);
    }

    return EXIT_SUCCESS;
  case 2:
    if (!(f = fopen(file_name, "wb"))) {
      printf("Error: creating file failure.");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < lines_num; i++) {
      int len = strlen(lines[i]);
      fwrite(&len, sizeof(int), 1, f);
      fwrite(lines[i], sizeof(char), len, f);
    }

    return EXIT_SUCCESS;
  }
}
