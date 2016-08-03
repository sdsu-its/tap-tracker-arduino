Tap Tracker - Arduino
=====================

Tap Tracker is a IoT Business Event Management System that allows events (from
clients, like the one described in this Repo) to log events to the centralized
server. The server then aggregates the data, which can be presented in a variety
of forms.

## Components
The Tap Tracker is based on the [Adafruit HUZZAH ESP8266](https://www.adafruit.com/products/2471).
Other components include:
 - [Adafruit HUZZAH ESP8266 Breakout](https://www.adafruit.com/products/2471)
 - [USB Micro-B Breakout Board](https://www.adafruit.com/products/1833)
    + This is used to power the device once in its enclosure, this is only a
    recommendation and could be replaced by a variety of options.
 - [Monochrome 128x32 I2C OLED graphic display](https://www.adafruit.com/products/931)
 - A Button (Illuminated is best)
    + We are particularly fond of the [LED Illuminated Pushbutton - 30mm Square](https://www.adafruit.com/products/491),
    but any Illuminated Panel Mountable button would work well.
 - A Red/Green 2-Led LED
    + These work well with Illuminated Arcade Style pushbuttons which are designed
    for 2 lead LEDs. These aren't available from Adafruit, but are available at a
    variety of Distributors, like Mouser or DigiKey.
    + Sample Component: [Kingbright Through Hole RED/GREEN DIFFUSED 2-LEAD Standard LED](http://www.mouser.com/ProductDetail/Kingbright/WP57EGW)
 - Appropriate Resistor(s)
    + Depending on your LED/Button choice, you will need resistor(s) with
    appropriate values to your application.
 - Hook-Up Wire

A Serial Programmer is needed to program the HUZZAH, several tested options are
listed below.
 - [SparkFun FTDI Basic Breakout - 5V](https://www.sparkfun.com/products/9716)
 - [FTDI Serial TTL-232 USB Cable](https://www.adafruit.com/products/70)

## Software
The Arduino Client can be run in 2 different modes.
 - **Multi-Button Mode** - This mode allows 2 buttons to be connected to the
 HUZZAH, each reporting different events.
 - **Single Button Mode** (SBM) - This mode uses a single button to cycle through the
 event options, till the desired type is selected.

2 Examples, that you can use with your Tap Tracker instance, are provided.
- `TapTracker.ino` uses 2 buttons with RG 2-lead LEDs.
- `TapTrackkerSBM.ino` uses a single button to toggle through the different types.

### Configuration
**WiFi**
- `ssid` - Network SSID
- `pass` - WPA or WPA2 PSK

**Tap Tracker Server**
- `STATION_ID` - Unique Station Identifier (int); used to identify the device with the server
    + Make sure the Device ID is registered with the Tap Tracker Server
    + The form is located on the Web UI under `Devices > Create Device`
- `SERVER` - Server Domain Name, should not contain a '/'
- `CONTEXT` - Server Context, everything after the '/'

**NTP**
- `ntpServerName` - NTP Server to use for Time/Date Information
    + Info on the NTP Pool is available on their [website](http://www.pool.ntp.org/zone).
- `timeZone` - The Java String of the Timezone the device will be in.
    + This is used instead of an GMT offset, because of Daylight Savings.
    + [A list has been provided by Gary Gregory](https://garygregory.wordpress.com/2013/06/18/what-are-the-java-timezone-ids/)

**Hardware**
- `BTN1` - Pin to which Button 1 is connected
- `BTN2` - Pin to which Button 2 is connected, *Can be ignored in SMB*


- `B1R` - Pin to which the Red LED for Button 1 is connected
- `B1G` - Pin to which the Green LED for Button 1 is connected
- `B2R` - Pin to which the Red LED for Button 2 is connected, *Can be ignored in SMB*
- `B2G` - Pin to which the Green LED for Button 2 is connected, *Can be ignored in SMB*
