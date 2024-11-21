# URIM - Universal Serial Bus Replication and Injection Medium
**URIM** is an Arduino firmware that serves as a controlled USB mouse emulator. It works by receiving designated instructions from the host for mouse actions through the USB HID protocol and executing these actions while acting as an actual USB HID Mouse. **URIM** firmware is also capable of interpreting 16-bit data from the mouse and replicating it to the computer.

A unique feature of URIM is its ability to receive external UART command injections, enabling real-time control and manipulation of mouse actions via a secondary chip (CH340). This allows for integration with other devices or systems for custom input manipulation.

USB-RIM also includes a vendor-defined interface to perform IOCTL-based transmit (TX) and receive (RX) transfers. These transfers are utilized to retrieve the state of external keys. This functionality has been implemented in the rk-mouse API under the GetAsyncKeyState() method, which allows for asynchronous polling of key states via IOCTL requests.

# How is URIM different?
The main difference between **URIM** firmware and other mouse replication firmware is that **URIM** can programmatically fetch and parse the report descriptor for any 16-bit or 8-bit mouse connected to the host.

## Key Features
```sh-session
Replication
- 980hz polling rate
- Automatic descriptor fetch
- Automatic interface enumeration
- Support for both 16 bit and 8 bit mice
- External key states via vendor defined IOCTL requests
- Custom ping features

UART Command Injection
- Support external UART command injection via a second chip (ch340)
```
>[!IMPORTANT]
> The Caterina bootloader is required for UART Command Injection (UCI)

The system utilizes two HID interfaces. One interface belongs to the HID Mouse class, while the other involves a Generic HID class `Raw HID`, primarily functioning as an output from host to relay mouse reports via `Raw HID` to Arduino and dictate mouse behaviors like movements and button clicks.

## Purpose
**URIM** serves a distinct purpose: **externally** mimicking a mouse device to execute mouse actions while being controlled by the host system. In environments where automated hardware testing is required, URIM can be used to simulate and control mouse movements without direct user interaction. Its ability to replicate exact movements allows for repeatable and precise testing across different systems.
The UART injection feature allows remote systems to send mouse control commands to URIM.
```sh-session
[USB Mouse] --> [USB Host Shield 2.0] --> [Arduino Leonardo] 
                           ^                               |
                           |                               |
                           +                               |
                           |                               |
                           |                         [Inject Inputs] <-- [Second PC]
                           |                               |
                           v                               |
                        [Firmware]                         |
                           |                               |
   +----------Parse Descriptor---------+                   |
   |           |            |          |                   |
   v           v            v          v                   |
[Buttons]    [Axes]      [Wheel]   [Mouse Inputs]          |
   |           |            |          |                   |
   v           v            v          v                   |
[Process] --> [Process] --> [Process]                      |
                          +------------+                   |
                                       |                   |
                                       |                   v
                                   [EMULATED HID USB] <----+
                                       |
                                       v
                                   [Host PC]

```
## Hardware Requirements
```sh-session
- Arduino Leonardo
- USB Host Shield 2.0
```
## Client Dependencies
USB-RIM client relies on rkMouseApi class which is reponsible for providing a simple Mouse API and forwarding the instructions to external MCU. USB-RIM depends on [hidapi](https://github.com/libusb/hidapi) library to interface with Arduino over USB HID.

# Firmware Setup
## 1. Install Dependencies (ARDUINO CLI)
```sh-session
core install arduino:avr@1.8.6
lib install "USB Host Shield Library 2.0"
```


# Credits
<br>
[hidapi](https://github.com/libusb/hidapi) - Very useful HID library
<br>
[rawhid](https://github.com/NicoHood/HID/blob/master/src/SingleReport/RawHID.cpp)
