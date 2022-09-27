#include "crc32.h"
#include "crc32_table.h"
#include <stdio.h>
#include <stdlib.h>

uint32_t crc32_0x04C11DB7(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0x04C11DB7[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_0xEDB88320(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0xEDB88320[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_0xDB710641(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0xDB710641[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_0x82608EDB(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0x82608EDB[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_0x741B8CD7(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0x741B8CD7[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_0xEB31D82E(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0xEB31D82E[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_0x11111111(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0x11111111[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_0x22222222(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0x22222222[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_0x06D23852(uint8_t *data, size_t cnt) {
    uint32_t crc_res = CRC32_INIT;   // initialize
    while (cnt--) {
        uint32_t nLookupIndex = ((crc_res >> 24) ^ *data++) & 0xff;
        crc_res = (crc_res << 8) ^ CRC_table_0x06D23852[nLookupIndex];
    }
    crc_res = crc_res ^ CRC32_XOR;
    return crc_res;
}

uint32_t crc32_hash(uint32_t poly, uint8_t *data, size_t cnt) {
    uint32_t res;
    switch(poly) {
        case 0x04C11DB7:
            res = crc32_0x04C11DB7(data, cnt);
            break;
        case 0xEDB88320:
            res = crc32_0xEDB88320(data, cnt);
            break;
        case 0xDB710641:
            res = crc32_0xDB710641(data, cnt);
            break;
        case 0x82608EDB:
            res = crc32_0x82608EDB(data, cnt);
            break;
        case 0x741B8CD7:
            res = crc32_0x741B8CD7(data, cnt);
            break;
        case 0xEB31D82E:
            res = crc32_0xEB31D82E(data, cnt);
            break;
		case 0x11111111:
			res = crc32_0x11111111(data, cnt);
			break;
		case 0x22222222:
			res = crc32_0x22222222(data, cnt);
			break;
        case 0x06D23852:
            res = crc32_0x06D23852(data, cnt);
            break;
        default:
            printf("Error in crc32_hash: the poly is not supported.\n");
            exit(1);
    }
    return res;
}
