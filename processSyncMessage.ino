#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message

/*

  echo T$(date -u +%s) > /dev/cu.usbserial-A1017SEK

*/

#define TIMEZONE_MSG_LEN 4
#define TIMEZONE_HEADER 'Z'

/*

  echo Z+01 > /dev/cu.usbserial-A1017SEK

*/

#define BRIGHTNESS_MSG_LEN 3
#define BRIGHTNESS_HEADER 'B'

/*
  echo B+
*/

uint16_t processSyncMessage() {
  // if time sync available from serial port, update time and return true
  while(BtSerial.available()) {
    char c = BtSerial.read();
    #ifdef DEBUG
    BtSerial.print(c);
    #endif

    if (c == TIME_HEADER) {
      uint32_t pctime = 0;

      for (uint8_t i=TIME_MSG_LEN; i>0; i--) {
        c = BtSerial.read();
        if (c >= '0' && c <= '9') {
          pctime = (10 * pctime) + (c - '0'); // convert digits to a number
        }
      }

      #ifdef DEBUG
      BtSerial.println(pctime);
      #endif

      // Sync Arduino clock to the time received on the serial port
      RTC.set(pctime);
      setTime(pctime);

      #ifdef DEBUG
      BtSerial.println(now());
      #endif
      
      return TIME_HEADER;
    } else if (c == TIMEZONE_HEADER) {
      int8_t NewTimeZone = 0;
      int8_t TZ_Neg = 1;
      
      for (uint8_t i=TIMEZONE_MSG_LEN; i>0; i--) {
        c = BtSerial.read();
        if (c >= '0' && c <= '9') {
          NewTimeZone = (10 * NewTimeZone) + (c - '0');
        } else if (c == '-') {
          TZ_Neg = -1;
        }
      }
      NewTimeZone *= TZ_Neg;

      while (!eeprom_is_ready());
      eeprom_write_word((uint16_t*)TimeZoneAddr, 3600 * NewTimeZone);

      #ifdef DEBUG
      BtSerial.println(NewTimeZone);
      #endif
      
      return TIMEZONE_HEADER;
    } else if (c == BRIGHTNESS_HEADER) {
      uint8_t NewBrightness = 0;
      boolean UpDown = false;
      
      for (uint8_t i=BRIGHTNESS_MSG_LEN; i>0; i--) {
        c = BtSerial.read();
        if (UpDown) break;

        if (c >= '0' && c <= '9') {
          NewBrightness = (10 * NewBrightness) + (c - '0');
          
          if (NewBrightness < 0) {NewBrightness = 0;}
          if (NewBrightness > 15) {NewBrightness = 15;}
        } else if (c == '+') {
          UpDown = true;
          NewBrightness = Brightness < 15 ? Brightness + 1 : Brightness;
        } else if (c == '-') {
          UpDown = true;
          NewBrightness = Brightness > 0 ? Brightness - 1 : Brightness;
        }
      }
      
      while (!eeprom_is_ready());
      eeprom_write_byte((uint8_t*)BrightnessAddr, NewBrightness);
      
      #ifdef DEBUG
      BtSerial.println(NewBrightness);
      #endif
      
      return BRIGHTNESS_HEADER;
    } else {
      #ifdef DEBUG
      BtSerial.print("unk: ");
      BtSerial.println(c);
      #endif
      
      return 0;
    }
  }
  
  return 0;
}

