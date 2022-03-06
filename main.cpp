#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

const char STR_DELIMITER = '\n';

struct TXT_FILE_SIGNATURE {
};
const TXT_FILE_SIGNATURE TXT_FILE = TXT_FILE_SIGNATURE();

struct BIN_FILE_SIGNATURE {
};
const BIN_FILE_SIGNATURE BIN_FILE = BIN_FILE_SIGNATURE();

struct FileString;

struct FilesMetadata;

void pushString(FilesMetadata *, FileString);

void concatFileMetadata(FilesMetadata *, FilesMetadata *);

FILE *createBinFile(const char *);

FILE *openBinFile(const char *);

FilesMetadata *getBinFileMetadata(FILE *);

char *readStringFromBin(FileString *);

void writeStringToBin(FILE *, char *);

FILE *createTxtFile(const char *);

FILE *openTxtFile(const char *);

FilesMetadata *getTxtFileMetadata(FILE *);

char *readStringFromTxt(FileString *);

void writeStringToTxt(FILE *, char *);

void sortMetadataByMerge(FilesMetadata *,
                                   bool (*)(FileString *,
                                            FileString *));

bool isLess(FileString *, FileString *);

bool isGreater(FileString *, FileString *);

void sortByDescending(FilesMetadata *);

void sortByAscending(FilesMetadata *);

void sortFilesStrings(TXT_FILE_SIGNATURE, const char *,
                      void (*)(FilesMetadata *), ...);

void sortFilesStrings(BIN_FILE_SIGNATURE _, const char *,
                      void (*)(FilesMetadata *), ...);

void buildTxtTestData(const char *, ...);

void buildBinTestData(const char *, ...);

void testTxt();

void testBin();

void test();

int main() {
    test();
    return 0;
}

struct FileString {
    FILE *file;
    int offset;
    int len;

    FileString() {}

    FileString(FILE *_file, int _offset, int _len) {
        file = _file;
        offset = _offset;
        len = _len;
    }
};

struct FilesMetadata {
    FileString *strings;
    int len;

    FilesMetadata(int _len = 0) {
        strings = new FileString[_len];
        len = _len;
    }

    ~FilesMetadata() {
        if (len != 0)
            delete[] strings;
    }
};

void pushString(FilesMetadata *metadata, FileString string) {
    FileString *new_strings = new FileString[metadata->len + 1];

    for (int i = 0; i < metadata->len; i++)
        new_strings[i] = metadata->strings[i];
    new_strings[metadata->len] = string;

    if (metadata->len != 0)
        delete[] metadata->strings;

    metadata->strings = new_strings;
    metadata->len += 1;
}

void concatFileMetadata(FilesMetadata *metadata, FilesMetadata *additionMetadata) {
    for (int i = 0; i < additionMetadata->len; i++)
        pushString(metadata, additionMetadata->strings[i]);

    delete additionMetadata;
}

FILE *createBinFile(const char *src) {
    FILE *file;

    if (!(file = fopen(src, "wb"))) {
        printf("Error: %s file creating failure", src);
        exit(EXIT_FAILURE);
    }

    return file;
}

FILE *openBinFile(const char *src) {
    FILE *file;

    if (!(file = fopen(src, "rb"))) {
        printf("Error: %s file opening failure", src);
        exit(EXIT_FAILURE);
    }

    return file;
}

/**
 * Предпологается, что в конце строки бинарного файла отсутствует символ
 * разделения строк и длина строки считается без этого символа.
 */
FilesMetadata *getBinFileMetadata(FILE *file) {
    FilesMetadata *metadata = new FilesMetadata();

    int stringOffset = 0, stringLen = 0;
    fseek(file, stringOffset, SEEK_SET);

    while (fread(&stringLen, sizeof(int), 1, file)) {
        pushString(metadata,
                   FileString(file, stringOffset + sizeof(int), stringLen));

        stringOffset += sizeof(int) + stringLen * sizeof(char);
        fseek(file, stringOffset, SEEK_SET);
    }

    return metadata;
}

char *readStringFromBin(FileString *fileString) {
    char *str = new char[fileString->len + 1];

    fseek(fileString->file, fileString->offset, SEEK_SET);
    fread(str, sizeof(char), fileString->len, fileString->file);

    str[fileString->len] = '\0';

    return str;
}

void writeStringToBin(FILE *file, char *str) {
    int len = strlen(str);
    fwrite(&len, sizeof(int), 1, file);
    fwrite(str, sizeof(char), len, file);
}

FILE *createTxtFile(const char *src) {
    FILE *file;

    if (!(file = fopen(src, "w"))) {
        printf("Error: %s file creating failure", src);
        exit(EXIT_FAILURE);
    }

    return file;
}

FILE *openTxtFile(const char *src) {
    FILE *file;

    if (!(file = fopen(src, "r"))) {
        printf("Error: %s file opening failure", src);
        exit(EXIT_FAILURE);
    }

    return file;
}

FilesMetadata *getTxtFileMetadata(FILE *file) {
    FilesMetadata *metadata = new FilesMetadata();

    int stringLen = 0, stringOffset = 0;
    fseek(file, 0, SEEK_SET);

    while (true) {
        char symbol = fgetc(file);

        if (symbol != STR_DELIMITER && symbol != EOF) {
            stringLen++;
            continue;
        }

        if (stringLen == 0) break;

        pushString(metadata, FileString(file, stringOffset, stringLen));

        stringOffset += stringLen + 1;
        stringLen = 0;

        if (symbol == EOF)
            break;
    }

    return metadata;
}

char *readStringFromTxt(FileString *fileString) {
    char *str = new char[fileString->len + 1];

    fseek(fileString->file, fileString->offset, SEEK_SET);
    fgets(str, fileString->len + 1, fileString->file);

    str[fileString->len] = '\0';

    return str;
}

void writeStringToTxt(FILE *destFile, char *str) {
    fputs(str, destFile);
    fputc('\n', destFile);
}

void sortMetadataByMerge(FilesMetadata *metadata,
                                   bool (*predicate)(FileString *,
                                                     FileString *)) {
    int blockSize, blockStartPos, leftBorder, middleBorder, rightBorder,
            leftBlockIter, rightBlockIter;

    for (blockSize = 1; blockSize < metadata->len; blockSize *= 2) {
        for (blockStartPos = 0; blockStartPos < metadata->len - blockSize;
             blockStartPos += 2 * blockSize) {
            leftBlockIter = 0;
            rightBlockIter = 0;

            leftBorder = blockStartPos;
            middleBorder = blockStartPos + blockSize;
            rightBorder = blockStartPos + 2 * blockSize;
            rightBorder = rightBorder < metadata->len ? rightBorder : metadata->len;

            FilesMetadata *metadataBlock = new FilesMetadata(2 * blockSize);

            while (leftBorder + leftBlockIter < middleBorder &&
                   middleBorder + rightBlockIter < rightBorder) {
                if (predicate(&metadata->strings[leftBorder + leftBlockIter],
                              &metadata->strings[middleBorder + rightBlockIter])) {
                    metadataBlock->strings[leftBlockIter + rightBlockIter] =
                            metadata->strings[leftBorder + leftBlockIter];
                    leftBlockIter++;
                } else {
                    metadataBlock->strings[leftBlockIter + rightBlockIter] =
                            metadata->strings[middleBorder + rightBlockIter];
                    rightBlockIter++;
                }
            }

            while (leftBorder + leftBlockIter < middleBorder) {
                metadataBlock->strings[leftBlockIter + rightBlockIter] =
                        metadata->strings[leftBorder + leftBlockIter];
                leftBlockIter++;
            }

            while (middleBorder + rightBlockIter < rightBorder) {
                metadataBlock->strings[leftBlockIter + rightBlockIter] =
                        metadata->strings[middleBorder + rightBlockIter];
                rightBlockIter++;
            }

            for (int mergeIter = 0; mergeIter < leftBlockIter + rightBlockIter; mergeIter++) {
                metadata->strings[leftBorder + mergeIter] = metadataBlock->strings[mergeIter];
            }

            delete metadataBlock;
        }
    }
}

bool isLess(FileString *string_a, FileString *string_b) {
    return string_a->len < string_b->len;
}

bool isGreater(FileString *string_a, FileString *string_b) {
    return string_a->len > string_b->len;
}

void sortByDescending(FilesMetadata *metadata) {
    sortMetadataByMerge(metadata, isGreater);
}

void sortByAscending(FilesMetadata *metadata) {
    sortMetadataByMerge(metadata, isLess);
}

void sortFilesStrings(TXT_FILE_SIGNATURE _, const char *dest,
                      void (*sortMetadata)(FilesMetadata *), ...) {
    va_list vaList;
    va_start(vaList, sortMetadata);

    FILE *srcFiles[100];
    int srcFilesNum = 0;
    FilesMetadata *metadata = new FilesMetadata();

    const char *src;
    while ((src = va_arg(vaList, const char *)) != NULL) {
        srcFiles[srcFilesNum] = openTxtFile(src);
        concatFileMetadata(metadata, getTxtFileMetadata(srcFiles[srcFilesNum]));
        srcFilesNum++;
    }

    sortMetadata(metadata);
    FILE *destFile = createTxtFile(dest);

    for (int i = 0; i < metadata->len; i++) {
        char *str = readStringFromTxt(&metadata->strings[i]);
        writeStringToTxt(destFile, str);
    }

    fclose(destFile);
    for (int i = 0; i < srcFilesNum; i++)
        fclose(srcFiles[i]);

    delete metadata;

    va_end(vaList);
}

void sortFilesStrings(BIN_FILE_SIGNATURE _, const char *dest,
                      void (*sortMetadata)(FilesMetadata *), ...) {
    va_list vaList;
    va_start(vaList, sortMetadata);

    FILE *srcFiles[100];
    int srcFilesNum = 0;
    FilesMetadata *metadata = new FilesMetadata();

    const char *src;
    while ((src = va_arg(vaList, const char *)) != NULL) {
        srcFiles[srcFilesNum] = openBinFile(src);
        concatFileMetadata(metadata, getBinFileMetadata(srcFiles[srcFilesNum]));
        srcFilesNum++;
    }

    sortMetadata(metadata);
    FILE *destFile = createBinFile(dest);

    for (int i = 0; i < metadata->len; i++) {
        char *str = readStringFromBin(&metadata->strings[i]);
        writeStringToBin(destFile, str);
    }

    fclose(destFile);
    for (int i = 0; i < srcFilesNum; i++)
        fclose(srcFiles[i]);

    delete metadata;

    va_end(vaList);
}

void buildTxtTestData(const char *dist, ...) {
    va_list vaList;
    va_start(vaList, dist);

    FILE *file = createTxtFile(dist);
    char *str;
    while ((str = va_arg(vaList, char *)) != NULL) {
        writeStringToTxt(file, str);
    }

    fclose(file);
    va_end(vaList);
}

void buildBinTestData(const char *dist, ...) {
    va_list vaList;
    va_start(vaList, dist);

    FILE *file = createBinFile(dist);
    char *str;
    while ((str = va_arg(vaList, char *)) != NULL) {
        writeStringToBin(file, str);
    }

    fclose(file);
    va_end(vaList);
}

void testTxt() {
    buildTxtTestData("tests/txt/test1_1_1.txt", "ssss", "aaaa", "dddd", "bbbb", "0000", NULL);
    sortFilesStrings(TXT_FILE, "tests/txt/output1_1.txt", sortByDescending, "tests/txt/test1_1_1.txt", NULL);

    buildTxtTestData("tests/txt/test1_2_1.txt", "dfdwebhbw", "nb ewf", "no; o23h  9dsdfnq", "3sq<", "o", NULL);
    buildTxtTestData("tests/txt/test1_2_2.txt", "werwb", "023 sdfaaa", "q", ",.", "3f", NULL);
    buildTxtTestData("tests/txt/test1_2_3.txt", "sdasdff", "8q", "werinwi ai", "[[[hwe323(()", NULL);
    sortFilesStrings(TXT_FILE, "tests/txt/output1_2.txt", sortByDescending, "tests/txt/test1_2_1.txt",
                     "tests/txt/test1_2_2.txt", "tests/txt/test1_2_3.txt",
                     NULL);

    buildTxtTestData("tests/txt/test1_3_1.txt", "", NULL);
    buildTxtTestData("tests/txt/test1_3_2.txt", "sdf", "q", "werinwi ai", NULL);
    sortFilesStrings(TXT_FILE, "tests/txt/output1_3.txt", sortByDescending, "tests/txt/test1_3_1.txt",
                     "tests/txt/test1_3_2.txt", NULL);

    buildTxtTestData("tests/txt/test1_4_1.txt", "", NULL);
    buildTxtTestData("tests/txt/test1_4_2.txt", "", NULL);
    sortFilesStrings(TXT_FILE, "tests/txt/output1_4.txt", sortByDescending, "tests/txt/test1_4_1.txt",
                     "tests/txt/test1_4_2.txt", NULL);

    buildTxtTestData("tests/txt/test2_1_1.txt", "ssss", "aaaa", "dddd", "bbbb", "0000", NULL);
    sortFilesStrings(TXT_FILE, "tests/txt/output2_1.txt", sortByAscending, "tests/txt/test2_1_1.txt", NULL);

    buildTxtTestData("tests/txt/test2_2_1.txt", "dfdwebhbw", "nb ewf", "no; o23h  9dsdfnq", "3sq<", "o", NULL);
    buildTxtTestData("tests/txt/test2_2_2.txt", "werwb", "023 sdfaaa", "q", ",.", "3f", NULL);
    buildTxtTestData("tests/txt/test2_2_3.txt", "sdasdff", "8q", "werinwi ai", "[[[hwe323(()", NULL);
    sortFilesStrings(TXT_FILE, "tests/txt/output2_2.txt", sortByAscending, "tests/txt/test2_2_1.txt",
                     "tests/txt/test2_2_2.txt", "tests/txt/test2_2_3.txt",
                     NULL);

    buildTxtTestData("tests/txt/test2_3_1.txt", "", NULL);
    buildTxtTestData("tests/txt/test2_3_2.txt", "sdf", "q", "werinwi ai", NULL);
    sortFilesStrings(TXT_FILE, "tests/txt/output2_3.txt", sortByAscending, "tests/txt/test2_3_1.txt",
                     "tests/txt/test2_3_2.txt", NULL);

    buildTxtTestData("tests/txt/test2_4_1.txt", "", NULL);
    buildTxtTestData("tests/txt/test2_4_2.txt", "", NULL);
    sortFilesStrings(TXT_FILE, "tests/txt/output2_4.txt", sortByAscending, "tests/txt/test2_4_1.txt",
                     "tests/txt/test2_4_2.txt", NULL);
}

void testBin() {
    buildBinTestData("tests/bin/test1_1_1.bin", "ssss", "aaaa", "dddd", "bbbb", "0000", NULL);
    sortFilesStrings(BIN_FILE, "tests/bin/output1_1.bin", sortByDescending, "tests/bin/test1_1_1.bin", NULL);

    buildBinTestData("tests/bin/test1_2_1.bin", "dfdwebhbw", "nb ewf", "no; o23h  9dsdfnq", "3sq<", "o", NULL);
    buildBinTestData("tests/bin/test1_2_2.bin", "werwb", "023 sdfaaa", "q", ",.", "3f", NULL);
    buildBinTestData("tests/bin/test1_2_3.bin", "sdasdff", "8q", "werinwi ai", "[[[hwe323(()", NULL);
    sortFilesStrings(BIN_FILE, "tests/bin/output1_2.bin", sortByDescending, "tests/bin/test1_2_1.bin",
                     "tests/bin/test1_2_2.bin", "tests/bin/test1_2_3.bin",
                     NULL);

    buildBinTestData("tests/bin/test1_3_1.bin", "", NULL);
    buildBinTestData("tests/bin/test1_3_2.bin", "sdf", "q", "werinwi ai", NULL);
    sortFilesStrings(BIN_FILE, "tests/bin/output1_3.bin", sortByDescending, "tests/bin/test1_3_1.bin",
                     "tests/bin/test1_3_2.bin", NULL);

    buildBinTestData("tests/bin/test1_4_1.bin", "", NULL);
    buildBinTestData("tests/bin/test1_4_2.bin", "", NULL);
    sortFilesStrings(BIN_FILE, "tests/bin/output1_4.bin", sortByDescending, "tests/bin/test1_4_1.bin",
                     "tests/bin/test1_4_2.bin", NULL);

    buildBinTestData("tests/bin/test2_1_1.bin", "ssss", "aaaa", "dddd", "bbbb", "0000", NULL);
    sortFilesStrings(BIN_FILE, "tests/bin/output2_1.bin", sortByAscending, "tests/bin/test2_1_1.bin", NULL);

    buildBinTestData("tests/bin/test2_2_1.bin", "dfdwebhbw", "nb ewf", "no; o23h  9dsdfnq", "3sq<", "o", NULL);
    buildBinTestData("tests/bin/test2_2_2.bin", "werwb", "023 sdfaaa", "q", ",.", "3f", NULL);
    buildBinTestData("tests/bin/test2_2_3.bin", "sdasdff", "8q", "werinwi ai", "[[[hwe323(()", NULL);
    sortFilesStrings(BIN_FILE, "tests/bin/output2_2.bin", sortByAscending, "tests/bin/test2_2_1.bin",
                     "tests/bin/test2_2_2.bin", "tests/bin/test2_2_3.bin",
                     NULL);

    buildBinTestData("tests/bin/test2_3_1.bin", "", NULL);
    buildBinTestData("tests/bin/test2_3_2.bin", "sdf", "q", "werinwi ai", NULL);
    sortFilesStrings(BIN_FILE, "tests/bin/output2_3.bin", sortByAscending, "tests/bin/test2_3_1.bin",
                     "tests/bin/test2_3_2.bin", NULL);

    buildBinTestData("tests/bin/test2_4_1.bin", "", NULL);
    buildBinTestData("tests/bin/test2_4_2.bin", "", NULL);
    sortFilesStrings(BIN_FILE, "tests/bin/output2_4.bin", sortByAscending, "tests/bin/test2_4_1.bin",
                     "tests/bin/test2_4_2.bin", NULL);
}

void test() {
    testTxt();
    testBin();
    /*
                    testX_X_X.xxx
                        ^ ^ ^
                        | | |
                        | | +--------------------------------+
                        | +----------------------+           |
    1: sort by descending; 2: sort by ascending; |           |
                                             Number of test; |
                                                         Number of test file;
     */
}
