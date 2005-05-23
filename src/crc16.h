#ifndef CRC16_H
#define CRC16_H
/*
 * Derived from CRC algorithm for JTAG ICE mkII, published in Atmel
 * Appnote AVR067.  Converted from C++ to C.
 */
/* $Id$ */

#if defined(__cplusplus)
extern "C" {
#endif
  unsigned short crcsum(const unsigned char* message,
			unsigned long length,
			unsigned short crc);
  /*
   * Verify that the last two bytes is a (LSB first) valid CRC of the
   * message.
   */
  int crcverify(const unsigned char* message,
			 unsigned long length);
  /*
   * Append a two byte CRC (LSB first) to message.  length is size of
   * message excluding crc.  Space for the CRC bytes must be allocated
   * in advance!
   */
  void crcappend(unsigned char* message,
		 unsigned long length);
#if defined(__cplusplus)
}
#endif

#endif
