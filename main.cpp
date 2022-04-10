#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Максимальная длина строки
const int STR_MAX_LEN = 1000;

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
struct File_string {
    File_string();

    File_string(const char *, int, int);

    const char *path;
    int offset;
    int len;
};

// Структура, хранящая массив данных о строках и длину массива. Т.е. метаданные файлов.
struct File_metadata {
    File_metadata(int = 0);

    ~File_metadata();

    File_string *strings;
    int len;
};

// Добавление данных о строке в файловые метаданные.
void push_string(File_metadata *, File_string);

// Объединение файловых метаданных.
void concat_files_metadata(File_metadata *, File_metadata *);

// Получение матаданных бинарного файла.
File_metadata *get_bin_file_metadata(FILE *, const char *);

// Получение матаданных текстового файла.
File_metadata *get_txt_file_metadata(FILE *, const char *);

// Прочтение строки из файла, используя данные о строке.
char *read_string(FILE *, File_string *);

// Сортировка метаданных методом слияния.
void sort_metadata_by_merge(File_metadata *,
                            bool, int, int);

// Сортировка по убыванию.
void sort_by_descending(File_metadata *);

// Сортировка по возрастанию.
void sort_by_ascending(File_metadata *);

// Сортировка строк текстовых файлов с выводом в один файл.
void sort_files_strings(TXT_FILE_SIGNATURE, const char *,
                        void (*)(File_metadata *), ...);

// Сортировка строк бинарных файлов с выводом в один файл.
void sort_files_strings(BIN_FILE_SIGNATURE _, const char *,
                        void (*)(File_metadata *), ...);

// Вывод бинарного файла. Данная функция успользуется для тестирования.
void print_bin_file(const char *);

// Создания бинарного файла из текстового. Данная функция успользуется для тестирования.
void create_bin_file_from(const char *, const char *);

int main() {
    sort_files_strings(TXT_FILE, "test_1_dest.txt", sort_by_ascending, "test_1_src_1.txt", "test_1_src_2.txt",
                       "test_1_src_3.txt", NULL);
    sort_files_strings(BIN_FILE, "test_1_dest.bin", sort_by_ascending, "test_1_src_1.bin", "test_1_src_2.bin",
                       "test_1_src_3.bin", NULL);

    sort_files_strings(TXT_FILE, "test_2_dest.txt", sort_by_descending, "test_2_src_1.txt", "test_2_src_2.txt",
                       "test_2_src_3.txt", NULL);
    sort_files_strings(BIN_FILE, "test_2_dest.bin", sort_by_descending, "test_2_src_1.bin", "test_2_src_2.bin",
                       "test_2_src_3.bin", NULL);

    sort_files_strings(TXT_FILE, "test_3_dest.txt", sort_by_descending, "test_3_src_1.txt", "test_3_src_2.txt", NULL);
    sort_files_strings(BIN_FILE, "test_3_dest.bin", sort_by_descending, "test_3_src_1.bin", "test_3_src_2.bin", NULL);

    return 0;
}

File_string::File_string() {
    path = "";
    offset = 0;
    len = 0;
}

File_string::File_string(const char *_path, int _offset, int _len) {
    path = _path;
    offset = _offset;
    len = _len;
}

File_metadata::File_metadata(int _len) {
    strings = new File_string[_len];
    len = _len;
}

File_metadata::~File_metadata() {
    if (len != 0)
        delete[] strings;
}

void push_string(File_metadata *metadata, File_string string) {
    File_string *new_strings = new File_string[metadata->len + 1];

    for (int i = 0; i < metadata->len; i++)
        new_strings[i] = metadata->strings[i];
    new_strings[metadata->len] = string;

    if (metadata->len != 0)
        delete[] metadata->strings;

    metadata->strings = new_strings;
    metadata->len += 1;
}

void concat_files_metadata(File_metadata *metadata, File_metadata *additional_metadata) {
    for (int i = 0; i < additional_metadata->len; i++)
        push_string(metadata, additional_metadata->strings[i]);

    delete additional_metadata;
}

File_metadata *get_bin_file_metadata(FILE *file, const char *src) {
    File_metadata *metadata = new File_metadata();

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

File_metadata *get_txt_file_metadata(FILE *file, const char *src) {
    File_metadata *metadata = new File_metadata();

    fseek(file, 0, SEEK_END);
    int file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    for (int offset = 0, len = 0; offset + len < file_len;) {
        char symbol = getc(file);

        if (symbol != STR_DELIMITER && offset + len < file_len - 1) {
            len++;
            continue;
        };

        if (symbol != STR_DELIMITER) {
            len++;
        }

        push_string(metadata, File_string(src, offset, len));

        offset += len + 2;
        len = 0;
    }

    return metadata;
}

char *read_string(FILE *file, File_string *file_string) {
    char *str = new char[file_string->len + 1];

    fseek(file, file_string->offset, SEEK_SET);
    fread(str, sizeof(char), file_string->len, file);

    str[file_string->len] = '\0';

    return str;
}

void sort_metadata_by_merge(File_metadata *metadata,
                            bool sort_by_ascending, int left_bound, int right_bound) {
    if (left_bound + 1 >= right_bound) return;

    int middle_bound = (left_bound + right_bound) / 2;

    // Тривиальный случай

    sort_metadata_by_merge(metadata, sort_by_ascending, left_bound, middle_bound);
    sort_metadata_by_merge(metadata, sort_by_ascending, middle_bound, right_bound);

    // Декомпозиция общего случая

    File_metadata *metadata_block = new File_metadata(right_bound - left_bound);

    int left_i = 0;
    int right_i = 0;

    while (left_bound + left_i < middle_bound &&
           middle_bound + right_i < right_bound) {

        if (!sort_by_ascending &&
            metadata->strings[left_bound + left_i].len > metadata->strings[middle_bound + right_i].len) {
            metadata_block->strings[left_i + right_i] =
                    metadata->strings[left_bound + left_i];
            left_i++;
        } else if (sort_by_ascending &&
                   metadata->strings[left_bound + left_i].len <= metadata->strings[middle_bound + right_i].len) {
            metadata_block->strings[left_i + right_i] =
                    metadata->strings[left_bound + left_i];
            left_i++;
        } else {
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

void sort_by_descending(File_metadata *metadata) {
    sort_metadata_by_merge(metadata, false, 0, metadata->len);
}

void sort_by_ascending(File_metadata *metadata) {
    sort_metadata_by_merge(metadata, true, 0, metadata->len);
}

void sort_files_strings(TXT_FILE_SIGNATURE _, const char *dest,
                        void (*sort_metadata)(File_metadata *), ...) {
    va_list args;
            va_start(args, sort_metadata);

    File_metadata *metadata = new File_metadata[1];

    const char *src;
    while ((src = va_arg(args, const char*)) != NULL) {
        FILE *file;
        if (fopen_s(&file, src, "r")) {
            printf("Ошибка! Не удалось открыть файл %s", src);
            exit(EXIT_FAILURE);
        }

        concat_files_metadata(metadata, get_txt_file_metadata(file, src));
        fclose(file);
    }

    sort_metadata(metadata);

    FILE *dest_file;
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

        char *str = read_string(file, &file_string);
        fwrite(str, sizeof(char), strlen(str), dest_file);
        if (i < metadata->len - 1) fwrite(&STR_DELIMITER, sizeof(char), 1, dest_file);

        delete[] str;
        fclose(file);
    }

            va_end(args);
    fclose(dest_file);
    delete[] metadata;
}

void sort_files_strings(BIN_FILE_SIGNATURE _, const char *dest,
                        void (*sort_metadata)(File_metadata *), ...) {
    va_list args;
            va_start(args, sort_metadata);

    File_metadata *metadata = new File_metadata[1];

    const char *src;
    while ((src = va_arg(args, const char*)) != NULL) {
        FILE *file;
        if (fopen_s(&file, src, "rb")) {
            printf("Ошибка! Не удалось открыть файл %s", src);
            exit(EXIT_FAILURE);
        }

        concat_files_metadata(metadata, get_bin_file_metadata(file, src));
        fclose(file);
    }

    sort_metadata(metadata);

    FILE *dest_file;
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

        char *str = read_string(file, &file_string);
        int str_len = strlen(str);

        fwrite(&str_len, sizeof(int), 1, dest_file);
        fwrite(str, sizeof(char), str_len, dest_file);

        delete[] str;
        fclose(file);
    }

            va_end(args);
    fclose(dest_file);
    delete[] metadata;
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


void create_bin_file_from(const char *dest_path, const char *src_path) {
    FILE *src;
    if (fopen_s(&src, src_path, "r")) {
        printf("Ошибка! Не удалось создать файл %s", src_path);
        exit(EXIT_FAILURE);
    }

    fseek(src, 0, SEEK_END);
    int src_len = ftell(src);
    fseek(src, 0, SEEK_SET);

    FILE *dest;
    if (fopen_s(&dest, dest_path, "wb")) {
        printf("Ошибка! Не удалось создать файл %s", dest_path);
        exit(EXIT_FAILURE);
    }

    fseek(src, 0, SEEK_SET);

    char str[STR_MAX_LEN];

    for (int offset = 0, len = 0; offset + len < src_len;) {
        char symbol = getc(src);

        if (symbol != STR_DELIMITER && offset + len < src_len - 1) {
            str[len] = symbol;
            len++;
            continue;
        };

        if (symbol != STR_DELIMITER) {
            str[len] = symbol;
            len++;
        }

        fwrite(&len, sizeof(int), 1, dest);
        fwrite(str, sizeof(char), len, dest);

        offset += len + 2;
        len = 0;
    }

    fclose(src);
    fclose(dest);
}
