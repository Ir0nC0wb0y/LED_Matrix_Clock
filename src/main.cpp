#include <Arduino.h>
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
//#define FASTLED_FORCE_SOFTWARE_SPI
#define FASTLED_ESP32_LCD_DRIVER
#include <FastLED.h>

#include <DS3232RTC.h>
#include <TimeLib.h>
#include <Wire.h>

#define MATRIX_WIDTH  48
#define MATRIX_HEIGHT  8
#define NUM_LEDS (MATRIX_HEIGHT * MATRIX_WIDTH)

#define LED_CHIPSET NEOPIXEL
#define LED_COLOR_ORDER GRB

#define PIN_LED_DATA_1  4
#define PIN_LED_DATA_2  5
#define PIN_LED_DATA_3  6
#define PIN_LED_DATA_4  7
#define PIN_LED_DATA_5 15 
#define PIN_LED_DATA_6 16 
#define PIN_LED_DATA_7 17 
#define PIN_LED_DATA_8 18 


CRGB leds[MATRIX_WIDTH * MATRIX_HEIGHT];

/*
#define MATRIX_COLUMNS
#ifdef MATRIX_COLUMNS
  int matrix_column = 0;
  int matrix_direction = 1;
#endif

//#define MATRIX_ID
#ifdef MATRIX_ID
  int matrix_id = 0;
#endif

//#define MATRIX_RANDOM
#ifdef MATRIX_RANDOM
#endif
*/

// Setup for messages
  unsigned long message_last = 0;
  #define MESSAGE_DURATION 2000

// Setup for Font/Frame
  #include "customfont.h"
  struct frame {
    char frame[MATRIX_WIDTH] = {0};
    int length = 0;
  };
  frame frame_current;
  int  led_position(int col, int row);
  bool get_nth_bit(char byte, int n);
  void updateFrame();
  void convertMessage(String message);
  void clearDisplay();
  //void testMessage();
  #define MATRIX_BRIGHTNESS 24

// Setup for RTC
  bool valid_RTC = false;
  void SerialClockDisplay(bool force = false);
  void printDigits(int digits);
  #define SERIAL_CLOCK_REFRESH_RATE 1000
  unsigned long serial_clock_update = 0;
  //#define SET_HARDCODED_TIME

  void MatrixClockDisplay();
  #define MATRIX_CLOCK_REFRESH_RATE 50
  unsigned long matrix_clock_update = 0;

unsigned long tic = 0;
unsigned long toc = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

  Wire.begin();

  // ESP Diagnostics
    Serial.print("Total heap: "); Serial.println(ESP.getHeapSize());
    Serial.print("Free heap: "); Serial.println(ESP.getFreeHeap());
    Serial.print("Total PSRAM: "); Serial.println(ESP.getPsramSize());  // If this prints out 0, then PSRAM is not enabled.
    Serial.print("Free PSRAM: "); Serial.println(ESP.getFreePsram());
    Serial.println();

  Serial.print("FastLED Version Integer: ");
  Serial.println(FASTLED_VERSION);
  Serial.println();

  Serial.println("Starting Matrix");

  Serial.print("Adding LED's to array");
  Serial.print(" Row1");
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_1>(leds,              0, MATRIX_WIDTH);
  Serial.print(" Row2");
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_2>(leds,   MATRIX_WIDTH, MATRIX_WIDTH);
  Serial.print(" Row3");
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_3>(leds, 2*MATRIX_WIDTH, MATRIX_WIDTH);
  Serial.print(" Row4");
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_4>(leds, 3*MATRIX_WIDTH, MATRIX_WIDTH);
  Serial.print(" Row5");
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_5>(leds, 4*MATRIX_WIDTH, MATRIX_WIDTH);
  Serial.print(" Row6");
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_6>(leds, 5*MATRIX_WIDTH, MATRIX_WIDTH);
  Serial.print(" Row7");
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_7>(leds, 6*MATRIX_WIDTH, MATRIX_WIDTH);
  Serial.print(" Row8");
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_8>(leds, 7*MATRIX_WIDTH, MATRIX_WIDTH);
  Serial.println(" ... Complete");

  FastLED.setBrightness(MATRIX_BRIGHTNESS);
  convertMessage("01:23:45");
  delay(2000);
  convertMessage("45:67:89");
  delay(2000);

  clearDisplay();

  // Setup RTC
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    if(timeStatus() != timeSet) {
      Serial.println("Unable to sync with the RTC");
    } else {
      Serial.println("RTC setup complete");
      valid_RTC = true;

      #ifdef SET_HARDCODED_TIME
        // Set Hardcoded Time
        tmElements_t tm;
        time_t t;
        tm.Year   = CalendarYrToTm(2026);
        tm.Month  = 4;
        tm.Day    = 17;
        tm.Hour   = 23;
        tm.Minute = 58;
        tm.Second = 45;
        t = makeTime(tm);
        RTC.set(t);        //use the time_t value to ensure correct weekday is set
        setTime(t);
        Serial.println("Forced hardcode time");
        SerialClockDisplay(true);
      #endif
    }

  Serial.println("Setup Complete, moving into loop");

  //delay(10000);
}

void loop() {
  //if (millis() - message_last >= MESSAGE_DURATION) {
  //  // create new message
  //}

  // Test RTC Clock
  SerialClockDisplay();
  MatrixClockDisplay();

}

void clearDisplay() {
  // Clear all LED's
  for (int i=0; i<NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

int led_position(int col, int row) {
  int led = 0;
  led = (row)*48+col;
  return led;
}

bool get_nth_bit(char byte_value, int n) {
  if (n < 0 || n >= 8) {
    Serial.println("####### ERROR: bitwise op out of bounds");
    return false;
  }
  // Right shift the value by n positions
    // This moves the nth bit to the least significant position
    char shifted_value = byte_value >> n;

    //Serial.print("Byte: "); Serial.println(byte_value, BIN);
    //Serial.print("place: "); Serial.println(n);
    //Serial.print("Shifted: "); Serial.println(shifted_value, BIN);
    //Serial.print("Nth value: "); Serial.println(shifted_value & 1);

    // Perform a bitwise AND with 1 (00000001 in binary)
    // This isolates the LSB, which is our target bit
    if (shifted_value & 1) {
      return true;
    } else {
      return false;
    }
}

void updateFrame() {
  // loop through led array, may require an XY translator
  for (int col = 0; col < MATRIX_WIDTH; ++col) { // column
    char current_byte = frame_current.frame[col];
    for (int row = 0; row < 8; row++) { // row
      //Serial.print("Current Row: "); Serial.print(row); Serial.print(" and Column: "); Serial.println(col);
      int current_led = led_position(col, row);
      //Serial.print("Current LED: "); Serial.println(current_led);
      if (get_nth_bit(current_byte,row)) {
        leds[current_led] = CRGB::Red;
      } else {
        leds[current_led] = CRGB::Black;
      }
      //Serial.println();
    }
  }
  FastLED.show();
}

void convertMessage(String message) {
  //Serial.print("Displaying Message: "); Serial.println(message);
  // test string: "01:23:45"
  int string_length = message.length();
  int display_length = 0;
  frame frame_temp;
  int display_column_id = 0;
  //Serial.print("Message Length: "); Serial.println(string_length);
  for (int i = 0; i < string_length; i++) {
    // Search through the character order
    int font_number_id = font_numbers_order.indexOf(message[i]);
    //Serial.print("Searching for: "); Serial.println(message[i]);
    if (font_number_id >= 0) {
      // Add character to display frame
      int number_width = font_numbers_length[font_number_id];
      //Serial.print("Found number "); Serial.print(font_numbers_order[font_number_id]); Serial.print(" at index "); Serial.print(font_number_id); Serial.print(" with width "); Serial.println(number_width);
      for (int j = 0; j < number_width; j++) {
        frame_temp.frame[display_column_id + j] = font_numbers[font_number_id][j];
        //Serial.print("j"); Serial.print(j); Serial.print(" col"); Serial.print(display_column_id + j); Serial.print(" BIN "); Serial.println(font_numbers[font_number_id][j],BIN);
      }
      display_column_id = display_column_id + number_width;
    } else {
      int font_symbol_id = font_symbols_order.indexOf(message[i]);
      if (font_symbol_id >= 0) {
        // add symbol to display frame
        int symbol_width = font_symbols_length[font_symbol_id];
        //Serial.print("Found symbol "); Serial.print(font_symbols_order[font_symbol_id]); Serial.print(" at index "); Serial.print(font_symbol_id); Serial.print(" with width "); Serial.println(symbol_width);
        for (int j = 0; j < symbol_width; j++) {
          frame_temp.frame[display_column_id + j] = font_symbols[font_symbol_id][j];
          //Serial.print("j"); Serial.print(j); Serial.print(" col"); Serial.print(display_column_id + j); Serial.print(" BIN "); Serial.println(font_symbols[font_symbol_id][j],BIN);
        }
        display_column_id = display_column_id + symbol_width;
      } else {
        // character not found in font file
        //Serial.print("Symbol "); Serial.print(message[i]); Serial.println(" not found in font file");
      }
    }
    
    // Add blankspace
    if (i<string_length-1) {
      //Serial.println("Adding space");
      frame_temp.frame[display_column_id + 1] = 0x00000000;
      display_column_id++;
    }
    //Serial.print("Frame length: "); Serial.println(display_column_id);
  }

  //Serial.println("Current Frame:");
  for (int i = 0; i < MATRIX_WIDTH; ++i) {
  //  //Serial.println(frame_temp[i], BIN);
    frame_current.frame[i] = frame_temp.frame[i];
  }
  //Serial.println();
  updateFrame();
}

void MatrixClockDisplay() {
  if (millis() - matrix_clock_update >= MATRIX_CLOCK_REFRESH_RATE) {
    // convert time_t to String
    time_t t = now();
    char buffer[20];
    sprintf(buffer, "%02d:%02d:%02d", hour(t), minute(t), second(t));
    String formattedTime = String(buffer);

    // Push message to matrix
    convertMessage(formattedTime);
  }
}


void SerialClockDisplay(bool force) {
  // digital clock display of the time
  if (valid_RTC) {
    if (millis() - serial_clock_update >= SERIAL_CLOCK_REFRESH_RATE || force) {
      serial_clock_update = millis();
      Serial.print(hour());
      printDigits(minute());
      printDigits(second());
      Serial.print(' ');
      Serial.print(day());
      Serial.print(' ');
      Serial.print(month());
      Serial.print(' ');
      Serial.print(year()); 
      Serial.println(); 
    }
  }
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(':');
  if(digits < 10)
      Serial.print('0');
  Serial.print(digits);
}

/*
void testMessage() {
  Serial.println("Displaying test message: 01:23:45");
  frame test_message;
  // add 0
    test_message.frame[ 0] = font_numbers[0][0];
    test_message.frame[ 1] = font_numbers[0][1];
    test_message.frame[ 2] = font_numbers[0][2];
    test_message.frame[ 3] = font_numbers[0][3];
    test_message.frame[ 4] = font_numbers[0][4];
    test_message.frame[ 5] = 0B00000000;

  // add 1
    test_message.frame[ 6] = font_numbers[1][0];
    test_message.frame[ 7] = font_numbers[1][1];
    test_message.frame[ 8] = font_numbers[1][2];
    test_message.frame[ 9] = font_numbers[1][3];
    test_message.frame[10] = font_numbers[1][4];
    test_message.frame[11] = 0B00000000;

  // add :
    test_message.frame[12] = font_symbols[0][0];
    test_message.frame[13] = font_symbols[0][1];
    test_message.frame[14] = 0B00000000;

  // add 2
    test_message.frame[15] = font_numbers[2][0];
    test_message.frame[16] = font_numbers[2][1];
    test_message.frame[17] = font_numbers[2][2];
    test_message.frame[18] = font_numbers[2][3];
    test_message.frame[19] = font_numbers[2][4];
    test_message.frame[20] = 0B00000000;

  // add 3
    test_message.frame[21] = font_numbers[3][0];
    test_message.frame[22] = font_numbers[3][1];
    test_message.frame[23] = font_numbers[3][2];
    test_message.frame[24] = font_numbers[3][3];
    test_message.frame[25] = font_numbers[3][4];
    test_message.frame[26] = 0B00000000;

  // add :
    test_message.frame[27] = font_symbols[0][0];
    test_message.frame[28] = font_symbols[0][1];
    test_message.frame[29] = 0B00000000;

  // add 4
    test_message.frame[30] = font_numbers[4][0];
    test_message.frame[31] = font_numbers[4][1];
    test_message.frame[32] = font_numbers[4][2];
    test_message.frame[33] = font_numbers[4][3];
    test_message.frame[34] = font_numbers[4][4];
    test_message.frame[35] = 0B00000000;

  // add 5
    test_message.frame[36] = font_numbers[5][0];
    test_message.frame[37] = font_numbers[5][1];
    test_message.frame[38] = font_numbers[5][2];
    test_message.frame[39] = font_numbers[5][3];
    test_message.frame[40] = font_numbers[5][4];

  test_message.length = 41;

  //Serial.println("Current Frame:");
  for (int i = 0; i < MATRIX_WIDTH; ++i) {
    //Serial.println(frame_temp[i], BIN);
    frame_current.frame[i] = test_message.frame[i];
  }
  //Serial.println();
  updateFrame();
}
*/

/*  // Old loop, retaining for history
void loop() {
  //Serial.print("Loop time [us]: "); Serial.println(toc-tic);
  //tic = micros();
  
  //Serial.print("Running LED: "); Serial.println(matrix_id);
  for (int i = 0; i < NUM_LEDS; ++i) {
    #ifdef MATRIX_COLUMNS
      if (i % MATRIX_WIDTH == matrix_column) {
        leds[i] = CRGB::Red;
      } else {
        leds[i] = CRGB::Black;
        //leds[i] = CHSV( hue, 255, 255);
      }
    #endif

    #ifdef MATRIX_ID
      if (i == matrix_id) {
        leds[i] = CRGB::Red;
      } else {
        leds[i] = CRGB::Black;
      }
    #endif

    #ifdef MATRIX_RANDOM
      int irand = (int)random(1000);
      if (irand> 500) {
        leds[i] = CRGB::Red;
      } else {
        leds[i] = CRGB::Black;
      }
    #endif
  }

  #ifdef MATRIX_COLUMNS
    matrix_column = matrix_column + matrix_direction;
    if (matrix_column >= MATRIX_WIDTH || matrix_column < 0) {
      matrix_direction = -1;
      matrix_column = MATRIX_WIDTH - 1;
    } else if (matrix_column <= 0) {
      matrix_direction = 1;
      matrix_column = 0;
    }
  #endif

  #ifdef MATRIX_ID
    matrix_id++;
    if (matrix_id >= NUM_LEDS) {
      matrix_id = 0;
    }
  #endif

  FastLED.show();
  //toc = micros();
  //delay(16);
}
  */