/**
 * NeoPixel NTP Clock
 *
 * Author: Nicholas Klopfer-Webber
 * Date:   2018-06-15
 * Board:  NodeMCU 1.0 (ESP-12E Module)
 * Board Settings:
 *    Flash Size: 4M (3M SPIFFS)
 *    Debug Port: Disabled
 *    Debug Level: None
 *    IwIP Variant: v2 Lower Memory
 *    CPU Frequency: 80Mhz
 *    Upload Speed: 921600
 *    Erase Flash: Sketch Only
 *
 * To enable this board in Arduino IDE enter the following url into prefferences -> Additional Boards URL
 * http://arduino.esp8266.com/stable/package_esp8266com_index.json
 * Open Boards Manager from Tools > Board menu and find esp8266 platform.
 * Select the version you need from a drop-down box.
 *
 * Code based on:
 *   TimeNTP_ESP8266WiFi
 *   SSD1306ClockDemo
 *
 * Hardware:
 *   Board: WeMos D1 R1
 *   60 NeoPixels in a circle + 24 Neo in a inner circle.
 *   1-Wire Temp sensor
 *   LDR with 430K resistor to +ve.
 */

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiUdp.h>

Adafruit_NeoPixel Strip = Adafruit_NeoPixel(60, D3, NEO_GRB + NEO_KHZ800);
OneWire OneWire(D4);
WiFiUDP UDP;

IPAddress timeServerIP;
const char* NTPServerName = "au.pool.ntp.org";
const int NTP_PACKET_SIZE = 48;

byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

const char* ssid     = "CPH1707";
const char* password = "plm132rt";

unsigned long currentTime = 1000 * 60 * 60 * 3 + 1000 * 60 * 30 + 1000 * 40;
unsigned long lastSync = 0;

void setup() {
	Serial.begin(9600);

	WiFi.persistent(false);

	Strip.begin();
	Strip.setBrightness(10);

	testLeds();
	connectWifi();
	startUDP();
	syncTime();
}

void connectWifi() {
	WiFi.begin(ssid, password);

	Strip.clear();
	for (uint8_t i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++) {
		Strip.setPixelColor(i, Strip.Color(65, 190, 244));
		Strip.show();
		if (i == 59) {
			ESP.reset();
		}
		delay(1000);
	}
	Strip.clear();
}

void startUDP() {
	UDP.begin(123);
	if(!WiFi.hostByName(NTPServerName, timeServerIP)) {
		ESP.reset();
	}
}

void syncTime() {
	Strip.clear();

	for (uint8_t i = 0; i < 60; i++) {
		Strip.setPixelColor(i, Strip.Color(190, 0, 100));
		Strip.show();
		sendNTPpacket(timeServerIP);
		delay(1000);

		unsigned long time = (getTime() % (60 * 60 * 24)) * 1000 + (1000 * 60 * 60 * 10);

		if (time > 0) {
			currentTime = time;
			lastSync = time;
			break;
		}

		if (i == 59) {
			ESP.reset();
		}
	}
	Strip.clear();
}

unsigned long getTime() {
  if (UDP.parsePacket() == 0) {
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE);
  return (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);
  NTPBuffer[0] = 0b11100011;
  UDP.beginPacket(address, 123);
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

void testLeds() {
	for (uint8_t i = 0; i < 60; i++) {
		Strip.clear();
		Strip.setPixelColor(i, Strip.Color(65, 190, 244));
		Strip.show();

		delay(20);
	}
	Strip.clear();
	Strip.show();
}

void display() {
	unsigned int sec = (currentTime % (1000 * 60)) / 1000;
	unsigned int min = (currentTime % (1000 * 60 * 60)) / 1000 / 60;
	unsigned int hr = currentTime % (1000 * 60 * 60 * 24) / 1000 / 60 / 60;

	unsigned int hrLed = 60 * hr / 24;

	Strip.clear();
	Strip.setPixelColor(sec, Strip.Color(65, 190, 244));
	Strip.setPixelColor(min, Strip.Color(65, 100, 0));
	Strip.setPixelColor(hrLed, Strip.Color(190, 190, 244));
	Strip.show();
}

void loop() {
	display();
	currentTime += 1000;

	if ((currentTime - lastSync) > (1000 * 60 * 15)) {
		syncTime();
	}

	delay(1000);
}
