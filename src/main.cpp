#include <Arduino.h>
// Needed by the USB library's mass-storage sources in this framework version.
#include <FS.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

USBHIDKeyboard keyboard;

constexpr unsigned long debounceMs = 30;
constexpr unsigned long longPressMs = 700;
constexpr unsigned long slideshowLoadMs = 1500;
bool firstSlidePending = false;
unsigned long slideshowStartedMs = 0;

struct Button {
  uint8_t pin;
  const char *name;
  uint8_t key;
  int lastReading = HIGH;
  int stableState = HIGH;
  unsigned long lastChangeMs = 0;
  unsigned long pressedMs = 0;
  bool trackingPress = false;
  bool longPressHandled = false;
};

Button buttons[] = {
    {7, "Previous slide", KEY_LEFT_ARROW},
    {6, "Start slideshow", KEY_F5},
    {5, "Next slide", KEY_RIGHT_ARROW},
};

void startSlideshow() {
  keyboard.press(KEY_LEFT_CTRL);
  keyboard.press(KEY_F5);
  delay(20);
  keyboard.releaseAll();
}

void shortPress(const Button &button) {
  firstSlidePending = false;
  if (button.key == KEY_F5) {
    startSlideshow();
  } else {
    keyboard.write(button.key);
  }
  Serial.println(button.name);
}

void longPress(const Button &button) {
  firstSlidePending = false;
  if (button.key == KEY_LEFT_ARROW) {
    startSlideshow();
    // Allow Slides to open before sending Home; no browser feedback is available.
    slideshowStartedMs = millis();
    firstSlidePending = true;
    Serial.println("Start slideshow on first slide");
  } else if (button.key == KEY_F5) {
    keyboard.write('b');
    Serial.println("Toggle black screen");
  } else {
    // Activates a focused link/control or selected video, not an arbitrary item.
    keyboard.write(KEY_RETURN);
    Serial.println("Activate focused link/video (Enter)");
  }
}

void setup() {
  Serial.begin(115200);
  for (auto &button : buttons) {
    // Each button connects its GPIO to GND when pressed.
    pinMode(button.pin, INPUT_PULLUP);
    button.lastReading = digitalRead(button.pin);
    button.stableState = button.lastReading;
  }
  keyboard.begin();
  USB.begin();
}

void loop() {
  const unsigned long now = millis();
  if (firstSlidePending && now - slideshowStartedMs >= slideshowLoadMs) {
    keyboard.write(KEY_HOME);
    firstSlidePending = false;
  }
  for (auto &button : buttons) {
    const int reading = digitalRead(button.pin);
    if (reading != button.lastReading) {
      button.lastReading = reading;
      button.lastChangeMs = now;
    }

    if (now - button.lastChangeMs >= debounceMs &&
        reading != button.stableState) {
      button.stableState = reading;
      if (button.stableState == LOW) {
        button.pressedMs = now;
        button.trackingPress = true;
        button.longPressHandled = false;
      } else if (button.trackingPress) {
        // A hold must never also trigger the short-press action on release.
        if (!button.longPressHandled) {
          if (now - button.pressedMs >= longPressMs) {
            longPress(button);
          } else {
            shortPress(button);
          }
        }
        button.trackingPress = false;
      }
    }
    if (button.trackingPress && !button.longPressHandled &&
        button.stableState == LOW && reading == LOW &&
        now - button.pressedMs >= longPressMs) {
      button.longPressHandled = true;
      longPress(button);
    }
  }
  delay(1);
}
