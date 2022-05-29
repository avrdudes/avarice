#ifndef CRC16_H
#define CRC16_H

/*
 * C++ checksum calculation as found in "AVR067: JTAGICE mkII Communication Protocol"
 */
class Crc16 {
  public:
    // Calculate the checksum of a message.
    static unsigned short Checksum(const unsigned char *message, unsigned long length,
                                   unsigned short crc = 0xffff);

    // Verify that the last two bytes is a (LSB first)
    // valid CRC of the message.
    static bool VerifyChecksum(const unsigned char *message, unsigned long length);

    // Append a two byte CRC (LSB first) to message.
    // length is size of message excluding crc.
    // Space for the CRC bytes must be allocated in advance!
    static void AppendChecksum(unsigned char *message, unsigned long length);
};

#endif
