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
#define PIXEL_COUNT 100
#endif

#if defined(TARGET_wokwi)
#define PIXEL_COUNT 18
#define LED_DATA 9
#define COLOR_ORDER BGR
#endif

CRGB pixels[PIXEL_COUNT];
int BASE_COLOR = 0x6666ff;
byte BASE_HUE = 240;

struct fading_pixel {
  int id = -1;
  byte val = 255;
  int8_t step = 0;
  bool step_downward = true;
};

byte FADE_MAX = 255;
byte FADE_MIN = 150;
byte FADE_STEPS[] = {1, 2, 3, 5, 8, 13};
#define FADE_NEW_PIXEL_CHANCE 1
#define FADING_PIXEL_COUNT PIXEL_COUNT
fading_pixel FADING_PIXELS[FADING_PIXEL_COUNT];

Scheduler scheduler;
void next_frame();
Task task_next_frame( 1000/FRAMES_PER_SECOND * TASK_MILLISECOND , TASK_FOREVER, &next_frame );

void fade_pixels();
void start_fade(int fading_pixel_id, int pixel_id);
int random_pixel();

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting up..."));
  #if defined(CHIPSET_WS2812B)
  FastLED.addLeds<WS2812B, LED_DATA, COLOR_ORDER>(pixels, PIXEL_COUNT).setCorrection(TypicalLEDStrip);
  #elif defined(CHIPSET_APA102)
  FastLED.addLeds<APA102, LED_DATA, LED_CLK, COLOR_ORDER>(pixels, PIXEL_COUNT).setCorrection( TypicalLEDStrip );
  #endif

  FastLED.setBrightness(MAX_BRIGHTNESS);

  for (int i = 0; i < PIXEL_COUNT; i++) {
    pixels[i] = CHSV(BASE_HUE, 255, 255);
  }
  FastLED.show();

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

  for (int i = 0; i < FADING_PIXEL_COUNT; i++) {
    fading_pixel &pixel = FADING_PIXELS[i];
    bool stop_fade = false;

    if (pixel.id == -1) {
      // This slot does not contain an active animation.
      // See if we should start a new animation on a random pixel
      if (random(100) > FADE_NEW_PIXEL_CHANCE) {
        // Not starting a new fade this round
        continue;
      }

      int new_pixel_id = random_pixel();

      // Check to see if it is already animating in another slot
      bool found = false;
      for (int j = 0; j < FADING_PIXEL_COUNT; j++) {
        if (FADING_PIXELS[j].id == new_pixel_id) {
          found = true;
          break;
        }
      }
      if (found) {
        // Serial.println("Pixel already fading; skipping");
        continue;
      } else {
        start_fade(i, new_pixel_id);
      }
    }

    step_width = FADE_STEPS[pixel.step];
    // Serial.printf("Initial val %d, step %d, width %d\r\n", pixel.val, pixel.step, step_width);

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
        // We are at the dimmest point
        new_val = FADE_MIN;
        pixel.step = 0;
        pixel.step_downward = false;
      }
    } else {
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
        // We've returned to full brightness
        new_val = FADE_MAX;
        pixel.step = 0;
        pixel.step_downward = true;
        stop_fade = true;
      }
    }

    // Serial.printf("Set pixel %d %d -> %d (step %d, width %d)\r\n", pixel.id, pixel.val, new_val, pixel.step, step_width);
    pixel.val = new_val;
    pixel.step = min(sizeof(FADE_STEPS)/sizeof(byte) - 1, (uint)pixel.step + 1); // try to speed up
    pixels[pixel.id] = CHSV(BASE_COLOR, 255, new_val);
    if (stop_fade) {
      // Serial.printf("Stopping fade for pixel %d\r\n", pixel.id);
      pixel.id = -1;  // Stop dimming this pixel
    }
    // Serial.println("");
  }
}

void start_fade(int fading_pixel_id, int pixel_id) {
  Serial.printf("Starting fade for pixel %d @ %d\r\n", pixel_id, millis());
  FADING_PIXELS[fading_pixel_id].id = pixel_id;
  FADING_PIXELS[fading_pixel_id].step = 0;
  FADING_PIXELS[fading_pixel_id].step_downward = true;
}

int random_pixel() {
  return rand() % PIXEL_COUNT;
}
