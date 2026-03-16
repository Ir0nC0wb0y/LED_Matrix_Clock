#include <Arduino.h>
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
//#define FASTLED_FORCE_SOFTWARE_SPI
#define FASTLED_ESP32_LCD_DRIVER
#include <FastLED.h>

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

unsigned long tic = 0;
unsigned long toc = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

  // This is used so that you can see if PSRAM is enabled. If not, we will crash in setup() or in loop().
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
  FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA_1>(leds,            0, MATRIX_WIDTH);
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

  FastLED.setBrightness( 64 );
  FastLED.setMaxPowerInVoltsAndMilliamps(5,500);

  Serial.println("Setup Complete, moving into loop");
}

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