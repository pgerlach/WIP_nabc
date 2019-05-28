/**
 Library for using the RFID readers found on Nabaztags, based on the
 ST CR14 chip, with an Arduino.
*/
#ifndef CR14_rfid_h
# define CR14_rfid_h

#include "Arduino.h"

class Rfid {

  public:
    Rfid();
    Rfid(const int i2cAddr);
    boolean init();
    boolean read();
    char currentTag[17]; // 8 bytes (en ascii sur 2 caractères) + '\0'

  private:
    int readRegister(byte regAddr, int len);
    boolean writeRegister(byte regAddr, int len);
    void ackPolling();

    byte data[10];  // buffer pour la com' i2c
    byte error;
    int rfidI2cAddr; // l'addr du lecteur sur le bus i2c

    static const int DEFAULT_I2C_ADDR = 0x50;
};

#endif // ! CR14_rfid_h

