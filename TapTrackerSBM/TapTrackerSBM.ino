#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

#include <RestClient.h>   // https://github.com/fabianofranca/ESP8266RestClient

// Display Libs
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ====================== BEGIN Config ======================

char ssid[] = "SSID";              //  your network SSID (name)
char pass[] = "WPA PSK";           // your network password

#define STATION_ID 1
#define SINGLE_BUTTON_MODE  // Uncomment for Single Button Mode
#define MAX_TAPS 2 // Used by Single Button Mode to determin the Maximum number of Tap Types

#define SEND_DELAY 5000
#define TAP_LOCKOUT 10000
#define DISP_UPDATE 100
#define FLASH_SPEED 250

#define BTN1 13  // Only BTN1 and it's LEDs need to be defined for Single Button Mode

#define B1R 0   // BTN 1 Red LED
#define B1G 15  // BTN 1 Green LED

#define SERVER "example.com"
#define CONTEXT "/taps"
RestClient client = RestClient(SERVER);
String response;

static const char ntpServerName[] = "us.pool.ntp.org";
const String timeZone = "America/Los_Angeles";  // List: https://garygregory.wordpress.com/2013/06/18/what-are-the-java-timezone-ids/

// ====================== END Config ======================

#define OLED_RESET 2
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// NTP Servers:
const String months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
int last_offset;

WiFiUDP Udp;
unsigned int udpPort = 8888;  // local port to listen for UDP packets

volatile byte tap_count = 0;
volatile unsigned int last_tap = 0;

unsigned int last_event = 0;
unsigned int last_display_update = 0;

int ledState = LOW; // LOW = Green && HIGH = Red
unsigned int last_led_update = 0;

String device_name;
int count[] = {0, 0};

#define ICO_HEIGHT 16
#define ICO_WIDTH  16

static const unsigned char PROGMEM connected_bmp[] =
{
  0x00, 0x00, 0x30, 0x0C, 0x30, 0x0C, 0x20, 0x0C, 0x21, 0x84,
  0x23, 0xC6, 0x63, 0xC4, 0x23, 0xC4, 0x21, 0x84, 0x31, 0x8C,
  0x31, 0x8C, 0x11, 0x88, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
  0x01, 0x80
};

static const unsigned char PROGMEM disconnected_bmp[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80,
  0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x01, 0x80, 0x01, 0x80,
  0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
  0x01, 0x80
};

// Prototype Functions
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

String startAPI(int id);
String sendEvent(int tapCount);
int getTZOffset(String zone);
void getCounts();

void setup()
{
  Serial.begin(74880);
  Serial.print("ITS Tap Tracker - ");
#ifndef SINGLE_BUTTON_MODE
  Serial.println("Multi Button Mode");
#endif
#ifdef SINGLE_BUTTON_MODE
  Serial.println("Single Button Mode");
#endif
  Serial.println();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.display();
  display.invertDisplay(1);
  display.setTextColor(WHITE); // 'inverted' text
  display.setCursor(4, 8);
  display.setTextSize(2);
  display.println("ITS TapTKR");
  display.display();

  pinMode(BTN1, INPUT_PULLUP);

  pinMode(B1G, OUTPUT);
  pinMode(B1R, OUTPUT);

  digitalWrite(B1G, LOW);
  digitalWrite(B1R, HIGH);

  attachInterrupt(digitalPinToInterrupt(BTN1), tap, FALLING);

#ifndef SINGLE_BUTTON_MODE
  pinMode(BTN2, INPUT_PULLUP);

  pinMode(B2G, OUTPUT);
  pinMode(B2R, OUTPUT);

  digitalWrite(B2G, LOW);
  digitalWrite(B2R, HIGH);

  attachInterrupt(digitalPinToInterrupt(BTN2), tap, FALLING);
#endif

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  Udp.begin(udpPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("Waiting for NTP sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  device_name = startAPI(STATION_ID);
  Serial.print("Device Name: ");
  Serial.println(device_name);

  getCounts();
  updateDisplay();
}

void loop() {
  if (tap_count > 0 && millis() - last_tap > SEND_DELAY) {
    Serial.print("Tap Type ");
    Serial.print(tap_count);
    Serial.println(" was counted. Sending to server...");

    sendEvent(tap_count);
    count[tap_count - 1]++;

    tap_count = 0;
  }

#ifndef SINGLE_BUTTON_MODE
  if (tap_count == 1) {
    digitalWrite(B2R, LOW);
    digitalWrite(B2G, HIGH);

    // Flash B1 Red
    digitalWrite(B1G, LOW);
    if (millis() - last_led_update > FLASH_SPEED) {
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      digitalWrite(B1R, ledState);
      last_led_update = millis();
    }
  } else if (tap_count == 2) {
    digitalWrite(B1R, LOW);
    digitalWrite(B1G, HIGH);

    // Flash B2 Red
    digitalWrite(B2G, LOW);
    if (millis() - last_led_update > FLASH_SPEED) {
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      digitalWrite(B2R, ledState);
      last_led_update = millis();
    }

  } else if (millis() - last_event > TAP_LOCKOUT || last_event == 0) {
    digitalWrite(B1R, LOW);
    digitalWrite(B2R, LOW);

    digitalWrite(B1G, HIGH);
    digitalWrite(B2G, HIGH);
  } else {
    digitalWrite(B1G, LOW);
    digitalWrite(B2G, LOW);

    digitalWrite(B1R, HIGH);
    digitalWrite(B2R, HIGH);
  }
#endif

#ifdef SINGLE_BUTTON_MODE
  if (tap_count != 0) {
    // Flash B1 Red
    digitalWrite(B1G, LOW);
    if (millis() - last_led_update > FLASH_SPEED) {
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      digitalWrite(B1R, ledState);
      last_led_update = millis();
    }
  }
  else if (millis() - last_event > TAP_LOCKOUT || last_event == 0) {
    digitalWrite(B1R, LOW);
    digitalWrite(B1G, HIGH);
  }
  else {
    digitalWrite(B1G, LOW);
    digitalWrite(B1R, HIGH);

  }
#endif

  if (hour() == 0 && minute() == 0 && timeStatus() != timeNotSet) {
    Serial.println("Resetting counts for the day");
    count[0] = 0;
    count[1] = 0;
    delay(60000);
  }

  if (millis() - last_display_update > DISP_UPDATE || last_display_update == 0) {
    updateDisplay();
    last_display_update = millis();
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.invertDisplay(0);
  display.setTextColor(WHITE);
  if (timeStatus() != timeNotSet) {
    display.setTextSize(1);
    display.drawFastVLine(0, 0, 8, WHITE);
    display.setCursor(1, 0);

    display.setTextColor(BLACK, WHITE);
    display.print(device_name);
    display.setTextColor(WHITE);
    display.print(" ");

    display.print(months[month() - 1]);
    display.print(" ");
    display.print(day());
    display.print(".");

    display.print(" ");
    display.print(hour());
    display.print(":");
    int m = minute();
    if (m < 10) display.print(0);
    display.print(minute());
  } else {
    Serial.println("NO NTP Data");
  }

  if (WiFi.status() != WL_CONNECTED) display.drawBitmap(112, 0,  disconnected_bmp, ICO_HEIGHT, ICO_WIDTH, 1);
  else display.drawBitmap(112, 0,  connected_bmp, ICO_HEIGHT, ICO_WIDTH, 1);

  display.setTextSize(2);
  display.setCursor(0, 16);
  if (tap_count == 1) display.setTextColor(BLACK, WHITE);
  else display.setTextColor(WHITE);
  display.print("1");
  display.setTextColor(WHITE);

  display.print(":");
  display.print(String(count[0]));
  display.print(" ");

  if (tap_count == 2) display.setTextColor(BLACK, WHITE);
  else display.setTextColor(WHITE);
  display.print("2");
  display.setTextColor(WHITE);

  display.print(":");
  display.print(String(count[1]));
  display.display();
}

void tap() {
  if (millis() - last_tap > 250 && millis() - last_event > TAP_LOCKOUT) {
#ifndef SINGLE_BUTTON_MODE
    if (digitalRead(BTN1) == LOW) {
      if (tap_count == 1) tap_count = 0;
      else tap_count = 1;

      last_tap = millis();
      Serial.println("TAP (1)!");
      Serial.println();
    }
    else if (digitalRead(BTN2) == LOW) {
      if (tap_count == 2) tap_count = 0;
      else tap_count = 2;

      last_tap = millis();
      Serial.println("TAP (2)!");
      Serial.println();
    }
#endif

#ifdef SINGLE_BUTTON_MODE
    tap_count = (tap_count + 1) % (MAX_TAPS + 1);
    last_tap = millis();
    Serial.println("TAP (" + String(tap_count) + ")!");
    Serial.println();
#endif

  }
}

/* -------- Tap Tracker API Code ---------- */
String startAPI(int id) {
  Serial.println("Retreiving Device Name from Server");
  response = "";
  String end_point = CONTEXT "/api/client/start?id=" + String(STATION_ID);

  int statusCode = client.get(end_point.c_str(), &response);
  Serial.print("Status code from server : ");
  Serial.println(statusCode);
  if ((statusCode / 100) != 2) {
    Serial.print("Response body from server : ");
    Serial.println(response);
  }

  return String(response);
}

String sendEvent(int tapCount) {
  Serial.println("Sending Event to Server (" + String(tapCount) + ")");
  last_event = millis();

  String payload = " {\"deviceID\":" + String(STATION_ID) + ",\"type\":" + String(tapCount) + "}";
  Serial.print("Payload: ");
  Serial.println(payload);

  response = "";
  int statusCode = client.post(CONTEXT "/api/client/event", payload.c_str(), &response);

  Serial.print("Status code from server: ");
  Serial.println(statusCode);
  if ((statusCode / 100) != 2) {
    Serial.print("Response body from server: ");
    Serial.println(response);
  }

  return response;
}

int getTZOffset(String zone) {
  Serial.println("Converting TimeZone into Offset");
  String end_point = CONTEXT "/api/client/tz_offset?zone=" + String(timeZone);

  response = "";
  int statusCode = client.get(end_point.c_str(), &response);
  if ((statusCode / 100) != 2) {
    Serial.print("Status code from server : ");
    Serial.println(statusCode);
    Serial.print("Response body from server : ");
    Serial.println(response);
  } else {
    last_offset = response.toInt();
  }

  return last_offset;
}

void getCounts() {
  Serial.println("Reterieving Counts from Server");
  String end_point = CONTEXT "/api/client/counts?id=" + String(STATION_ID);

  response = "";
  int statusCode = client.get(end_point.c_str(), &response);
  if ((statusCode / 100) != 2) {
    Serial.print("Status code from server : ");
    Serial.println(statusCode);
    Serial.print("Response body from server : ");
    Serial.println(response);
  }
  int spaceIndex = response.indexOf(' ');

  count[0] = response.substring(0, spaceIndex).toInt();
  count[1] = response.substring(spaceIndex + 1).toInt();

  Serial.print("1: ");
  Serial.print(count[0]);
  Serial.print("   2: ");
  Serial.print(count[1]);

  Serial.println();
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + getTZOffset(timeZone) * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress & address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
