#include <Arduino.h>
#include <FastLED.h>


#define MATRIX_WIDTH  48
#define MATRIX_HEIGHT  4
#define NUM_LEDS (MATRIX_HEIGHT * MATRIX_WIDTH)

#define LED_CHIPSET WS2812B
#define LED_COLOR_ORDER GRB

#define PIN_LED_DATA_1 D1
#define PIN_LED_DATA_2 D2
#define PIN_LED_DATA_3 D3
#define PIN_LED_DATA_4 D4

CRGB leds[MATRIX_WIDTH * MATRIX_HEIGHT];

int matrix_column = 0;
int matrix_direction = 1;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting Matrix");

  FastLED.addLeds<NEOPIXEL, PIN_LED_DATA_1>(leds, MATRIX_WIDTH);
  FastLED.addLeds<NEOPIXEL, PIN_LED_DATA_2>(leds, MATRIX_WIDTH);
  FastLED.addLeds<NEOPIXEL, PIN_LED_DATA_3>(leds, MATRIX_WIDTH);
  FastLED.addLeds<NEOPIXEL, PIN_LED_DATA_4>(leds, MATRIX_WIDTH);

  FastLED.setBrightness( 64 );
  FastLED.setMaxPowerInVoltsAndMilliamps(5,500);
}

void loop() {
  //Serial.print("Displaying column "); Serial.println(matrix_column);

  //int32_t hue = ((int32_t)cos16( millis() * (27/1) ) * (350 / MATRIX_WIDTH));
  for (int i = 0; i < NUM_LEDS; ++i) {
    //Serial.print("i % MATRIX_WIDTH : "); Serial.print(i); Serial.print(" & "); Serial.print(MATRIX_WIDTH); Serial.print(" = "); Serial.print(i % MATRIX_WIDTH); Serial.print(" "); Serial.println((i % MATRIX_WIDTH == matrix_column) ? "True": "False");
    if (i % MATRIX_WIDTH == matrix_column) {
      leds[i] = CRGB::Red;
    } else {
      leds[i] = CRGB::Black;
      //leds[i] = CHSV( hue, 255, 255);
    }
  }
  matrix_column = matrix_column + matrix_direction;
  if (matrix_column >= MATRIX_WIDTH || matrix_column < 0) {
    matrix_direction = -1;
    matrix_column = MATRIX_WIDTH - 1;
  } else if (matrix_column <= 0) {
    matrix_direction = 1;
    matrix_column = 0;
  }
  FastLED.show();

  delayMicroseconds(20833);
}