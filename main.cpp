#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TODO(message)                                                          \
  printf("%s%s", "TODO: ", message);                                           \
  exit(EXIT_FAILURE);

const char STR_DELIMITER = '\n';
const int FILE_STR_MAX_LEN = 1024;
const int FILE_STRS_MAX_NUM = 256;

enum F_kind {
  BIN,
  TXT,
};

typedef struct F_str {
  FILE *f;
  F_kind f_kind;
  int offset;
  int len;
} F_str;

FILE *open_txt_file(char *src) {
  FILE *f;

  if (!(f = fopen(src, "r"))) {
    printf("Error: %s file open failure", src);
    exit(EXIT_FAILURE);
  }

  return f;
}

FILE *open_bin_file(char *src) {
  FILE *f;

  if (!(f = fopen(src, "rb"))) {
    printf("Error: %s file open failure", src);
    exit(EXIT_FAILURE);
  }

  return f;
}

F_str *get_txt_file_metadata(FILE *f, int &file_metadata_len) {
  file_metadata_len = 0;
  F_str *file_metadata = new F_str[FILE_STRS_MAX_NUM];

  int str_len = 0;
  int offset = 0;
  fseek(f, offset, SEEK_SET);

  while (1) {
    char c = fgetc(f);

    if (c != STR_DELIMITER && c != EOF) {
      str_len++;
      continue;
    }

    F_str &f_str = file_metadata[file_metadata_len++];
    f_str.f = f;
    f_str.f_kind = TXT;
    f_str.len = str_len;
    f_str.offset = offset;

    offset += str_len + 1;
    str_len = 0;

    if (c == EOF) break;
  }

  return file_metadata;
}

/**
 * Предпологается, что в конце строки бинарного файла отсутствует символ
 * разделения строк и длина строки считается без этого символа.
 */
F_str *get_bin_file_metadata(FILE *f, int &file_metadata_len) {
  file_metadata_len = 0;
  F_str *file_metadata = new F_str[FILE_STRS_MAX_NUM];

  int offset = 0, len = 0;
  fseek(f, offset, SEEK_SET);

  while (fread(&len, sizeof(int), 1, f)) {
    F_str &f_str = file_metadata[file_metadata_len++];
    f_str.f = f;
    f_str.f_kind = BIN;
    f_str.len = len;
    f_str.offset = offset + sizeof(int);

    offset += sizeof(int) + len * sizeof(char);
    fseek(f, offset, SEEK_SET);
  }

  return file_metadata;
}

char *read_file_str(F_str &f_str) {
  char *str = new char[f_str.len + 1];

  fseek(f_str.f, f_str.offset, SEEK_SET);

  switch (f_str.f_kind) {
  case BIN:
    fread(str, sizeof(char), f_str.len, f_str.f);
    break;
  case TXT:
    fgets(str, f_str.len + 1, f_str.f);
    break;
  }

  str[f_str.len] = '\0';

  return str;
}

int main() {
  FILE *f = open_bin_file("bar.bin");
  int file_metadata_len;
  F_str *file_metadata = get_bin_file_metadata(f, file_metadata_len);
  
  for (int i = 0; i < file_metadata_len; i++) {
    char *str = read_file_str(file_metadata[i]);
    printf("%s\n", str);
  }

  return 0;
}
