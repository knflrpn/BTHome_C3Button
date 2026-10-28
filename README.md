# ESP32-C3 BTHome Button Remote

This project configures a generic ESP32-C3 Super Mini as an ultra-low power, 6-button remote using ESPHome for use with BTHome in Home Assistant.

## Hardware Modifications

If powering with a battery, one physical modification to the board is required and one is recommended. Refer to the reference image below for the component locations:

![Super Mini Board](/documentation/SuperMiniBoard.jpg)

* **Power LED:** Snip off or desolder the power LED to stop it from drawing continuous current.
* **Diode:** If you have the ability to, remove *and short* the diode. If you cannot short it, it can be left as-is -- it will just slightly reduce the amount of charge you can get out of the battery before the voltage gets too low for the processor.

## Buttons

The ESP32-C3 can be woken from sleep with GPIOs 0-5, so this project uses all of those pins for buttons.  The buttons are named `button_1` through `button_6`, so keep in mind that the indices are +1.  Buttons need to be normally-open and should connect the board's pin to ground when closed.

## Pull-up Resistors

The ESP32's internal pull-ups are used, but in my testing I have found that the pull-up on GPIO0 is insufficient, causing it to trigger when other buttons are pressed.  This can be fixed by using an external pull-up resistor (to 3.3-V).  An appropriate resistance is 10k.  These pull-ups can be added to all pins if desired.

## Battery Power

The board can be powered by connecting a LiPo battery directly to VBUS (labeled `5V`) on the board.  **Only when USB is NOT connected**!  Never connect both a battery and USB at the same time!

My board draws <10&nbsp;μA during sleep and consumes ~0.075&nbsp;C per button press.  Based on that, an 18650 battery should last around a year or 100k button presses.  Real-world performance might be different but battery life is expected to be good.

## Firmware Configuration

The primary firmware configuration is defined in the `bthome_c3button.yaml` file.  The files in `/components/bthome_c3button/` provide an interface with the hardware.  Put all these files (including the directory structure) in your esphome directory.

## Maintenance Mode for OTA Updates

By default, WiFi is disabled. To flash updates over the air, enter Maintenance Mode by waking the board using both button 1 (GPIO 0) and button 6 (GPIO 5) at the same time.  That will enable WiFi and keep the board awake.  The board will stay awake until getting reset, so if you do not install OTA firmware (which will reset the board), you will need to manually reset the board.

## Setup in Home Assistant

You need Bluetooth, which is usually done in Home Assistant with a Bluetooth proxy.  This assumes you have already set that up.

You need BTHome (In `Devices and services` in Home Assistant).  This assumes you have already installed that.

After flashing the Super Mini board with the firmware, press one of the buttons.  The board should automatically be detected by BTHome, which will forward it to Home Assistant.

It will show up in Home Assistant as `C3 Btn WXYZ` (`WXYZ` will change based on your board).  The events that it emits will be named `Button 1` through `Button 6`.

# AI Disclaimer

Much of the code was written by an LLM.  A human did physical testing to guide debugging and iteration.  A human reviewed the code at the end and added finishing touches.