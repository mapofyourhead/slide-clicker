#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>

// USB/Bluetooth HID key numbers (these are not ASCII characters).
constexpr uint8_t KEY_LEFT_ARROW = 0x50;
constexpr uint8_t KEY_RIGHT_ARROW = 0x4F;
constexpr uint8_t KEY_F5 = 0x3E;
constexpr uint8_t KEY_HOME = 0x4A;
constexpr uint8_t KEY_RETURN = 0x28;
constexpr uint8_t KEY_B = 0x05;
constexpr uint8_t CTRL_MODIFIER = 0x01;

// Keeps the Bluetooth protocol details separate from the button logic.
class BluetoothKeyboard {
 public:
  void begin() {
    Serial.println("Starting Bluetooth...");
    if (!BLEDevice::init("Slide Clicker")) {
      Serial.println("ERROR: Bluetooth initialization failed");
      return;
    }
    BLESecurity::setCapability(ESP_IO_CAP_NONE);
    BLESecurity::setAuthenticationMode(true, false, true);
    BLESecurity::setInitEncryptionKey();
    BLESecurity::setRespEncryptionKey();
    BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

    server = BLEDevice::createServer();
    server->advertiseOnDisconnect(true);
    auto *hid = new BLEHIDDevice(server);
    // manufacturer() creates the optional characteristic before we set its value.
    hid->manufacturer()->setValue("DIY Slide Clicker");
    hid->hidInfo(0, 1);

    // Standard keyboard report: modifiers, reserved byte, six key slots.
    static uint8_t reportMap[] = {
        0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x01,
        0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00,
        0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
        0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
        0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01,
        0x29, 0x05, 0x91, 0x02, 0x95, 0x01, 0x75, 0x03,
        0x91, 0x01, 0x95, 0x06, 0x75, 0x08, 0x15, 0x00,
        0x25, 0x65, 0x05, 0x07, 0x19, 0x00, 0x29, 0x65,
        0x81, 0x00, 0xC0};
    hid->reportMap(reportMap, sizeof(reportMap));
    input = hid->inputReport(1);
    hid->outputReport(1);  // Keyboard LED output, required by some hosts.
    uint8_t released[8] = {};
    input->setValue(released, sizeof(released));
    hid->startServices();

    auto *advertising = BLEDevice::getAdvertising();
    // Put all discovery information in one packet (26 bytes, limit is 31).
    // This avoids depending on a second scan-response packet for the name.
    BLEAdvertisementData discovery;
    discovery.setFlags(0x06);  // General discoverable, Bluetooth LE only.
    discovery.setName("Slide Clicker");
    discovery.setAppearance(HID_KEYBOARD);
    discovery.setCompleteServices(BLEUUID(uint16_t(0x1812)));
    advertising->setScanResponse(false);
    if (!advertising->setAdvertisementData(discovery)) {
      Serial.println("ERROR: Could not configure Bluetooth advertising");
      return;
    }
    advertisingStarted = advertising->start();
    Serial.println(advertisingStarted ? "Bluetooth advertising requested: Slide Clicker"
                                      : "ERROR: Bluetooth advertising failed to start");
  }

  bool isConnected() const {
    return server && server->getConnectedCount() > 0;
  }

  bool isReady() const { return advertisingStarted; }

  void write(uint8_t key, uint8_t modifiers = 0) {
    if (!isConnected()) return;  // Never queue actions while disconnected.
    uint8_t report[8] = {modifiers, 0, key, 0, 0, 0, 0, 0};
    input->setValue(report, sizeof(report));
    input->notify();
    delay(30);
    memset(report, 0, sizeof(report));
    input->setValue(report, sizeof(report));
    if (isConnected()) input->notify();  // Release all keys.
  }

 private:
  class SecurityCallbacks : public BLESecurityCallbacks {
    bool onSecurityRequest() override { return true; }
  };
  BLEServer *server = nullptr;
  BLECharacteristic *input = nullptr;
  bool advertisingStarted = false;
};
