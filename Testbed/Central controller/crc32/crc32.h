/* 
 * compute crc32 hash in tofino
 * require the CRCPolynomial to have following limits:
 *  poly: 0x04C11DB7 or 0xEDB88320 or 0xDB710641 or 0x82608EDB
 *  reversed: false
 *  init: 0xffffffff
 *  xor: 0xffffffff
 */

#ifndef _CRC32_H_
#define _CRC32_H_

#include <stdint.h>
#include <stddef.h>
#define CRC32_INIT 0XFFFFFFFF
#define CRC32_XOR  0XFFFFFFFF

uint32_t crc32_hash(uint32_t poly, uint8_t *data, size_t cnt);

#endif