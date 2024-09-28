#include <Arduino.h>
#include <FastLED.h>
#include <TaskScheduler.h>

#if !defined(TARGET_dark_duchess) && !defined(TARGET_wokwi)
#error "Must define a target board (eg. TARGET_wokwi)"
#endif

#define MAX_BRIGHTNESS 150
#define FRAMES_PER_SECOND 60
#define CHIPSET_WS2812B true

#if defined(TARGET_dark_duchess)
#define NUM_LEDS 100
#endif

#if defined(TARGET_wokwi)
#define NUM_LEDS 13
#define LED_DATA 9
#define COLOR_ORDER BGR
#endif

CRGB leds[NUM_LEDS];
int BASE_COLOR = 0x6666ff;
byte BASE_HUE = 240;

struct fading_pixel {
  int pixel;
  byte val = 255;
};

fading_pixel FADING_PIXELS[1];
int GLOWING_PIXELS[NUM_LEDS/10];

Scheduler scheduler;
void next_frame();
Task task_next_frame( 1000/FRAMES_PER_SECOND * TASK_MILLISECOND , TASK_FOREVER, &next_frame );

void fade_pixels();
int random_pixel();

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting up..."));
  #if defined(CHIPSET_WS2812B)
  FastLED.addLeds<WS2812B, LED_DATA, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  #elif defined(CHIPSET_APA102)
  FastLED.addLeds<APA102, LED_DATA, LED_CLK, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  #endif

  FADING_PIXELS[0].pixel = 1;
  FADING_PIXELS[0].val = 255;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(BASE_HUE, 255, 255);
  }
  FastLED.show();

  scheduler.addTask(task_next_frame);
  task_next_frame.enable();
}

void loop() {
  scheduler.execute();
}

void next_frame() {
  int pixel = random_pixel();
  leds[pixel] = CHSV(BASE_HUE, 255, 50);
  // for (int i = 0; i < NUM_LEDS; i++) {
  //   leds[i] = CHSV(random8(),255,255);
  // }
  // FastLED.show();
  // delay(500);
  // leds[pixel] = CHSV(BASE_HUE, 255, 255);
  fade_pixels();
  FastLED.show();
}

void fade_pixels() {
  byte new_val = FADING_PIXELS[0].val - 2;
  leds[FADING_PIXELS[0].pixel] = CHSV(BASE_COLOR, 255, new_val);
  FADING_PIXELS[0].val = new_val;
}

int random_pixel() {
  return 1;
  return rand() % NUM_LEDS;
}
