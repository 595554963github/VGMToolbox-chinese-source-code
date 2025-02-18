#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdint.h>
#include <locale.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define BUFFER_SIZE 1024

// Function declarations
unsigned char* parse_start_sequence(const char* input, size_t* length);
unsigned char* parse_end_sequence(const char* input, size_t* length);
size_t get_end_index(const unsigned char* content, size_t content_length, size_t start_index, const unsigned char* end_sequence, size_t end_length, const unsigned char* start_sequence, size_t start_length);
size_t find_end_index(const unsigned char* content, size_t content_length, size_t start_index, const unsigned char* end_sequence, size_t end_length, size_t min_repeat_count, const unsigned char* start_sequence, size_t start_length);
int extract_content_normal(const char* file_path, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format);
int extract_content_repeat(const char* file_path, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format, size_t min_repeat_count);
int extract_before_address(const char* file_path, size_t target_index, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format, size_t min_repeat_count);
int extract_after_address(const char* file_path, size_t target_index, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format, size_t min_repeat_count);
int save_notes(const char* file_path, const char** notes, size_t note_count);
void get_extraction_parameters(unsigned char** start_sequence, size_t* start_length, unsigned char** end_sequence, size_t* end_length, int* use_repeat_method, size_t* min_repeat_count, char** output_format);
void process_directory(const char* directory_path, int extract_mode, size_t target_index, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format, int use_repeat_method, size_t min_repeat_count);

// 提取公共文件读取和内存分配逻辑到辅助函数
int read_file(const char* file_path, unsigned char** content, size_t* content_length) {
    FILE* f = fopen(file_path, "rb");
    if (f == NULL) {
        printf("无法读取文件 %s。\n", file_path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    *content_length = ftell(f);
    fseek(f, 0, SEEK_SET);
    *content = (unsigned char*)malloc(*content_length);
    if (*content == NULL) {
        fclose(f);
        printf("内存分配失败。\n");
        return -1;
    }
    fread(*content, 1, *content_length, f);
    fclose(f);
    return 0;
}

// 提取公共文件名生成逻辑到辅助函数
void generate_file_name(const char* file_path, const char* base_name, const char* output_format, size_t count, char* new_filename) {
    char* dot = strrchr(base_name, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    sprintf(new_filename, "%s_%zu.%s", base_name, count, output_format);
}

// 提取公共文件路径生成逻辑到辅助函数
void generate_file_path(const char* file_path, const char* new_filename, char* new_filepath) {
    strncpy(new_filepath, file_path, sizeof(new_filepath));
    new_filepath[sizeof(new_filepath) - 1] = '\0';
    char* last_slash = strrchr(new_filepath, '/');
    if (last_slash != NULL) {
        *(last_slash + 1) = '\0';
    }
    else {
        new_filepath[0] = '\0';
    }
    strcat(new_filepath, new_filename);
}

int main() {
#ifdef _WIN32
    // 设置控制台输出为UTF - 8
    SetConsoleOutputCP(65001);
#else
    // 非Windows系统设置locale为UTF - 8
    setlocale(LC_ALL, "");
#endif

    char directory_path[BUFFER_SIZE];
    printf("请输入要处理的文件夹路径: ");
    if (fgets(directory_path, sizeof(directory_path), stdin) == NULL) {
        fprintf(stderr, "输入错误。\n");
        return 1;
    }
    // Remove trailing newline
    directory_path[strcspn(directory_path, "\n")] = 0;

    // Check if directory exists
    struct stat st;
    if (stat(directory_path, &st) != 0 ||!S_ISDIR(st.st_mode)) {
        printf("错误: %s 不是一个有效的目录。\n", directory_path);
        return 1;
    }

    printf("请选择提取模式（1:正常提取，2:提取指定地址前的内容，3:提取指定地址后的内容）: ");
    char extract_mode_input[10];
    if (fgets(extract_mode_input, sizeof(extract_mode_input), stdin) == NULL) {
        fprintf(stderr, "输入错误。\n");
        return 1;
    }
    extract_mode_input[strcspn(extract_mode_input, "\n")] = 0;
    int extract_mode = atoi(extract_mode_input);

    size_t target_index = 0;
    unsigned char* start_sequence = NULL;
    size_t start_length = 0;
    unsigned char* end_sequence = NULL;
    size_t end_length = 0;
    char* output_format = NULL;
    int use_repeat_method = 0;
    size_t min_repeat_count = 0;

    if (extract_mode == 2 || extract_mode == 3) {
        char target_address_input[20];
        printf("请输入指定地址（例如: 0x00006F20）: ");
        if (fgets(target_address_input, sizeof(target_address_input), stdin) == NULL) {
            fprintf(stderr, "输入错误。\n");
            return 1;
        }
        target_address_input[strcspn(target_address_input, "\n")] = 0;
        target_index = (size_t)strtoul(target_address_input, NULL, 16);
    }

    get_extraction_parameters(&start_sequence, &start_length, &end_sequence, &end_length, &use_repeat_method, &min_repeat_count, &output_format);

    if (extract_mode == 2) {
        process_directory(directory_path, extract_mode, target_index, start_sequence, start_length, end_sequence, end_length, output_format, use_repeat_method, min_repeat_count);
    }
    else if (extract_mode == 3) {
        process_directory(directory_path, extract_mode, target_index, start_sequence, start_length, end_sequence, end_length, output_format, use_repeat_method, min_repeat_count);
    }
    else if (extract_mode == 1) {
        process_directory(directory_path, extract_mode, 0, start_sequence, start_length, end_sequence, end_length, output_format, use_repeat_method, min_repeat_count);
    }
    else {
        printf("无效的提取模式选择，请重新运行脚本并正确选择。\n");
        return 1;
    }

    // Free allocated memory
    free(start_sequence);
    free(end_sequence);
    free(output_format);

    return 0;
}

void get_extraction_parameters(unsigned char** start_sequence, size_t* start_length, unsigned char** end_sequence, size_t* end_length, int* use_repeat_method, size_t* min_repeat_count, char** output_format) {
    char start_sequence_input[256];
    printf("请输入起始序列的字节值，以空格分隔（也可输入类似00*16）: ");
    if (fgets(start_sequence_input, sizeof(start_sequence_input), stdin) == NULL) {
        fprintf(stderr, "输入错误。\n");
        exit(1);
    }
    start_sequence_input[strcspn(start_sequence_input, "\n")] = 0;
    *start_sequence = parse_start_sequence(start_sequence_input, start_length);
    if (*start_sequence == NULL) {
        fprintf(stderr, "无效的起始序列输入。\n");
        exit(1);
    }

    char end_sequence_input[256];
    printf("请输入结束序列字节值（以空格分割，使用*表示重复，如00*4，直接回车跳过）: ");
    if (fgets(end_sequence_input, sizeof(end_sequence_input), stdin) == NULL) {
        fprintf(stderr, "输入错误。\n");
        exit(1);
    }
    end_sequence_input[strcspn(end_sequence_input, "\n")] = 0;

    if (strlen(end_sequence_input) > 0) {
        *end_sequence = parse_end_sequence(end_sequence_input, end_length);
        if (*end_sequence == NULL) {
            fprintf(stderr, "无效的结束序列输入。\n");
            exit(1);
        }
        // Check for '*' in end_sequence_input and length ==1
        if (strchr(end_sequence_input, '*') == NULL && *end_length == 1) {
            char min_repeat_input[20];
            printf("请输入最小重复字节数量作为结束条件: ");
            if (fgets(min_repeat_input, sizeof(min_repeat_input), stdin) == NULL) {
                *min_repeat_count = 0;
            }
            else {
                *min_repeat_count = (size_t)atoi(min_repeat_input);
                if (*min_repeat_count > 0) {
                    *use_repeat_method = 1;
                }
            }
        }
    }
    else {
        *end_sequence = NULL;
        *end_length = 0;
    }

    char output_format_input[50];
    printf("请输入输出文件格式 (例如: bin): ");
    if (fgets(output_format_input, sizeof(output_format_input), stdin) == NULL) {
        fprintf(stderr, "输入错误。\n");
        exit(1);
    }
    output_format_input[strcspn(output_format_input, "\n")] = 0;
    *output_format = strdup(output_format_input);
}

unsigned char* parse_start_sequence(const char* input, size_t* length) {
    // Handle cases like "00*16" or space-separated bytes
    if (strchr(input, '*') != NULL) {
        char byte_str[3] = {0};
        char count_str[10] = {0};
        sscanf(input, "%2s*%9s", byte_str, count_str);
        unsigned char byte = (unsigned char)strtoul(byte_str, NULL, 16);
        size_t count = (size_t)atoi(count_str);
        unsigned char* result = (unsigned char*)malloc(count);
        if (result == NULL) return NULL;
        for (size_t i = 0; i < count; i++) {
            result[i] = byte;
        }
        *length = count;
        return result;
    }
    else {
        // Space-separated hex bytes
        size_t count = 0;
        for (size_t i = 0; input[i]; i++) {
            if (input[i] == ' ') count++;
        }
        count += 1;
        unsigned char* result = (unsigned char*)malloc(count);
        if (result == NULL) return NULL;
        size_t index = 0;
        char byte_str[3] = {0};
        const char* ptr = input;
        while (*ptr) {
            if (*ptr == ' ') {
                ptr++;
                continue;
            }
            strncpy(byte_str, ptr, 2);
            result[index++] = (unsigned char)strtoul(byte_str, NULL, 16);
            ptr += 2;
        }
        *length = index;
        return result;
    }
}

unsigned char* parse_end_sequence(const char* input, size_t* length) {
    // Handle multiple parts separated by space, each part can have '*'
    unsigned char* result = NULL;
    size_t result_size = 0;

    char input_copy[512];
    strncpy(input_copy, input, sizeof(input_copy));
    input_copy[sizeof(input_copy) - 1] = '\0';

    char* token = strtok(input_copy, " ");
    while (token != NULL) {
        unsigned char* part = NULL;
        size_t part_length = 0;
        if (strchr(token, '*') != NULL) {
            char byte_str[3] = {0};
            char count_str[10] = {0};
            sscanf(token, "%2s*%9s", byte_str, count_str);
            unsigned char byte = (unsigned char)strtoul(byte_str, NULL, 16);
            size_t count = (size_t)atoi(count_str);
            part = (unsigned char*)malloc(count);
            if (part == NULL) { free(result); return NULL; }
            for (size_t i = 0; i < count; i++) {
                part[i] = byte;
            }
            part_length = count;
        }
        else {
            char byte_str[3] = {0};
            strncpy(byte_str, token, 2);
            unsigned char byte = (unsigned char)strtoul(byte_str, NULL, 16);
            part = (unsigned char*)malloc(1);
            if (part == NULL) { free(result); return NULL; }
            part[0] = byte;
            part_length = 1;
        }

        // Append to result
        unsigned char* temp = realloc(result, result_size + part_length);
        if (temp == NULL) {
            free(part);
            free(result);
            return NULL;
        }
        result = temp;
        memcpy(result + result_size, part, part_length);
        result_size += part_length;
        free(part);

        token = strtok(NULL, " ");
    }

    *length = result_size;
    return result;
}

int extract_content_normal(const char* file_path, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format) {
    FILE* f = fopen(file_path, "rb");
    if (f == NULL) {
        printf("无法读取文件 %s。\n", file_path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size_t content_length = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* content = (unsigned char*)malloc(content_length);
    if (content == NULL) {
        fclose(f);
        printf("内存分配失败。\n");
        return -1;
    }
    fread(content, 1, content_length, f);
    fclose(f);

    size_t count = 0;
    size_t start_index = 0;
    const char* notes[1000];
    size_t note_count = 0;

    while (start_index < content_length) {
        // Find start_sequence
        size_t found = start_index;
        while (found <= content_length - start_length) {
            if (memcmp(content + found, start_sequence, start_length) == 0) {
                break;
            }
            found++;
        }
        if (found > content_length - start_length) {
            printf("No more start sequences found in %s\n", file_path);
            break;
        }
        start_index = found;

        // Get end_index
        size_t end_index = get_end_index(content, content_length, start_index, end_sequence, end_length, start_sequence, start_length);

        // Extract data
        size_t extracted_size = end_index - start_index;
        unsigned char* extracted_data = (unsigned char*)malloc(extracted_size);
        if (extracted_data == NULL) {
            printf("内存分配失败。\n");
            free(content);
            return -1;
        }
        memcpy(extracted_data, content + start_index, extracted_size);

        // Create new filename
        char new_filename[1024];
        char base_name[512];
        const char* slash = strrchr(file_path, '/');
        if (slash != NULL) {
            strcpy(base_name, slash + 1);
        }
        else {
            strcpy(base_name, file_path);
        }
        char* dot = strrchr(base_name, '.');
        if (dot != NULL) {
            *dot = '\0';
        }
        sprintf(new_filename, "%s_%zu.%s", base_name, count, output_format);

        // Create new filepath
        char new_filepath[2048];
        strncpy(new_filepath, file_path, sizeof(new_filepath));
        new_filepath[sizeof(new_filepath)-1] = '\0';
        char* last_slash = strrchr(new_filepath, '/');
        if (last_slash != NULL) {
            *(last_slash + 1) = '\0';
        }
        else {
            new_filepath[0] = '\0';
        }
        strcat(new_filepath, new_filename);

        // Write to new file
        FILE* new_file = fopen(new_filepath, "wb");
        if (new_file == NULL) {
            printf("无法写入文件 %s。\n", new_filepath);
            free(extracted_data);
            start_index = end_index;
            continue;
        }
        fwrite(extracted_data, 1, extracted_size, new_file);
        fclose(new_file);
        printf("Extracted content saved as: %s\n", new_filepath);

        // Add note
        char note[1024];
        sprintf(note, "File: %s, Start Address: %zu, End Address: %zu", new_filepath, start_index, end_index);
        notes[note_count++] = strdup(note);

        count++;
        free(extracted_data);
        start_index = end_index;
    }

    // Save notes
    save_notes(file_path, (const char**)notes, note_count);

    // Free notes
    for (size_t i = 0; i < note_count; i++) {
        free((void*)notes[i]);
    }

    free(content);
    return 0;
}

int extract_content_repeat(const char* file_path, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format, size_t min_repeat_count) {
    // Similar to extract_content_normal but uses find_end_index
    FILE* f = fopen(file_path, "rb");
    if (f == NULL) {
        printf("无法读取文件 %s。\n", file_path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size_t content_length = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* content = (unsigned char*)malloc(content_length);
    if (content == NULL) {
        fclose(f);
        printf("内存分配失败。\n");
        return -1;
    }
    fread(content, 1, content_length, f);
    fclose(f);

    size_t count = 0;
    size_t start_index = 0;
    const char* notes[1000];
    size_t note_count = 0;

    while (start_index < content_length) {
        // Find start_sequence
        size_t found = start_index;
        while (found <= content_length - start_length) {
            if (memcmp(content + found, start_sequence, start_length) == 0) {
                break;
            }
            found++;
        }
        if (found > content_length - start_length) {
            printf("No more start sequences found in %s\n", file_path);
            break;
        }
        start_index = found;

        // Get end_index using find_end_index
        size_t end_index = find_end_index(content, content_length, start_index, end_sequence, end_length, min_repeat_count, start_sequence, start_length);

        // Extract data
        size_t extracted_size = end_index - start_index;
        unsigned char* extracted_data = (unsigned char*)malloc(extracted_size);
        if (extracted_data == NULL) {
            printf("内存分配失败。\n");
            free(content);
            return -1;
        }
        memcpy(extracted_data, content + start_index, extracted_size);

        // Create new filename
        char new_filename[1024];
        char base_name[512];
        const char* slash = strrchr(file_path, '/');
        if (slash != NULL) {
            strcpy(base_name, slash + 1);
        }
        else {
            strcpy(base_name, file_path);
        }
        char* dot = strrchr(base_name, '.');
        if (dot != NULL) {
            *dot = '\0';
        }
        sprintf(new_filename, "%s_%zu.%s", base_name, count, output_format);

        // Create new filepath
        char new_filepath[2048];
        strncpy(new_filepath, file_path, sizeof(new_filepath));
        new_filepath[sizeof(new_filepath)-1] = '\0';
        char* last_slash = strrchr(new_filepath, '/');
        if (last_slash != NULL) {
            *(last_slash + 1) = '\0';
        }
        else {
            new_filepath[0] = '\0';
        }
        strcat(new_filepath, new_filename);

        // Write to new file
        FILE* new_file = fopen(new_filepath, "wb");
        if (new_file == NULL) {
            printf("无法写入文件 %s。\n", new_filepath);
            free(extracted_data);
            start_index = end_index;
            continue;
        }
        fwrite(extracted_data, 1, extracted_size, new_file);
        fclose(new_file);
        printf("Extracted content saved as: %s\n", new_filepath);

        // Add note
        char note[1024];
        sprintf(note, "File: %s, Start Address: %zu, End Address: %zu", new_filepath, start_index, end_index);
        notes[note_count++] = strdup(note);

        count++;
        free(extracted_data);
        start_index = end_index;
    }

    // Save notes
    save_notes(file_path, (const char**)notes, note_count);

    // Free notes
    for (size_t i = 0; i < note_count; i++) {
        free((void*)notes[i]);
    }

    free(content);
    return 0;
}

int extract_before_address(const char* file_path, size_t target_index, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format, size_t min_repeat_count) {
    if (target_index > (size_t)-1) {
        // Prevent overflow
    }
    FILE* f = fopen(file_path, "rb");
    if (f == NULL) {
        printf("无法读取文件 %s。\n", file_path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size_t content_length = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* content = (unsigned char*)malloc(content_length);
    if (content == NULL) {
        fclose(f);
        printf("内存分配失败。\n");
        return -1;
    }
    fread(content, 1, content_length, f);
    fclose(f);

    if (target_index > content_length) {
        printf("指定地址 超出文件范围，无法提取。\n");
        free(content);
        return -1;
    }

    size_t count = 0;
    size_t start_index = 0;
    const char* notes[1000];
    size_t note_count = 0;

    while (start_index < content_length && start_index < target_index) {
        // Find start_sequence
        size_t found = start_index;
        while (found <= content_length - start_length && found < target_index) {
            if (memcmp(content + found, start_sequence, start_length) == 0) {
                break;
            }
            found++;
        }
        if (found > content_length - start_length || found >= target_index) {
            printf("No more start sequences found in %s before the target address\n", file_path);
            break;
        }
        start_index = found;

        // Get end_index
        size_t end_index = get_end_index(content, content_length, start_index, end_sequence, end_length, start_sequence, start_length);
        if (end_index > target_index) {
            end_index = target_index;
        }

        // Extract data
        size_t extracted_size = end_index - start_index;
        unsigned char* extracted_data = (unsigned char*)malloc(extracted_size);
        if (extracted_data == NULL) {
            printf("内存分配失败。\n");
            free(content);
            return -1;
        }
        memcpy(extracted_data, content + start_index, extracted_size);

        // Create new filename
        char new_filename[1024];
        char base_name[512];
        const char* slash = strrchr(file_path, '/');
        if (slash != NULL) {
            strcpy(base_name, slash + 1);
        }
        else {
            strcpy(base_name, file_path);
        }
        char* dot = strrchr(base_name, '.');
        if (dot != NULL) {
            *dot = '\0';
        }
        sprintf(new_filename, "%s_%zu.%s", base_name, count, output_format);

        // Create new filepath
        char new_filepath[2048];
        strncpy(new_filepath, file_path, sizeof(new_filepath));
        new_filepath[sizeof(new_filepath)-1] = '\0';
        char* last_slash = strrchr(new_filepath, '/');
        if (last_slash != NULL) {
            *(last_slash + 1) = '\0';
        }
        else {
            new_filepath[0] = '\0';
        }
        strcat(new_filepath, new_filename);

        // Write to new file
        FILE* new_file = fopen(new_filepath, "wb");
        if (new_file == NULL) {
            printf("无法写入文件 %s。\n", new_filepath);
            free(extracted_data);
            start_index = end_index;
            continue;
        }
        fwrite(extracted_data, 1, extracted_size, new_file);
        fclose(new_file);
        printf("Extracted content saved as: %s\n", new_filepath);

        // Add note
        char note[1024];
        sprintf(note, "File: %s, Start Address: %zu, End Address: %zu", new_filepath, start_index, end_index);
        notes[note_count++] = strdup(note);

        count++;
        free(extracted_data);
        start_index = end_index;
    }

    // Save notes
    save_notes(file_path, (const char**)notes, note_count);

    // Free notes
    for (size_t i = 0; i < note_count; i++) {
        free((void*)notes[i]);
    }

    free(content);
    return 0;
}

int extract_after_address(const char* file_path, size_t target_index, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format, size_t min_repeat_count) {
    FILE* f = fopen(file_path, "rb");
    if (f == NULL) {
        printf("无法读取文件 %s。\n", file_path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size_t content_length = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* content = (unsigned char*)malloc(content_length);
    if (content == NULL) {
        fclose(f);
        printf("内存分配失败。\n");
        return -1;
    }
    fread(content, 1, content_length, f);
    fclose(f);

    if (target_index > content_length) {
        printf("指定地址 超出文件范围，无法提取。\n");
        free(content);
        return -1;
    }

    size_t count = 0;
    size_t start_index = target_index;
    const char* notes[1000];
    size_t note_count = 0;

    while (start_index < content_length) {
        // Find start_sequence
        size_t found = start_index;
        while (found <= content_length - start_length) {
            if (memcmp(content + found, start_sequence, start_length) == 0) {
                break;
            }
            found++;
        }
        if (found > content_length - start_length) {
            printf("No more start sequences found in %s after the target address\n", file_path);
            break;
        }
        start_index = found;

        // Get end_index
        size_t end_index = get_end_index(content, content_length, start_index, end_sequence, end_length, start_sequence, start_length);

        // Extract data
        size_t extracted_size = end_index - start_index;
        unsigned char* extracted_data = (unsigned char*)malloc(extracted_size);
        if (extracted_data == NULL) {
            printf("内存分配失败。\n");
            free(content);
            return -1;
        }
        memcpy(extracted_data, content + start_index, extracted_size);

        // Create new filename
        char new_filename[1024];
        char base_name[512];
        const char* slash = strrchr(file_path, '/');
        if (slash != NULL) {
            strcpy(base_name, slash + 1);
        }
        else {
            strcpy(base_name, file_path);
        }
        char* dot = strrchr(base_name, '.');
        if (dot != NULL) {
            *dot = '\0';
        }
        sprintf(new_filename, "%s_%zu.%s", base_name, count, output_format);

        // Create new filepath
        char new_filepath[2048];
        strncpy(new_filepath, file_path, sizeof(new_filepath));
        new_filepath[sizeof(new_filepath)-1] = '\0';
        char* last_slash = strrchr(new_filepath, '/');
        if (last_slash != NULL) {
            *(last_slash + 1) = '\0';
        }
        else {
            new_filepath[0] = '\0';
        }
        strcat(new_filepath, new_filename);

        // Write to new file
        FILE* new_file = fopen(new_filepath, "wb");
        if (new_file == NULL) {
            printf("无法写入文件 %s。\n", new_filepath);
            free(extracted_data);
            start_index = end_index;
            continue;
        }
        fwrite(extracted_data, 1, extracted_size, new_file);
        fclose(new_file);
        printf("Extracted content saved as: %s\n", new_filepath);

        // Add note
        char note[1024];
        sprintf(note, "File: %s, Start Address: %zu, End Address: %zu", new_filepath, start_index, end_index);
        notes[note_count++] = strdup(note);

        count++;
        free(extracted_data);
        start_index = end_index;
    }

    // Save notes
    save_notes(file_path, (const char**)notes, note_count);

    // Free notes
    for (size_t i = 0; i < note_count; i++) {
        free((void*)notes[i]);
    }

    free(content);
    return 0;
}

size_t get_end_index(const unsigned char* content, size_t content_length, size_t start_index, const unsigned char* end_sequence, size_t end_length, const unsigned char* start_sequence, size_t start_length) {
    if (end_sequence != NULL) {
        // Find end_sequence after start_index + start_length
        size_t found = start_index + start_length;
        while (found <= content_length - end_length) {
            if (memcmp(content + found, end_sequence, end_length) == 0) {
                return found + end_length;
            }
            found++;
        }
        return content_length;
    }
    else {
        // Find next start_sequence
        size_t found = start_index + start_length;
        while (found <= content_length - start_length) {
            if (memcmp(content + found, start_sequence, start_length) == 0) {
                return found;
            }
            found++;
        }
        return content_length;
    }
}

size_t find_end_index(const unsigned char* content, size_t content_length, size_t start_index, const unsigned char* end_sequence, size_t end_length, size_t min_repeat_count, const unsigned char* start_sequence, size_t start_length) {
    if (end_sequence == NULL) {
        // Find next start_sequence
        size_t found = start_index + 1;
        while (found <= content_length - start_length) {
            if (memcmp(content + found, start_sequence, start_length) == 0) {
                return found;
            }
            found++;
        }
        return content_length;
    }
    else {
        if (min_repeat_count == 0) {
            // Find first occurrence of end_sequence
            size_t found = start_index + 1;
            while (found <= content_length - end_length) {
                if (memcmp(content + found, end_sequence, end_length) == 0) {
                    return found + end_length;
                }
                found++;
            }
            return content_length;
        }
        else {
            // Find end_sequence repeated min_repeat_count times
            unsigned char byte_value = end_sequence[0];
            size_t repeat_count = 0;
            size_t current_index = start_index + 1;
            while (current_index < content_length) {
                if (content[current_index] == byte_value) {
                    repeat_count++;
                    if (repeat_count >= min_repeat_count) {
                        return current_index + 1;
                    }
                }
                else {
                    repeat_count = 0;
                }
                current_index++;
            }
            return content_length;
        }
    }
}

int save_notes(const char* file_path, const char** notes, size_t note_count) {
    // Create notes filename
    char notes_filename[1024];
    char base_name[512];
    const char* slash = strrchr(file_path, '/');
    if (slash != NULL) {
        strcpy(base_name, slash + 1);
    }
    else {
        strcpy(base_name, file_path);
    }
    char* dot = strrchr(base_name, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    sprintf(notes_filename, "%s_notes.txt", base_name);

    // Create notes filepath
    char notes_filepath[2048];
    strncpy(notes_filepath, file_path, sizeof(notes_filepath));
    notes_filepath[sizeof(notes_filepath)-1] = '\0';
    char* last_slash = strrchr(notes_filepath, '/');
    if (last_slash != NULL) {
        *(last_slash + 1) = '\0';
    }
    else {
        notes_filepath[0] = '\0';
    }
    strcat(notes_filepath, notes_filename);

    // Write notes to file
    FILE* notes_file = fopen(notes_filepath, "w");
    if (notes_file == NULL) {
        printf("无法写入文件 %s。\n", notes_filepath);
        return -1;
    }
    for (size_t i = 0; i < note_count; i++) {
        fprintf(notes_file, "%s\n", notes[i]);
    }
    fclose(notes_file);
    printf("Notes saved as: %s\n", notes_filepath);
    return 0;
}

void process_directory(const char* directory_path, int extract_mode, size_t target_index, const unsigned char* start_sequence, size_t start_length, const unsigned char* end_sequence, size_t end_length, const char* output_format, int use_repeat_method, size_t min_repeat_count) {
    DIR* dir = opendir(directory_path);
    if (dir == NULL) {
        printf("无法打开目录 %s。\n", directory_path);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build full file path
        char file_path[2048];
        snprintf(file_path, sizeof(file_path), "%s/%s", directory_path, entry->d_name);

        // Check if it's a file or directory
        struct stat st;
        if (stat(file_path, &st) == -1) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // Recursively process subdirectories
            process_directory(file_path, extract_mode, target_index, start_sequence, start_length, end_sequence, end_length, output_format, use_repeat_method, min_repeat_count);
        }
        else if (S_ISREG(st.st_mode)) {
            printf("Processing file: %s\n", file_path);
            if (extract_mode == 1) {
                if (use_repeat_method) {
                    extract_content_repeat(file_path, start_sequence, start_length, end_sequence, end_length, output_format, min_repeat_count);
                }
                else {
                    extract_content_normal(file_path, start_sequence, start_length, end_sequence, end_length, output_format);
                }
            }
            else if (extract_mode == 2) {
                extract_before_address(file_path, target_index, start_sequence, start_length, end_sequence, end_length, output_format, min_repeat_count);
            }
            else if (extract_mode == 3) {
                extract_after_address(file_path, target_index, start_sequence, start_length, end_sequence, end_length, output_format, min_repeat_count);
            }
        }
    }

    closedir(dir);
}
