#include <Arduino.h>
#include "BluetoothKeyboard.h"

BluetoothKeyboard keyboard;

constexpr unsigned long debounceMs = 30;
constexpr unsigned long longPressMs = 700;

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
  keyboard.write(KEY_F5, CTRL_MODIFIER);
}

void shortPress(const Button &button) {
  if (!keyboard.isConnected()) return;
  if (button.key == KEY_F5) {
    startSlideshow();
  } else {
    keyboard.write(button.key);
  }
  Serial.println(button.name);
}

void longPress(const Button &button) {
  if (!keyboard.isConnected()) return;
  if (button.key == KEY_LEFT_ARROW) {
    keyboard.write(KEY_HOME);
    Serial.println("First slide");
  } else if (button.key == KEY_F5) {
    keyboard.write(KEY_B);
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
  if (keyboard.isReady()) {
    Serial.println("Pair with Slide Clicker in your computer's Bluetooth settings.");
  }
}

void loop() {
  const unsigned long now = millis();
  static bool wasConnected = false;
  const bool connected = keyboard.isConnected();
  static unsigned long lastStatusMs = 0;
  if (now - lastStatusMs >= 5000) {
    lastStatusMs = now;
    Serial.println(connected ? "Bluetooth connected" :
        (keyboard.isReady() ? "Bluetooth started; waiting for a connection to Slide Clicker"
                           : "ERROR: Bluetooth startup failed; reset and check startup messages"));
  }
  if (connected != wasConnected) {
    Serial.println(connected ? "Bluetooth connected" : "Bluetooth disconnected");
    wasConnected = connected;
    // Do not carry an unfinished press across a connection change.
    for (auto &button : buttons) button.trackingPress = false;
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
