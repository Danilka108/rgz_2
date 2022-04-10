#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

// Разделитель строк
const char STR_DELIMITER = '\n';

// Тип для фиктивного аргумента функции. Необходим для перегрузки.
// Сигнатура текстого файла.
struct TXT_FILE_SIGNATURE {
};
const TXT_FILE_SIGNATURE TXT_FILE = TXT_FILE_SIGNATURE();

// Тип для фиктивного аргумента функции. Необходим для перегрузки.
// Сигнатура бинарного файла.
struct BIN_FILE_SIGNATURE {
};
const BIN_FILE_SIGNATURE BIN_FILE = BIN_FILE_SIGNATURE();

// Структура, хранящая длину строки, ее сдвиг в файле и название файла, к которому принадлежит строка. Т.е. метаданные строки файла.
struct File_string;

// Структура, хранящая массив данных о строках и длину массива. Т.е. метаданные файлов.
struct File_metadata;

// Добавление данных о строке в файловые метаданные.
void push_string(File_metadata*, File_string);

// Объединение файловых метаданных.
void concat_files_metadata(File_metadata*, File_metadata*);

// Получение матаданных бинарного файла.
File_metadata* get_bin_file_metadata(FILE*, const char *);

// Получение матаданных текстового файла.
File_metadata* get_txt_file_metadata(FILE*, const char*);

// Вывод бинарного файла для тестирования.
void print_bin_file(const char *);

// Прочтение строки из файла, используя данные о строке.
char* read_string(FILE *,File_string*);

// Сортировка метаданных методом слияния.
void sort_metadata_by_merge(File_metadata*,
                            bool, int, int);

// Сортировка по убыванию.
void sort_by_descending(File_metadata*);

// Сортировка по возрастанию.
void sort_by_ascending(File_metadata*);

// Сортировка строк текстовых файлов с выводом в один файл.
void sort_files_strings(TXT_FILE_SIGNATURE, const char*,
                      void (*)(File_metadata*), ...);

// Сортировка строк бинарных файлов с выводом в один файл.
void sort_files_strings(BIN_FILE_SIGNATURE _, const char*,
                      void (*)(File_metadata*), ...);

// Не добавлять данную функцию в отчет.
void build_txt_test_data(const char*, ...);

// Не добавлять данную функцию в отчет.
void build_bin_test_data(const char*, ...);

// Не добавлять данную функцию в отчет.
void test_txt();

// Не добавлять данную функцию в отчет.
void test_bin();

// Не добавлять данную функцию в отчет.
void test();

int main() {
    test();
    return 0;
}

struct File_string {
    const char *path;
    int offset;
    int len;

    File_string() {}

    File_string(const char *_path, int _offset, int _len) {
        path = _path;
        offset = _offset;
        len = _len;
    }
};

struct File_metadata {
    File_string* strings;
    int len;

    File_metadata(int _len = 0) {
        strings = new File_string[_len];
        len = _len;
    }

    ~File_metadata() {
        if (len != 0)
            delete[] strings;
    }
};

void push_string(File_metadata* metadata, File_string string) {
    File_string* new_strings = new File_string[metadata->len + 1];

    for (int i = 0; i < metadata->len; i++)
        new_strings[i] = metadata->strings[i];
    new_strings[metadata->len] = string;

    if (metadata->len != 0)
        delete[] metadata->strings;

    metadata->strings = new_strings;
    metadata->len += 1;
}

void concat_files_metadata(File_metadata *metadata, File_metadata* additional_metadata) {
    for (int i = 0; i < additional_metadata->len; i++)
        push_string(metadata, additional_metadata->strings[i]);

    delete additional_metadata;
}

File_metadata* get_bin_file_metadata(FILE *file, const char *src) {
    File_metadata* metadata = new File_metadata();

    int offset = 0, len = 0;
    fseek(file, offset, SEEK_SET);

    while (fread(&len, sizeof(int), 1, file)) {
        push_string(metadata,
                   File_string(src, offset + sizeof(int), len));

        offset += sizeof(int) + len * sizeof(char);
        fseek(file, offset, SEEK_SET);
    }

    return metadata;
}

void print_bin_file(const char *path) {
    FILE *file;
    if (fopen_s(&file, path, "rb")) {
        printf("Ошибка! Не удалось открыть файл %s", path);
        exit(EXIT_FAILURE);
    }

    printf("Содержимое бинарного файла %s:\n", path);

    int len = 0;
    while (fread(&len, sizeof(int), 1, file)) {
        for (int i = 0; i < len; i++) printf("%c", fgetc(file));
        printf("\n");
    }

    fclose(file);
}

File_metadata* get_txt_file_metadata(FILE* file, const char *src) {
    File_metadata* metadata = new File_metadata();

    int len = 0, offset = 0;
    fseek(file, offset, SEEK_SET);

    while(true) {
        char symbol;
        fread(&symbol, sizeof(char), 1, file);

        if (symbol != STR_DELIMITER && symbol != EOF) {
            len++;
            continue;
        }

        if (len == 0) break;

        push_string(metadata, File_string(src, offset, len));

        offset += len + 2;
        len = 0;

        if (symbol == EOF)
            break;
    }

    return metadata;
}

char* read_string(FILE* file, File_string* file_string) {
    char* str = new char[file_string->len + 1];

    fseek(file, file_string->offset, SEEK_SET);
    fread(str, sizeof(char), file_string->len, file);

    str[file_string->len] = '\0';

    return str;
}

void sort_metadata_by_merge(File_metadata* metadata,
                         bool sort_by_ascending, int left_bound, int right_bound) {
    if (left_bound + 1 >= right_bound) return;

    int middle_bound = (left_bound + right_bound) / 2;

    // Тривиальный случай

    sort_metadata_by_merge(metadata, sort_by_ascending, left_bound, middle_bound);
    sort_metadata_by_merge(metadata, sort_by_ascending, middle_bound, right_bound);

    // Декомпозиция общего случая

    File_metadata* metadata_block = new File_metadata(right_bound - left_bound);

    int left_i = 0;
    int right_i = 0;

    while (left_bound + left_i < middle_bound &&
           middle_bound + right_i < right_bound) {

        if (!sort_by_ascending && metadata->strings[left_bound + left_i].len > metadata->strings[middle_bound + right_i].len) {
            metadata_block->strings[left_i + right_i] =
                    metadata->strings[left_bound + left_i];
            left_i++;
        }
        else if (sort_by_ascending && metadata->strings[left_bound + left_i].len < metadata->strings[middle_bound + right_i].len) {
            metadata_block->strings[left_i + right_i] =
                    metadata->strings[left_bound + left_i];
            left_i++;
        }
        else {
            metadata_block->strings[left_i + right_i] =
                    metadata->strings[middle_bound + right_i];
            right_i++;
        }
    }

    while (left_bound + left_i < middle_bound) {
        metadata_block->strings[left_i + right_i] =
                metadata->strings[left_bound + left_i];
        left_i++;
    }

    while (middle_bound + right_i < right_bound) {
        metadata_block->strings[left_i + right_i] =
                metadata->strings[middle_bound + right_i];
        right_i++;
    }

    for (int i = 0; i < left_i + right_i; i++) {
        metadata->strings[left_bound + i] = metadata_block->strings[i];
    }

    delete metadata_block;
}

void sort_by_descending(File_metadata* metadata) {
    sort_metadata_by_merge(metadata, false, 0, metadata->len);
}

void sort_by_ascending(File_metadata* metadata) {
    sort_metadata_by_merge(metadata, true, 0, metadata->len);
}

void sort_files_strings(TXT_FILE_SIGNATURE _, const char* dest,
                      void (*sort_metadata)(File_metadata*), ...) {
    va_list args;
    va_start(args, sort_metadata);

    File_metadata* metadata = new File_metadata[1];

    const char* src;
    while ((src = va_arg(args, const char*)) != NULL) {
        FILE *file;
        if (fopen_s(&file, src, "r")) {
            printf("Ошибка! Не удалось открыть файл %s", src);
            exit(EXIT_FAILURE);
        }

        concat_files_metadata(metadata, get_txt_file_metadata(file, src));
    }

    sort_metadata(metadata);

    FILE* dest_file;
    if (fopen_s(&dest_file, dest, "w")) {
        printf("Ошибка! Не удалось создать файл %s", src);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < metadata->len; i++) {
        File_string file_string = metadata->strings[i];

        FILE *file;
        if (fopen_s(&file, file_string.path, "r")) {
            printf("Ошибка! Не удалось открыть файл %s", src);
            exit(EXIT_FAILURE);
        }

        char* str = read_string(file, &file_string);
        fwrite(str, sizeof(char), strlen(str), dest_file);
        fwrite(&STR_DELIMITER, sizeof(char), 1, dest_file);
    }

    va_end(args);
    fclose(dest_file);
}

void sort_files_strings(BIN_FILE_SIGNATURE _, const char* dest,
                      void (*sort_metadata)(File_metadata*), ...) {
    va_list args;
    va_start(args, sort_metadata);

    File_metadata* metadata = new File_metadata[1];

    const char* src;
    while ((src = va_arg(args, const char*)) != NULL) {
        FILE *file;
        if (fopen_s(&file, src, "rb")) {
            printf("Ошибка! Не удалось открыть файл %s", src);
            exit(EXIT_FAILURE);
        }

        concat_files_metadata(metadata, get_bin_file_metadata(file, src));
    }

    sort_metadata(metadata);

    FILE* dest_file;
    if (fopen_s(&dest_file, dest, "wb")) {
        printf("Ошибка! Не удалось создать файл %s", src);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < metadata->len; i++) {
        File_string file_string = metadata->strings[i];

        FILE *file;
        if (fopen_s(&file, file_string.path, "rb")) {
            printf("Ошибка! Не удалось открыть файл %s", src);
            exit(EXIT_FAILURE);
        }

        char* str = read_string(file, &file_string);
        int str_len = strlen(str);

        fwrite(&str_len, sizeof(int), 1, dest_file);
        fwrite(str, sizeof(char), str_len, dest_file);
    }

    va_end(args);
    fclose(dest_file);
}

void build_txt_test_data(const char* dest, ...) {
    va_list args;
    va_start(args, dest);

    FILE *file;
    if (fopen_s(&file, dest, "w")) {
        printf("Ошибка! Не удалось создать файл %s", dest);
        exit(EXIT_FAILURE);
    }

    char* str;
    while ((str = va_arg(args, char*)) != NULL) {
        fwrite(str, sizeof(char), strlen(str), file);
        fwrite(&STR_DELIMITER, sizeof(char), 1, file);
    }

    va_end(args);
    fclose(file);
}

void build_bin_test_data(const char* dest, ...) {
    va_list vaList;
    va_start(vaList, dest);

    FILE *file;
    if (fopen_s(&file, dest, "wb")) {
        printf("Ошибка! Не удалось создать файл %s", dest);
        exit(EXIT_FAILURE);
    }

    char* str;
    while ((str = va_arg(vaList, char*)) != NULL) {
        int strLen = strlen(str);

        fwrite(&strLen, sizeof(int), 1, file);
        fwrite(str, sizeof(char), strLen, file);
    }

    fclose(file);
    va_end(vaList);
}

void test_txt() {
    build_txt_test_data("tests/txt/test1_1_1.txt", "ssss", "aaaa", "dddd", "bbbb", "0000", NULL);
    sort_files_strings(TXT_FILE, "tests/txt/output1_1.txt", sort_by_descending, "tests/txt/test1_1_1.txt", NULL);

    build_txt_test_data("tests/txt/test1_2_1.txt", "dfdwebhbw", "nb ewf", "no; o23h  9dsdfnq", "3sq<", "o", NULL);
    build_txt_test_data("tests/txt/test1_2_2.txt", "werwb", "023 sdfaaa", "q", ",.", "3f", NULL);
    build_txt_test_data("tests/txt/test1_2_3.txt", "sdasdff", "8q", "werinwi ai", "[[[hwe323(()", NULL);
    sort_files_strings(TXT_FILE, "tests/txt/output1_2.txt", sort_by_descending, "tests/txt/test1_2_1.txt",
        "tests/txt/test1_2_2.txt", "tests/txt/test1_2_3.txt",
        NULL);

    build_txt_test_data("tests/txt/test1_3_1.txt", "", NULL);
    build_txt_test_data("tests/txt/test1_3_2.txt", "sdf", "q", "werinwi ai", NULL);
    sort_files_strings(TXT_FILE, "tests/txt/output1_3.txt", sort_by_descending, "tests/txt/test1_3_1.txt",
        "tests/txt/test1_3_2.txt", NULL);

    build_txt_test_data("tests/txt/test1_4_1.txt", "", NULL);
    build_txt_test_data("tests/txt/test1_4_2.txt", "", NULL);
    sort_files_strings(TXT_FILE, "tests/txt/output1_4.txt", sort_by_descending, "tests/txt/test1_4_1.txt",
        "tests/txt/test1_4_2.txt", NULL);

    build_txt_test_data("tests/txt/test2_1_1.txt", "ssss", "aaaa", "dddd", "bbbb", "0000", NULL);
    sort_files_strings(TXT_FILE, "tests/txt/output2_1.txt", sort_by_ascending, "tests/txt/test2_1_1.txt", NULL);

    build_txt_test_data("tests/txt/test2_2_1.txt", "dfdwebhbw", "nb ewf", "no; o23h  9dsdfnq", "3sq<", "o", NULL);
    build_txt_test_data("tests/txt/test2_2_2.txt", "werwb", "023 sdfaaa", "q", ",.", "3f", NULL);
    build_txt_test_data("tests/txt/test2_2_3.txt", "sdasdff", "8q", "werinwi ai", "[[[hwe323(()", NULL);
    sort_files_strings(TXT_FILE, "tests/txt/output2_2.txt", sort_by_ascending, "tests/txt/test2_2_1.txt",
        "tests/txt/test2_2_2.txt", "tests/txt/test2_2_3.txt",
        NULL);

    build_txt_test_data("tests/txt/test2_3_1.txt", "", NULL);
    build_txt_test_data("tests/txt/test2_3_2.txt", "sdf", "q", "werinwi ai", NULL);
    sort_files_strings(TXT_FILE, "tests/txt/output2_3.txt", sort_by_ascending, "tests/txt/test2_3_1.txt",
        "tests/txt/test2_3_2.txt", NULL);

    build_txt_test_data("tests/txt/test2_4_1.txt", "", NULL);
    build_txt_test_data("tests/txt/test2_4_2.txt", "", NULL);
    sort_files_strings(TXT_FILE, "tests/txt/output2_4.txt", sort_by_ascending, "tests/txt/test2_4_1.txt",
        "tests/txt/test2_4_2.txt", NULL);
}

void test_bin() {
    build_bin_test_data("tests/bin/test1_1_1.bin", "ssss", "aaaa", "dddd", "bbbb", "0000", NULL);
    sort_files_strings(BIN_FILE, "tests/bin/output1_1.bin", sort_by_descending, "tests/bin/test1_1_1.bin", NULL);

    print_bin_file("tests/bin/output1_1.bin");

    build_bin_test_data("tests/bin/test1_2_1.bin", "dfdwebhbw", "nb ewf", "no; o23h  9dsdfnq", "3sq<", "o", NULL);
    build_bin_test_data("tests/bin/test1_2_2.bin", "werwb", "023 sdfaaa", "q", ",.", "3f", NULL);
    build_bin_test_data("tests/bin/test1_2_3.bin", "sdasdff", "8q", "werinwi ai", "[[[hwe323(()", NULL);
    sort_files_strings(BIN_FILE, "tests/bin/output1_2.bin", sort_by_descending, "tests/bin/test1_2_1.bin",
                     "tests/bin/test1_2_2.bin", "tests/bin/test1_2_3.bin",
                     NULL);

    print_bin_file("tests/bin/output1_2.bin");

    build_bin_test_data("tests/bin/test1_3_1.bin", "", NULL);
    build_bin_test_data("tests/bin/test1_3_2.bin", "sdf", "q", "werinwi ai", NULL);
    sort_files_strings(BIN_FILE, "tests/bin/output1_3.bin", sort_by_descending, "tests/bin/test1_3_1.bin",
                     "tests/bin/test1_3_2.bin", NULL);

    print_bin_file("tests/bin/output1_3.bin");

    build_bin_test_data("tests/bin/test1_4_1.bin", "", NULL);
    build_bin_test_data("tests/bin/test1_4_2.bin", "", NULL);
    sort_files_strings(BIN_FILE, "tests/bin/output1_4.bin", sort_by_descending, "tests/bin/test1_4_1.bin",
                     "tests/bin/test1_4_2.bin", NULL);

    print_bin_file("tests/bin/output1_4.bin");

    build_bin_test_data("tests/bin/test2_1_1.bin", "ssss", "aaaa", "dddd", "bbbb", "0000", NULL);
    sort_files_strings(BIN_FILE, "tests/bin/output2_1.bin", sort_by_ascending, "tests/bin/test2_1_1.bin", NULL);

    print_bin_file("tests/bin/output2_1.bin");

    build_bin_test_data("tests/bin/test2_2_1.bin", "dfdwebhbw", "nb ewf", "no; o23h  9dsdfnq", "3sq<", "o", NULL);
    build_bin_test_data("tests/bin/test2_2_2.bin", "werwb", "023 sdfaaa", "q", ",.", "3f", NULL);
    build_bin_test_data("tests/bin/test2_2_3.bin", "sdasdff", "8q", "werinwi ai", "[[[hwe323(()", NULL);
    sort_files_strings(BIN_FILE, "tests/bin/output2_2.bin", sort_by_ascending, "tests/bin/test2_2_1.bin",
                     "tests/bin/test2_2_2.bin", "tests/bin/test2_2_3.bin",
                     NULL);

    print_bin_file("tests/bin/output2_2.bin");

    build_bin_test_data("tests/bin/test2_3_1.bin", "", NULL);
    build_bin_test_data("tests/bin/test2_3_2.bin", "sdf", "q", "werinwi ai", NULL);
    sort_files_strings(BIN_FILE, "tests/bin/output2_3.bin", sort_by_ascending, "tests/bin/test2_3_1.bin",
                     "tests/bin/test2_3_2.bin", NULL);

    print_bin_file("tests/bin/output2_3.bin");

    build_bin_test_data("tests/bin/test2_4_1.bin", "", NULL);
    build_bin_test_data("tests/bin/test2_4_2.bin", "", NULL);
    sort_files_strings(BIN_FILE, "tests/bin/output2_4.bin", sort_by_ascending, "tests/bin/test2_4_1.bin",
                     "tests/bin/test2_4_2.bin", NULL);

    print_bin_file("tests/bin/output2_4.bin");
}

void test() {
    test_txt();
    test_bin();
}