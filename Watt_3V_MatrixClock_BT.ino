#define DEBUG
// http://playground.arduino.cc/Code/time

#include <avr/sleep.h>
#include <avr/power.h>

// http://www.nongnu.org/avr-libc/user-manual/group__avr__eeprom.html
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>

Adafruit_8x8matrix matrix[4];
/*
PROGMEM static unsigned char digit[] = {
  B01111100, B10000010, B10010010, B10000010, B01111100, // 0
  B00000000, B00000000, B00000000, B00000000, B11111110, // 1
  B10001100, B10010010, B10010010, B10010010, B01100010, // 2
  B10000010, B10010010, B10010010, B10010010, B01101100, // 3
  B11100000, B00010000, B00010000, B00010000, B11111110, // 4
  B01100010, B10010010, B10010010, B10010010, B10001100, // 5
  B01111100, B10010010, B10010010, B10010010, B00001100, // 6
  B10000000, B10000000, B10000000, B10000000, B01111110, // 7
  B01101100, B10010010, B10010010, B10010010, B01101100, // 8
  B01100000, B10010010, B10010010, B10010010, B01111100  // 9
};
*/
#include <SoftwareSerial.h>
SoftwareSerial BtSerial(2, 3);

void drawDigit(uint8_t matrix_num, int16_t x, int16_t y, unsigned char num) {
  if((x >= 8)      || // Clip right
     (y >= 8)      || // Clip bottom
     ((x + 4) < 0) || // Clip left
     ((y + 7) < 0))   // Clip top
    return;

  for (int8_t i=4; i>=0; i--) {
    uint8_t line = pgm_read_byte(digit+(num*5)+i);

    for (int8_t j=7; j>=0; j--) {
      if (line & 0x1) {
        matrix[matrix_num].drawPixel(x+i, y+j, LED_ON);
      } else {
        matrix[matrix_num].drawPixel(x+i, y+j, LED_OFF);
      }
      line >>= 1;
    }
  }
}

void scrollDigit(uint8_t matrix_num, int16_t x_off, uint8_t count, uint8_t old_num, uint8_t new_num) {
  int8_t dir = (old_num == new_num ? 0 : (old_num > new_num ? -1 : 1));
  if (dir != 0) {
    drawDigit(matrix_num, x_off, dir * count - 8, new_num);
    drawDigit(matrix_num, x_off, dir * count, old_num);
    drawDigit(matrix_num, x_off, dir * count + 8, new_num);
  }
}

static time_t prevtime, last_time, current_time;

int16_t TimeZone;
uint16_t TimeZoneAddr = 1;

uint8_t Brightness;
uint16_t BrightnessAddr = 3;

void setup() {
/*
  // is only run once for first EEPROM-init
  while (!eeprom_is_ready());
  eeprom_write_word((uint16_t*)TimeZoneAddr, 7200);
  eeprom_write_byte((unit8_t*)BrightnessAddr, 15);
*/

//  Serial.begin(9600);
  BtSerial.begin(9600);
  
  // fetch TimeZone from EEPROM
  while (!eeprom_is_ready());
  TimeZone = eeprom_read_word((uint16_t*)TimeZoneAddr);
  Brightness = eeprom_read_byte((uint8_t*)BrightnessAddr);

  Wire.begin();
  setSyncProvider(RTC.get);
  
  prevtime = current_time = last_time = 0;

  for (uint8_t i=0; i<4; i++) {
    matrix[i] = Adafruit_8x8matrix();
    matrix[i].begin(0x70 + i);
    matrix[i].setBrightness(Brightness);
    matrix[i].setRotation(0);    
    matrix[i].clear();
    matrix[i].writeDisplay();
  }
  TWBR=12;

//  power_adc_disable(); // we ned Analog-Digital-Converter for Voltage-Check .. but not all the time ..
  power_spi_disable();
  power_timer2_disable();
}

void loop() {
  int16_t BtMessage = -1;
  
  if(BtSerial.available()) {
    BtMessage = processSyncMessage();
  }

  if (BtMessage != -1) {
    // do something
  }

  if (prevtime != now()) {
    prevtime = now();

    // fetch TimeZone from EEPROM
    while (!eeprom_is_ready());
    TimeZone = eeprom_read_word((uint16_t*)TimeZoneAddr);
    Brightness = eeprom_read_byte((uint8_t*)BrightnessAddr);

    current_time = prevtime + TimeZone;
    current_time += IsDst(hour(current_time), day(current_time), month(current_time), weekday(current_time)) ? 60 * 60 : 0;
    
    int8_t l_h1 = hour(last_time) / 10;
    int8_t l_h2 = hour(last_time) - (l_h1 * 10);
    int8_t l_m1 = minute(last_time) / 10;
    int8_t l_m2 = minute(last_time) - (l_m1 * 10);
    int8_t h1 = hour(current_time) / 10;
    int8_t h2 = hour(current_time) - (h1 * 10);
    int8_t m1 = minute(current_time) / 10;
    int8_t m2 = minute(current_time) - (m1 * 10);

    if (second(current_time) % 2 == 0) {
      matrix[1].drawPixel(7, 1, LED_ON);
      matrix[1].drawPixel(7, 2, LED_OFF);
      matrix[1].drawPixel(7, 4, LED_ON);
      matrix[1].drawPixel(7, 5, LED_OFF);      
      matrix[2].drawPixel(0, 1, LED_OFF);
      matrix[2].drawPixel(0, 2, LED_ON);
      matrix[2].drawPixel(0, 4, LED_OFF);
      matrix[2].drawPixel(0, 5, LED_ON);      
    } else {
      matrix[1].drawPixel(7, 1, LED_OFF);
      matrix[1].drawPixel(7, 2, LED_ON);
      matrix[1].drawPixel(7, 4, LED_OFF);
      matrix[1].drawPixel(7, 5, LED_ON);      
      matrix[2].drawPixel(0, 1, LED_ON);
      matrix[2].drawPixel(0, 2, LED_OFF);
      matrix[2].drawPixel(0, 4, LED_ON);
      matrix[2].drawPixel(0, 5, LED_OFF);      
    }

    if ((hour(current_time) != hour(last_time)) || (minute(current_time) != minute(last_time))) {
      // do all four digits in one loop for syncronized change
      for (uint8_t count = 0; count <= 8; count++) {
        scrollDigit(3, 1, count, l_m2, m2);
        matrix[3].writeDisplay();

        scrollDigit(2, 2, count, l_m1, m1);
        matrix[2].writeDisplay();

        scrollDigit(1, 1, count, l_h2, h2);
        matrix[1].writeDisplay();

        scrollDigit(0, 2, count, l_h1, h1);
        matrix[0].writeDisplay();

        delay(120);
      }
    }

    drawDigit(3, 1, 0, m2);
    drawDigit(2, 2, 0, m1);
    drawDigit(1, 1, 0, h2);
    drawDigit(0, 2, 0, h1);

    for (uint8_t i = 0; i < 4; i++) {
      matrix[i].setBrightness(Brightness);
      matrix[i].writeDisplay();
    }
  }
  
  last_time = current_time;
  delay(250);
}
