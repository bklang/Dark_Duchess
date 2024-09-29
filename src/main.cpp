#include <Arduino.h>
#include <FastLED.h>
#include <TaskScheduler.h>

#if !defined(TARGET_dark_duchess) && !defined(TARGET_wokwi)
#error "Must define a target board (eg. TARGET_wokwi)"
#endif

#define MAX_BRIGHTNESS 150
#define FRAMES_PER_SECOND 160
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
  uint8_t step = 0;
};

byte FADE_MAX = 255;
byte FADE_MIN = 150;
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
  for (int i = 0; i < FADING_PIXEL_COUNT; i++) {
    fading_pixel &pixel = FADING_PIXELS[i];

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

    uint8_t wave_point = cubicwave8(pixel.step++);
    // The wave starts & ends at zero, so subtract from max to invert it
    wave_point = map8(255 - wave_point, FADE_MIN, FADE_MAX);  //map from 0, 255 to min and maxBreath;

    pixels[pixel.id] = CHSV(BASE_HUE, 255, wave_point);

    if (pixel.step >= 255) {
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
}

int random_pixel() {
  return rand() % PIXEL_COUNT;
}
