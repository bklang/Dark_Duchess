#include <Arduino.h>
#include <FastLED.h>
#include <TaskScheduler.h>

#if !defined(TARGET_dark_duchess) && !defined(TARGET_wokwi)
#error "Must define a target board (eg. TARGET_wokwi)"
#endif

#define MAX_BRIGHTNESS 150
#define FRAMES_PER_SECOND 20
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
  int id;
  byte val = 255;
  int8_t step = 0;
  bool step_downward = true;
};

fading_pixel FADING_PIXELS[1];

byte FADE_MAX = 255;
byte FADE_MIN = 150;
byte FADE_STEPS[] = {1, 2, 3, 5, 8, 13};

Scheduler scheduler;
void next_frame();
Task task_next_frame( 1000/FRAMES_PER_SECOND * TASK_MILLISECOND , TASK_FOREVER, &next_frame );

void fade_pixels();
void start_fade(int pixel);
int random_pixel();

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting up..."));
  #if defined(CHIPSET_WS2812B)
  FastLED.addLeds<WS2812B, LED_DATA, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  #elif defined(CHIPSET_APA102)
  FastLED.addLeds<APA102, LED_DATA, LED_CLK, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  #endif

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(BASE_HUE, 255, 255);
  }
  FastLED.show();

  start_fade(1);

  scheduler.addTask(task_next_frame);
  task_next_frame.enable();
}

void loop() {
  scheduler.execute();
}

void next_frame() {
  fade_pixels();
  FastLED.show();
}

void fade_pixels() {
  byte new_val;
  byte step_width;

  fading_pixel &pixel = FADING_PIXELS[0];
  step_width = FADE_STEPS[pixel.step];
  // Serial.printf("Initial val %d, step %d, width %d\r\n", pixel.val, pixel.step, step_width);

  if (!pixel.step_downward) {
    // brightening
    while (pixel.val + (step_width * 5) > FADE_MAX) {
      // We are getting close to the end, slow down the step
      // Serial.printf("Slowing brightening down from step %d - val: %d width: %d\r\n", pixel.step, pixel.val, step_width);
      pixel.step = max(0, pixel.step - 1);
      step_width = FADE_STEPS[pixel.step];
      if (pixel.step == 0) break; // Can't slow down any more
    }
    new_val = pixel.val + step_width;
    if (new_val >= FADE_MAX) {
      new_val = FADE_MAX;
      pixel.step = 0;
      pixel.step_downward = true;
    }
  }
  if (pixel.step_downward) {
    // dimming
    while (pixel.val - (step_width * 5) < FADE_MIN) {
      // We are getting close to the end, slow down the step
      // Serial.printf("Slowing dimming down from step %d - val: %d width: %d\r\n", pixel.step, pixel.val, step_width);
      pixel.step = max(0, pixel.step - 1);
      step_width = FADE_STEPS[pixel.step];
      if (pixel.step == 0) break; // Can't slow down any more
    }
    new_val = pixel.val - step_width;
    if (new_val <= FADE_MIN) {
      new_val = FADE_MIN;
      pixel.step = 0;
      pixel.step_downward = false;
    }
  }

  leds[pixel.id] = CHSV(BASE_COLOR, 255, new_val);
  pixel.val = new_val;
  pixel.step = min(sizeof(FADE_STEPS)/sizeof(byte) - 1, (uint)pixel.step + 1); // try to speed up
  // Serial.printf("Set pixel to val %d, step %d, width %d\r\n", pixel.val, pixel.step, step_width);
  // Serial.println("");
}

void start_fade(int pixel_id) {
  Serial.printf("Starting fade for pixel %d\r\n", pixel_id);
  FADING_PIXELS[0].id = pixel_id;
  FADING_PIXELS[0].step = 0;
  FADING_PIXELS[0].step_downward = true;
}

int random_pixel() {
  return 1;
  return rand() % NUM_LEDS;
}
