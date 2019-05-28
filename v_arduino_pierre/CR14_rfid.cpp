/**
Building an RFID short-range reader using the STM8S-DISCOVERY (AN3255)
(Application Note + Code d'example)
http://www.st.com/web/en/catalog/tools/PF257999

Datasheet d'une EEPROM d'exemple
http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/CD00236829.pdf
*/

#include <Wire.h>

#include "CR14_Rfid.h"

Rfid::Rfid() {
  this->rfidI2cAddr = Rfid::DEFAULT_I2C_ADDR;
}

Rfid::Rfid(const int i2cAddr) {
  this->rfidI2cAddr = i2cAddr;
}


boolean Rfid::init() {
  Serial.print("Rfid::init, on addr: "); Serial.println(this->rfidI2cAddr, HEX);

  int error = 1;

  // "ping" on i2cbus
  Wire.beginTransmission(this->rfidI2cAddr);
  error = Wire.endTransmission();
  if (error != 0)
  {
    Serial.print("RFID reader not found on i2c bus at addr ");
    Serial.println(this->rfidI2cAddr, HEX);
    return false;
  }

  // enable RF
  data[0] = 0x30;
  return writeRegister(0x00, 1);
}

boolean Rfid::read() {
  // check if at leadt a tag is present

  data[0] = 0x00;
 
  /*** Send Cmd Initiate ***/
  data[0] = 0x02;
  data[1] = 0x06;
  data[2] = 0x00;
  writeRegister(0x01, 3);

  ackPolling();

  readRegister(0x01, 3);
  
  if (data[0] == 0x00) {
    // no tag
    return false;
  }

  /* Save Chip ID */
  byte Chip_ID = data[1];

  /*** Send Cmd Select + Chip_ID ***/
  data[0] = 0x02;
  data[1] = 0x0E;
  data[2] = Chip_ID;
  writeRegister(0x01, 3);

  ackPolling();

  readRegister(0x01, 2);

  if (0x00 == data[0]) {
    return false;
  }

  /*** Send Cmd Get_UID ***/
  data[0] = 0x01;
  data[1] = 0x0B;
  writeRegister(0x01, 2);

  ackPolling();

  /*** Get UID ***/
  readRegister(0x01, 9);
  sprintf(currentTag,
          "%02X%02X%02X%02X%02X%02X%02X%02X",
          data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
  return true;
}


int Rfid::readRegister(byte regAddr, int len) {
  int error;

  // dummy write to indicate which register we want to read
  Wire.beginTransmission(rfidI2cAddr);
  Wire.write(regAddr);
  error = Wire.endTransmission();
  if (error != 0) {
    return -1;
  }

  // now read it
  int idx = 0;
  Wire.requestFrom(rfidI2cAddr, len);
  for (idx=0; Wire.available() && idx < len; idx++) {
    data[idx] = Wire.read();    // receive a byte as character
  }

  // return the number of characters read
  return (idx);
}

boolean Rfid::writeRegister(byte regAddr, int len) {
  int error;
  Wire.beginTransmission(rfidI2cAddr);
  Wire.write(regAddr); // register
  Wire.write(data, len); // value
  error = Wire.endTransmission();
  return (error == 0);
}


void Rfid::ackPolling() {
  while (true) {
     Wire.beginTransmission(rfidI2cAddr);
     if (0 == Wire.endTransmission()) {
       break;
     }
     delay(10);
  }
}


