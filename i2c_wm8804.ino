/************************************************************************************

I2C_WM8804

************************************************************************************/


#include <Wire.h>
int wm8804 = 58; // device wm8804, 58d = 0x3A = 0111010 (see datasheet pp. 17, table 11)
byte error;
int SPDSTAT, INTSTAT;
int fs = 0;
bool toggle = 0;

void setup()
{
  Wire.begin();

  delay(10000);  // for debugging, gives me time to fire up a serial terminal
  Wire.beginTransmission(wm8804);
  error = Wire.endTransmission();

  if (error == 0)
  {
    Serial.print("WM8804 found");
    Serial.println("  !");
  }
  else if (error==4) 
  {
    Serial.println("Unknown error at address 0x3A");
  }
  else
  {
    Serial.println("No response from WM8804!");
  }
  
  Serial.print("Device ID: ");
  byte c = ReadRegister(wm8804, 1);
  if (c < 10) {
    Serial.print('0');
  } 
  Serial.print(c,HEX);
  
  c = ReadRegister(wm8804, 0);
  if (c < 10) {
    Serial.print('0');
  } 
  Serial.print(c,HEX);
  
  Serial.print(" Rev. ");
  c = ReadRegister(wm8804, 2);
  Serial.println(c,HEX);

  DeviceInit(wm8804);
  
  delay(2000);   
} // setup

void loop()                            // is quasi-interrupt routine
{
  INTSTAT = 0;
  while (INTSTAT == 0){
    delay(100);
    INTSTAT = ReadRegister(wm8804, 11); // poll (and clear) interrupt register
  }
  
  // decode why interrupt was triggered
  // most of these shouldn't happen as they are masked off
  // but still useful for debugging
 
  if (bitRead(INTSTAT, 0)) {                                         // UPD_UNLOCK
    SPDSTAT = ReadRegister(wm8804, 12);
    Serial.print("UPD_UNLOCK: ");
    if (bitRead(SPDSTAT,6)) {
      delay(1000);
 
      // try again to try to reject transient errors
      SPDSTAT = ReadRegister(wm8804, 12);
      if (bitRead(SPDSTAT,6)) {
      
        Serial.println("S/PDIF PLL unlocked");
        
        // switch PLL coeffs around to try to find stable setting
        
        if (toggle) {
          Serial.println("trying 192 kHz mode...");
          fs = 192;
          WriteRegister(wm8804, 6, 8);                  // set PLL_N to 8
          WriteRegister(wm8804, 5, 12);                 // set PLL_K to 0C49BA (0C)
          WriteRegister(wm8804, 4, 73);                 // set PLL_K to 0C49BA (49)
          WriteRegister(wm8804, 3, 186);                // set PLL_K to 0C49BA (BA)
          toggle = 0;
          delay(2000);
        }
        else {
          Serial.println("trying normal mode...");
          WriteRegister(wm8804, 6, 7);                  // set PLL_N to 7
          WriteRegister(wm8804, 5, 54);                 // set PLL_K to 36FD21 (36)
          WriteRegister(wm8804, 4, 253);                // set PLL_K to 36FD21 (FD)
          WriteRegister(wm8804, 3, 33);                 // set PLL_K to 36FD21 (21)
          toggle = 1;
          delay(2000);
        } // if toggle


      } // if (bitRead(SPDSTAT,6))

    }
    else {
      Serial.println("S/PDIF PLL locked");
    }
    delay(50); // sort of debounce
  }
  
  if (bitRead(INTSTAT, 1)) {                                         // INT_INVALID
    Serial.println("INT_INVALID");
  }
  
  if (bitRead(INTSTAT, 2)) {                                         // INT_CSUD
    Serial.println("INT_CSUD");
  }
  
  if (bitRead(INTSTAT, 3)) {                                         // INT_TRANS_ERR
    Serial.println("INT_TRANS_ERR");
  }
  
  if (bitRead(INTSTAT, 4)) {                                         // UPD_NON_AUDIO
    Serial.println("UPD_NON_AUDIO");
  }
  
  if (bitRead(INTSTAT, 5)) {                                         // UPD_CPY_N
    Serial.println("UPD_CPY_N");
  }
  
  if (bitRead(INTSTAT, 6)) {                                         // UPD_DEEMPH
    Serial.println("UPD_DEEMPH");
  }
    
  if (bitRead(INTSTAT, 7)) {                                         // UPD_REC_FREQ
    // clear serial terminal (in case of using PuTTY)
    Serial.write(27);       // ESC command
    Serial.print("[2J");    // clear screen command
    Serial.write(27);
    Serial.print("[H");     // cursor to home command
    SPDSTAT = ReadRegister(wm8804, 12);
    int samplerate = 2*bitRead(SPDSTAT,5) + bitRead(SPDSTAT,4);      // calculate indicated rate
    Serial.print("UPD_REC_FREQ: ");
    Serial.print("Sample rate: ");

    switch (samplerate) {
      case 3:
      Serial.println("32 kHz");
      fs = 32;
      WriteRegister(wm8804, 29, 0);                 // set SPD_192K_EN to 0
      delay(500);
      break;

      case 2:
      Serial.println("44 / 48 kHz");
      fs = 48;
      WriteRegister(wm8804, 29, 0);                 // set SPD_192K_EN to 0
      delay(500);
      break;

      case 1:
      Serial.println("88 / 96 kHz");
      fs = 96;
      WriteRegister(wm8804, 29, 0);                 // set SPD_192K_EN to 0
      delay(500);
      break;

      case 0:
      Serial.println("192 kHz");
      fs = 176;
      WriteRegister(wm8804, 29, 128);                 // set SPD_192K_EN to 1
      delay(500);
      break;
    } // switch samplerate
  } // if (bitRead(INTSTAT, 7))


} // loop



/****************************************************************************************/
// Functions go below main loop

byte ReadRegister(int devaddr, int regaddr){                              // Read a data register value
  Wire.beginTransmission(devaddr);
  Wire.write(regaddr);
  Wire.endTransmission(false);                                            // repeated start condition: don't send stop condition, keeping connection alive.
  Wire.requestFrom(devaddr, 1); // only one byte
  byte data = Wire.read();
  Wire.endTransmission(true);
  return data;
}

void WriteRegister(int devaddr, int regaddr, int dataval){                // Write a data register value
  Wire.beginTransmission(devaddr); // device
  Wire.write(regaddr); // register
  Wire.write(dataval); // data
  Wire.endTransmission(true);
}

void DeviceInit(int devaddr){                                              // resets, initializes and powers a wm8804
  // reset device
  WriteRegister(devaddr, 0, 0);

  // REGISTER 7
  // bit 7:6 - always 0
  // bit 5:4 - CLKOUT divider select => 00 = 512 fs, 01 = 256 fs, 10 = 128 fs, 11 = 64 fs
  // bit 3 - MCLKDIV select => 0
  // bit 2 - FRACEN => 1
  // bit 1:0 - FREQMODE (is written by S/PDIF receiver) => 00
  WriteRegister(devaddr, 7, B00000100);

  // REGISTER 8
  // set clock outputs and turn off last data hold
  // bit 7 - MCLK output source select is CLK2 => 0
  // bit 6 - always valid => 0
  // bit 5 - fill mode select => 1 (we need to see errors when they happen)
  // bit 4 - CLKOUT pin disable => 1
  // bit 3 - CLKOUT pin select is CLK1 => 0
  // bit 2:0 - always 0
  WriteRegister(devaddr, 8, B00110000);

  // set masking for interrupts
  WriteRegister(devaddr, 10, 126);  // 1+2+3+4+5+6 => 0111 1110. We only care about unlock and rec_freq

  // set the AIF TX
  // bit 7:6 - always 0
  // bit   5 - LRCLK polarity => 0
  // bit   4 - BCLK invert => 0
  // bit 3:2 - data word length => 10 (24b) or 00 (16b)
  // bit 1:0 - format select: 11 (dsp), 10 (i2s), 01 (LJ), 00 (RJ)
  WriteRegister(devaddr, 27, B00000001);

  // set the AIF RX
  // bit   7 - SYNC => 1
  // bit   6 - master mode => 1
  // bit   5 - LRCLK polarity => 0
  // bit   4 - BCLK invert => 0
  // bit 3:2 - data word length => 10 (24b) or 00 (16b)
  // bit 1:0 - format select: 11 (dsp), 10 (i2s), 01 (LJ), 00 (RJ)
  WriteRegister(devaddr, 28, B11001001);

  // set PLL K and N factors
  // this should be sample rate dependent, but makes hardly any difference
  WriteRegister(devaddr, 6, 7);                  // set PLL_N to 7
  WriteRegister(devaddr, 5, 0x36);                 // set PLL_K to 36FD21 (36)
  WriteRegister(devaddr, 4, 0xFD);                 // set PLL_K to 36FD21 (FD)
  WriteRegister(devaddr, 3, 0x21);                 // set PLL_K to 36FD21 (21)
  
  // power up device
  WriteRegister(devaddr, 30, 0);
}
