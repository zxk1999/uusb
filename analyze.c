#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 512

typedef struct {
    int number;
    char type[64];
    char deque[20];
    char doorbell[20];
    char iman[20];
    char eint[20];
    char dma_addr[20];
    char dma_len[20];
    char TD_flag[20];
} BlockData;

void parse_block_line(BlockData *block, const char *line) {
    char key[32], value[32];
    const char *p = line;

    // 提取键名
    p = strchr(p, '"') + 1;
    char *end = strchr(p, '"');
    strncpy(key, p, end - p);
    key[end - p] = '\0';

    // 提取值
    p = strchr(end, ':') + 1;
    while (*p && isspace(*p)) p++; // 跳过空格
    end = p + strlen(p);
    while (end > p && isspace(*(end-1))) end--; // 去除尾部空格
    strncpy(value, p, end - p);
    value[end - p] = '\0';

    // 赋值到结构体
    if (strcmp(key, "deque") == 0) strcpy(block->deque, value);
    else if (strcmp(key, "doorbell") == 0) strcpy(block->doorbell, value);
    else if (strcmp(key, "iman") == 0) strcpy(block->iman, value);
    else if (strcmp(key, "eint") == 0) strcpy(block->eint, value);
    else if (strcmp(key, "dma_addr") == 0) strcpy(block->dma_addr, value);
    else if (strcmp(key, "dma_len") == 0) strcpy(block->dma_len, value);
    else if (strcmp(key, "TD_flag") == 0) strcpy(block->TD_flag, value);
}

int main() {
    FILE *fp = fopen("output_3_19_myloog.txt", "r");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    char line[MAX_LINE];
    BlockData current = {0};
    int in_block = 0;

    while (fgets(line, sizeof(line), fp)) {
        // 去除换行符
        line[strcspn(line, "\n")] = '\0';

        if (strstr(line, "Block ")) {
            if (in_block) {
                // 输出前一个块的数据
                printf("Block %d (%s):\n", current.number, current.type);
                printf("  deque: %s\n", current.deque);
                printf("  doorbell: %s\n", current.doorbell);
                printf("  iman: %s\n", current.iman);
                printf("  eint: %s\n", current.eint);
                printf("  dma_addr: %s\n", current.dma_addr);
                printf("  dma_len: %s\n", current.dma_len);
                printf("  TD_flag: %s\n\n", current.TD_flag);
            }

            // 初始化新块
            memset(&current, 0, sizeof(current));
            sscanf(line, "Block %d - Type: %63[^\n]", 
                  &current.number, current.type);
            in_block = 1;
        }
        else if (in_block && strstr(line, "block[\"")) {
            parse_block_line(&current, line);
        }
    }

    // 处理最后一个块
    if (in_block) {
        printf("Block %d (%s):\n", current.number, current.type);
        printf("  deque: %s\n", current.deque);
        printf("  doorbell: %s\n", current.doorbell);
        printf("  iman: %s\n", current.iman);
        printf("  eint: %s\n", current.eint);
        printf("  dma_addr: %s\n", current.dma_addr);
        printf("  dma_len: %s\n", current.dma_len);
        printf("  TD_flag: %s\n\n", current.TD_flag);
    }

    fclose(fp);
    return 0;
}