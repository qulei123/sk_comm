#ifndef _CRC16_H_
#define _CRC16_H_

#include "deftypes.h" 

extern U16 const crc16_table[256];

static inline U16 crc16_byte(U16 crc, const U8 data)
{
	return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

U16 crc16(const U8 *buffer, size_t len);


#endif /*  _CRC16_H_ */

