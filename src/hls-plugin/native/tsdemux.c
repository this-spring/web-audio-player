/*
 * @Author: xiuquanxu
 * @Company: kaochong
 * @Date: 2021-01-23 17:46:28
 * @LastEditors: xiuquanxu
 * @LastEditTime: 2021-01-27 17:32:16
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tsdemux.h"


ts_demux* init() {
    ts_demux *td = (ts_demux *)malloc(sizeof(ts_demux));
    td->audioPid = -1;
    td->videoPid = -1;
    return td;
}

// 一个ts至少包含3个ts包，pat、pmt、pid
int syncOffset(char* tsData, int size) {
    int i = 0;
    while(i < size) {
        if (tsData[i] == 0x47 && tsData[i + 188] == 0x47 && tsData[i + 2 * 188] == 0x47) {
            return i;
        } else {
            i += 1;
        }
    }
    return -1;
}

// 解析pat获取pmt的pid
int parsePat(char* tsData, int offset) {
    return (tsData[offset + 10] & 0x1f) << 8 | tsData[offset + 11];
}

// 解析pmt
void parsePmt(ts_demux* handle, char* tsData, int offset) {
    printf("start parsePmt\n");
    int section_len = (tsData[offset + 1] & 0x0f) << 8 | tsData[offset + 2];
    int table_end = offset + 3 + section_len  - 4;
    int program_infolen = (tsData[offset + 10] & 0x0f) << 8 | tsData[offset + 11];
    offset += 12 + program_infolen;
    while (offset < table_end)
    {
        int pid = (tsData[offset + 1] & 0x1f) << 8 | tsData[offset + 2];
        int stream_type = tsData[offset];
        switch (stream_type) {
        case 0x0f:
            // aac
            handle->audioPid = pid;
            printf("audio:%d\n", pid);
            break;
        case 0x1b:
            // h264
            handle->videoPid = pid;
            printf("video:%d\n", pid);
            break;
        default:
            break;
        }
        printf("audio or video pid:%d %d\n", pid, tsData[offset]);
        offset += ((tsData[offset + 3] & 0x0F) << 8 | tsData[offset + 4]) + 5;
    }    
}

uint8_t* parsePes(uint8_t* stream, int size) {
    int pes_len = 0;
    int pes_has_pes_pts = 0;
    int flag = 0;
    int pes_start_code = (stream[0] << 16) + (stream[1] << 8) + stream[2];
    printf("pes_start_code:%d\n", pes_start_code);
    if (pes_start_code == 0x000001) {
        // for (int i = 0; i < size; i += 1) {
        //     printf("pos:%d, val:%d\n", i, stream[i]);
        // }
        // pes_len如果为0表示长度不受限制
        pes_len = (stream[4] << 8) + stream[5];
        flag = stream[7];
        printf("stream[4]:%d\n, stream[5]:%d\n, pes_len:%d\n", stream[4], stream[5], pes_len);
        // pes_has_pes_pts = stream[offset + 7];
        // if (pes_has_pes_pts == 0xC0) {
            // int pts = stream[9];
        // }
    }
    return stream + pes_len;
}

void parseTs(ts_demux* handle, char* tsData, int size, FILE* fout) {
    int start = 0;
    int sync = syncOffset(tsData, size);
    // char* pes_packet = (char *)malloc(sizeof(char));
    pes_packet* pk = (pes_packet *)malloc(sizeof(pes_packet));
    pk->pakcet = (uint8_t *)malloc(1 * 1024 * 1024);
    start = sync;
    printf("sync:%d size:%d\n", sync, size);
    int pid; // pid值
    int afc; // 后面是否有负载
    int offset; // 
    int stt;
    int pmt_pid = -1;
    while (start < size) {
        if (tsData[start] == 0x47) {
            offset = start;
            pid = ((tsData[start + 1] & 0x1f) << 8) | tsData[start + 2];
            afc = (tsData[start + 3] >> 4) & 0x03;
            // 一个完整包的开始
            stt = !!(tsData[start + 1] & 0x40);
            printf("offset: %d, pid: %d, afc: %d, stt:%d\n", offset, pid, afc , stt);
            if (afc > 1) {
                offset = start + 5 + tsData[start + 4];
                if (offset == (start + EACH_TS_LEN)) {
                    continue;
                }
            } else {
                offset = start + 4;
            }
            if (pid == PAT_PID) {
                if (stt) {
                    offset += tsData[offset] + 1;
                }
                pmt_pid = parsePat(tsData, offset);
                printf("pmt pid:%d\n", pmt_pid);
            } else if (pid == pmt_pid) {
                if (stt) {
                    offset += tsData[offset] + 1;
                }
                parsePmt(handle, tsData, offset);
            } else if (pid == handle->audioPid) {
                // aac

            } else if (pid == handle->videoPid) {
                // h264                
                if (stt) {
                    if (pk->size > 0) {
                        printf("parsePes 0:%d, 1:%d, 2:%d, 3: %d, 4:%d\n", pk->pakcet[0], pk->pakcet[1], pk->pakcet[2], pk->pakcet[3], pk->pakcet[4]);
                        uint8_t* file = parsePes(pk->pakcet, pk->size);
                        // fwrite(pk->pakcet, pk->size, 1, fout);
                        if (pk->pakcet[8] == 5) {
                            fwrite(pk->pakcet + 14, pk->size - 14, 1, fout);
                        } else {
                            fwrite(pk->pakcet + 19, pk->size - 19, 1, fout);
                        }
                    }
                    memset(pk->pakcet, 0, pk->size);
                    pk->size = 0;
                }
                int len = start + 188 - offset;
                printf("pk-size:%d, 0:%d, 1:%d, 2:%d, 3:%d, len:%d\n", pk->size, pk->pakcet[0], pk->pakcet[1], pk->pakcet[2], pk->pakcet[3], len);
                memcpy(pk->pakcet + pk->size, tsData + offset, len);
                pk->size += len;
            } else {
                printf("pid is not define:%d\n", pid);
            }
            // printf(" pid:%d, afc:%d\n", pid, afc);
        } else {
            printf("ts start start is not 0x47, start:%d\n", start);
            break;
        }
        start += EACH_TS_LEN;
    }
}

int main() {
    FILE *fp = NULL;
    FILE *fout = NULL;
    fp = fopen("../resource/kc.ts", "r+");
    // fp = fopen("../resource/test.ts", "r+");
    fout = fopen("./test.h264", "wb");
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    char * data = (char *)malloc((length + 1) * sizeof(char));
    rewind(fp);
    length = fread(data, 1, length, fp);
    data[length] = '\0';
    fclose(fp);
    ts_demux* handle = init();
    parseTs(handle, data, length, fout);
    // fout.close();
    return 0;  
}
