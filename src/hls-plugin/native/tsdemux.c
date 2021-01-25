/*
 * @Author: xiuquanxu
 * @Company: kaochong
 * @Date: 2021-01-23 17:46:28
 * @LastEditors: xiuquanxu
 * @LastEditTime: 2021-01-25 17:49:56
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tsdemux.h"

// 解析pat获取pmt的pid
int parsePmtPid(char* tsData, int offset) {
    return (tsData[offset + 10] & 0x1f) << 8 | tsData[offset + 11];
}

// 解析pmt
void parsePmtData(char* tsData, int offset) {
    int section_len = (tsData[offset + 1] & 0x0f) << 8 | tsData[offset + 2];
    int table_end = offset + 3 + section_len  - 4;
    int program_infolen = (tsData[offset + 10] & 0x0f) << 8 | tsData[offset + 11];
    offset += 12 + program_infolen;
    while (offset < table_end)
    {
        int pid = (tsData[offset + 1] & 0x1f) << 8 | tsData[offset + 2];
        printf("pmt pid:%d\n", pid);
        offset += ((tsData[offset + 3] & 0x0F) << 8 | tsData[offset + 4]) + 5;
    }    
}

void parseTs(char* tsData, int size) {
    int pos = 0;
    while (pos < size) {
        int pid; // pid值
        int afc; // 后面是否有负载
        int offset; // 
        int pmt_pid = -1;
        if (tsData[pos] == 0x47) {
            offset = pos;
            pid = ((tsData[pos + 1] & 0x1f) << 8) | tsData[pos + 2];
            afc = (tsData[pos + 3] >> 4) & 0x03;
            int stt = (tsData[pos + 1] & 0x40);
            printf("stt:%d\n", stt);
            if (afc > 1) {
                offset = pos + 5 + tsData[pos + 4];
                if (offset == (pos + EACH_TS_LEN)) {
                    continue;
                }
            } else {
                offset = pos + 4;
            }
            if (pid == PAT_PID) {
                pmt_pid = parsePmtPid(tsData, offset);
                printf("pmt pid:%d\n", pmt_pid);
            } else if (pid == pmt_pid) {
                parsePmtData(tsData, offset);
            } else {
                printf("pid is not define:%d\n", pid);
            }
            // printf(" pid:%d, afc:%d\n", pid, afc);
        } else {
            printf("ts start pos is not 0x47, pos:%d\n", pos);
            break;
        }
        pos += EACH_TS_LEN;
    }
}

int main() {
    FILE *fp = NULL;
    fp = fopen("../resource/test.ts", "r+");
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    char * data = (char *)malloc((length + 1) * sizeof(char));
    rewind(fp);
    length = fread(data, 1, length, fp);
    data[length] = '\0';
    // int pos = 0;
    // for (int i = 0; i < 10; i += 1) {
    //     printf("header: %#X\n", data[pos]);
    //     pos += 188;
    // }
    fclose(fp);
    parseTs(data, length);
    return 0;  
}
