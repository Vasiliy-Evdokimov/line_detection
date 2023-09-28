#ifndef CRC_HPP_
#define CRC_HPP_

extern unsigned char crc8(unsigned char*, unsigned char);
extern unsigned int crc16(volatile unsigned char* array, unsigned char size);
extern unsigned char crc_sum(volatile unsigned char* array, unsigned char size);

#endif /* CRC_HPP_ */
