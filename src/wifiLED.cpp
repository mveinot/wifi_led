#include "Arduino.h"
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <WebSocketsServer.h>
#include <FS.h>

#include <Adafruit_NeoPixel.h>

#define PIN D5
#define NUM_LEDS 60

enum Mode_t { OFF, FANCY_OFF, ON, FIRE, SCAN, WIPE, RANDOM, RAINBOW1, RAINBOW2, ROLLING, THUNDERBURST, LIGHTNING };
enum Direction_t { UP, DOWN };

struct rgb {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip =
    Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);
WiFiClient wifiClient;
ESP8266WebServer httpServer(80);
WebSocketsServer webSocket(81);

// WebServer
void handleRoot();
void handleOn();
void handleOff();
void handleFire();
void handleWipe();
void handleScan();
void handleFlash();
void handleRandom();
void handleRainbow1();
void handleRainbow2();
void handleRolling();
void handleLightning();
void handleThunderburst();
void handleParams();
void parseParams();
void handleNotFound();
String getContentType(String filename);

// WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length);

// global variables
uint32_t current_color, black = strip.Color(0, 0, 0);
uint32_t white = strip.Color(255, 255, 255);
uint8_t prev_led = 0;
uint8_t curr_led = 0;
uint8_t next_led = 0;
Mode_t mode = ON;
Direction_t direction = UP;
uint16_t wait = 25;
bool string_lit = false;
bool clear_random = true;
unsigned long next_operation = 0;
uint8_t end_led = 0;

#include "led_routines.h"

void setup() {
  Serial.begin(9600);
  SPIFFS.begin();
  strip.begin();

  WiFi.hostname("ledstrip");
  Serial.println("\nconnecting to wifi...");
  WiFi.begin("ssid", "password");
  while (WiFi.status() != WL_CONNECTED) {
	  Serial.write('.');
    delay(250);
  }
  Serial.println("connected");
  WiFi.mode(WIFI_STA);

  ArduinoOTA.setHostname("led_strip");
  ArduinoOTA.onStart([]() {
    strip.clear();
    colorSet(strip.Color(32, 0, 0));
    strip.show();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    strip.setPixelColor(int(progress / (total / 30)), strip.Color(0, 0, 255));
    strip.show();
  });

  ArduinoOTA.onEnd([]() {
    strip.clear();
    strip.show();
  });

  ArduinoOTA.begin();

  httpServer.on("/", handleRoot);
  httpServer.on("/on", handleOn);
  httpServer.on("/off", handleOff);
	httpServer.on("/fire", handleFire);
  httpServer.on("/scan", handleScan);
  httpServer.on("/wipe", handleWipe);
  httpServer.on("/flash", handleFlash);
  httpServer.on("/params", handleParams);
  httpServer.on("/random", handleRandom);
	httpServer.on("/rainbow1", handleRainbow1);
	httpServer.on("/rainbow2", handleRainbow2);
	httpServer.on("/rolling", handleRolling);
	httpServer.on("/lightning", handleLightning);
	httpServer.on("/thunderburst", handleThunderburst);
  httpServer.onNotFound(handleNotFound);
  httpServer.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

	flash();

  delay(250);

}

void loop() {
  httpServer.handleClient();
  ArduinoOTA.handle();
  webSocket.loop();

	if (millis() > next_operation) {
  switch (mode) {
  case OFF:
		colorSet(black);
    break;
  case FANCY_OFF:
		end_led = strip.numPixels() - (curr_led + 1);
		Serial.printf("current: %d\n", curr_led);
		Serial.printf("end:     %d\n", end_led);
    safeSetPixel(curr_led, black);
    safeSetPixel(end_led, black);
    strip.show();
    nextLED(false);
    if (string_lit) {
      mode = OFF;
    }
    break;
  case ON:
    colorSet(current_color);
    break;
  case SCAN:
    strip.clear();
    safeSetPixel(prev_led, dimColor(colorToRGB(current_color)));
    safeSetPixel(curr_led, current_color);
    safeSetPixel(next_led, dimColor(colorToRGB(current_color)));
    strip.show();
    nextLED(true);
    break;
  case WIPE:
    safeSetPixel(curr_led, current_color);
    strip.show();
    nextLED(false);
    if (string_lit) {
      mode = ON;
    }
    break;
  case RANDOM:
    if (clear_random) {
      strip.clear();
    }
    strip.setPixelColor(random(0, strip.numPixels()), random(0, 255),
                        random(0, 255), random(255));
    strip.show();
	case FIRE:
		updateFire();
		break;
	case RAINBOW1:
		rainbow(wait);
		break;
	case RAINBOW2:
		rainbowCycle(wait);
		break;
	case ROLLING:
		rolling();
		break;
	case THUNDERBURST:
		thunderburst();
		break;
	case LIGHTNING:
		lightning();
		break;
  }
	next_operation = millis() + wait;
	}

  /*
  Serial.print("Mode     : ");
  Serial.println(mode);
  Serial.print("LED      : ");
  Serial.println(curr_led);
  Serial.print("Direction: ");
  Serial.println(direction);
  Serial.println();
  colorToRGB(current_color);
  */
//  delay(wait);
  yield();
}

void parseParams() {
  String color_string;
	int rmode;

  if (httpServer.hasArg("color")) {
    color_string = httpServer.arg("color");
    current_color = hex2color(color_string);
  } else {
    current_color = white;
  }

  if (httpServer.hasArg("delay")) {
    wait = httpServer.arg("delay").toInt();
  }

  if (httpServer.hasArg("clear_random")) {
    clear_random = httpServer.arg("clear_random") == "true";
  }
}

String getContentType(String filename) {
  if (httpServer.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void handleRoot() {
  String response;

	File f = SPIFFS.open("/index.html", "r");
	if (!f) {
		response = "File not found\n";
	} else {
		response = f.readString();
	}
	f.close();

	httpServer.send(200, "text/html", response);
}

void handleParams() {
	parseParams();
	httpServer.send(200, "text/plain", "Updated params");
}

void handleOff() {
  parseParams();
  mode = FANCY_OFF;
  curr_led = 0;
  wait = 100;
  string_lit = false;
  httpServer.send(200, "text/plain", "LEDs off");
}

void handleOn() {
  parseParams();
  mode = ON;
  httpServer.send(200, "text/plain", "LEDs ON");
}

void handleWipe() {
  parseParams();
  mode = WIPE;
  curr_led = 0;
  string_lit = false;
  strip.clear();
  strip.show();
  httpServer.send(200, "text/plain", "Wiping LEDs");
}

void handleScan() {
  parseParams();
  mode = SCAN;
  curr_led = 0;
  httpServer.send(200, "text/plain", "LEDs scanning");
}

void handleFlash() {
	parseParams();
	flash();
  httpServer.send(200, "text/plain", "Flash LEDs");
}

void handleRolling() {
	parseParams();
	mode = ROLLING;
  httpServer.send(200, "text/plain", "LED Rolling Lightning");
}

void handleThunderburst() {
	parseParams();
	mode = THUNDERBURST;
  httpServer.send(200, "text/plain", "LED Thunderburst");
}

void handleLightning() {
	parseParams();
	mode = LIGHTNING;
  httpServer.send(200, "text/plain", "LED Lightning show");
}

void handleRandom() {
  parseParams();
  mode = RANDOM;
  httpServer.send(200, "text/plain", "LEDs scanning");
}

void handleFire() {
	parseParams();
	mode = FIRE;
  httpServer.send(200, "text/plain", "Firing LEDs");
}

void handleRainbow1() {
	parseParams();
	mode = RAINBOW1;
  httpServer.send(200, "text/plain", "LED RAINBOW");
}

void handleRainbow2() {
	parseParams();
	mode = RAINBOW2;
  httpServer.send(200, "text/plain", "LED RAINBOW");
}

void handleNotFound()
{
  String response;
  String request = httpServer.uri();
  String mimetype = "text/html";
  File f;

  if (SPIFFS.exists(request))
  {
    File f = SPIFFS.open(request, "r");
    if (!f) {
      response = "Requested file: \"" + request + "\" exists, but there was a problem loading it\n";
      httpServer.send(503, "text/plain", response);
    } else
    {
      mimetype = getContentType(request);
      httpServer.streamFile(f, mimetype);
    }
    f.close();
  } else
  {
    String response = "File Not Found\n\n";
    response += "URI: ";
    response += httpServer.uri();
    response += "\nMethod: ";
    response += (httpServer.method() == HTTP_GET) ? "GET" : "POST";
    response += "\nArguments: ";
    response += httpServer.args();
    response += "\n";
    for (uint8_t i = 0; i < httpServer.args(); i++) {
      response += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
    }
    httpServer.send(404, "text/plain", response);
  }
}
                   
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED: {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
                  ip[1], ip[2], ip[3], payload);

    // send message to client
    webSocket.sendTXT(num, "Connected");
  } break;
  case WStype_TEXT:
    if (payload[0] == '#') {
      // we get RGB data
      // decode rgb data
      uint32_t rgb = (uint32_t)strtol((const char *)&payload[1], NULL, 16);

      uint8_t _red = ((rgb >> 16) & 0xFF);
      uint8_t _green = ((rgb >> 8) & 0xFF);
      uint8_t _blue = ((rgb >> 0) & 0xFF);
      current_color = strip.Color(_red, _green, _blue);
    }

    break;
  }
}
