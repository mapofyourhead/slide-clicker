# Slide Clicker

A three-button presentation remote built with an ESP32-S3 NeuroBoard. It connects to a computer as a Bluetooth Low Energy (BLE) keyboard and sends shortcuts for Google Slides on Windows.

**Bluetooth is the main version of this project**, on the `bluetooth` branch. The `usb` branch preserves the earlier wired USB keyboard version. The branches have different firmware and configuration files.

## Controls

| Button | GPIO | Short press | Hold for 700 ms |
| --- | --- | --- | --- |
| Left | 7 | Previous slide: Left Arrow | First slide: Home |
| Middle | 6 | Start slideshow: Ctrl+F5 | Show/clear black screen: B |
| Right | 5 | Next slide: Right Arrow | Activate focused item: Enter |

Keep the Google Slides window selected. First-slide navigation and the black-screen action are intended for presentation mode. The middle short press starts presentation mode from the editor.

The right hold sends **Enter**, not a mouse click. A link, video, or control must already have keyboard focus for Enter to activate it. The clicker cannot automatically find media on the slide. Other presentation apps may need different shortcuts.

A short press triggers when the button is released. A hold triggers once after 700 ms and does not also trigger the short action on release. Button inputs are debounced for 30 ms to avoid counting contact bounce as extra presses.

## Hardware and wiring

- ESP32-S3 NeuroBoard
- Three normally open momentary pushbuttons
- Jumper wires and a breadboard or soldered connections
- USB data cable for programming and initial power
- Computer with Bluetooth LE keyboard support

Connect each button between its GPIO and the board's GND:

```text
GPIO7 ---- left button ------+
GPIO6 ---- middle button ----+---- ESP32 GND
GPIO5 ---- right button -----+
```

The code enables `INPUT_PULLUP`, so external pull-up resistors are not needed for this short-wire setup:

- Released = HIGH / 1
- Pressed = LOW / 0

For four-legged buttons, choose terminals that connect only when pressed. Some pairs of legs are permanently connected internally.

Bluetooth carries the keyboard commands; USB can still provide power and serial debugging. For cordless operation, provide a suitable power source through the board's supported power input. Battery charging and battery-level measurement are not implemented in this project.

## Software setup

The firmware is written in **C++ with the Arduino framework** and built using **PlatformIO** in VS Code. Bluetooth HID uses the BLE library included with the ESP32 Arduino framework; no separate BLE keyboard library is required.

1. Install VS Code and the PlatformIO IDE extension.
2. Clone the repository and open its project folder.
3. Select the main Bluetooth version:

   ```powershell
   git switch bluetooth
   ```

4. Install the custom `NeuroBoardS3` board definition if it is not already available. The current project relies on `NeuroBoardS3.json` in PlatformIO's user `boards` directory, typically `%USERPROFILE%\.platformio\boards\` on Windows. This definition is **not bundled in this repository**. The board's upstream definition is available in the [ECE NeuroBoard-S3 repository](https://github.com/ECE-Ecole-Ingenieurs/elec-carte-neuroboard-s3/blob/main/software/NeuroBoardS3.json).
5. Connect the board using a USB data cable.
6. Build and upload with PlatformIO's toolbar buttons, or run these commands from the project folder:

   ```powershell
   pio run
   pio run -t upload
   ```

The project targets `board = NeuroBoardS3` with `framework = arduino`. Development builds used Espressif32 platform 8.0.0 and Arduino-ESP32 3.3.12; the platform version is currently not pinned in `platformio.ini`.

## Pairing and use

1. After uploading, press **RST once without holding BOOT** if the program does not start automatically.
2. Open the computer's Bluetooth **Add device** screen.
3. Select **Slide Clicker** and complete pairing. The firmware uses pairing without PIN entry and requests bonding to save pairing credentials.
4. Open Google Slides, select its window, and test the controls.

No Wi-Fi network, internet connection, or receiver is required for the Bluetooth connection. The firmware resumes advertising after disconnection. Button actions while disconnected are discarded instead of being queued.

## Project layout

```text
src/main.cpp                 Button wiring, debounce, tap/hold timing and actions
include/BluetoothKeyboard.h  BLE keyboard setup, advertising and key reports
platformio.ini              Board, framework and USB serial configuration
```

In `main.cpp`, `setup()` configures the buttons and starts Bluetooth. `loop()` repeatedly reads the buttons, accepts stable changes after the debounce interval, and distinguishes taps from holds. `shortPress()` and `longPress()` select the keyboard commands.

`BluetoothKeyboard.h` advertises the name **Slide Clicker** and the HID keyboard service. Each command sends a key report followed by an empty report that releases the keys.

To adjust sensitivity, change `debounceMs` or `longPressMs` near the top of `main.cpp`. GPIO assignments are in the `buttons[]` array.

## Serial debugging and upload recovery

Open PlatformIO Serial Monitor at **115200 baud**:

```powershell
pio device monitor
```

The firmware prints Bluetooth initialization results, connection changes, button actions, and a status message every five seconds. A successful advertising request is a startup diagnostic; pairing and keyboard operation still need to be checked on the host computer.

### No upload port or failed connection

Close Serial Monitor before uploading. List detected ports with:

```powershell
pio device list
```

If needed, enter download mode manually:

1. Hold **BOOT**.
2. Press and release **RST**.
3. Release **BOOT**.
4. Upload again, selecting the detected COM port if requested.

For example, if the detected port is COM5:

```powershell
pio run -t upload --upload-port COM5
```

COM numbers can change. Use the detected value rather than assuming COM5 is always correct. After upload, press **RST alone** to run the firmware. Download mode does not run the clicker program or advertise Bluetooth.

### Bluetooth device does not appear

- Confirm that the **Bluetooth firmware was uploaded**. Changing Git branches only changes files on the computer.
- Keep the board powered and press RST without BOOT.
- Check Serial Monitor for startup errors and repeated resets.
- If there is an old failed pairing, remove it from the host's Bluetooth settings and retry pairing.

### Button always reads as pressed

Disconnect its GPIO wire from the button. With `INPUT_PULLUP`, the input should return to HIGH. Check for a short to GND or a pair of button terminals that are permanently joined.

## USB version

Save and commit your edits before changing branches:

```powershell
git switch usb
```

Build and upload that branch to use the wired USB keyboard. Its controls reflect the earlier implementation and may differ from the Bluetooth version as the project evolves.

To return to Bluetooth:

```powershell
git switch bluetooth
```

Upload again to change the firmware running on the board.
