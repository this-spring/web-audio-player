/*
 * @Author: xiuquanxu
 * @Company: kaochong
 * @Date: 2021-01-23 17:46:34
 * @LastEditors: xiuquanxu
 * @LastEditTime: 2021-01-27 15:52:31
 */
#define EACH_TS_LEN 188
#define PAT_PID 0

typedef struct TsDemux {
    int audioPid;
    int videoPid;
} ts_demux;

typedef struct PesPacket {
    uint8_t* pakcet;
    int size;
} pes_packet;
