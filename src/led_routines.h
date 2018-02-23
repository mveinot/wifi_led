void colorSet(uint32_t c);
void rainbow(uint8_t wait);
void rainbowCycle(uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait);
void theaterChaseRainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);
uint32_t hex2color(String color);
uint32_t dimColor(rgb);
void safeSetPixel(uint16_t led, uint32_t color);
void nextLED(bool);
rgb colorToRGB(uint32_t color);
void flash();
void updateFire();
void rolling();
void thunderburst();
void lightning();

static int r = 255;
static int g = r-100;
static int b = 40;

void safeSetPixel(uint16_t led, uint32_t color) {
  if (led < 0 || led > strip.numPixels() - 1)
    return;

  strip.setPixelColor(led, color);
}

void nextLED(bool bounce) {
  prev_led = curr_led;

  if (!bounce && direction == UP && curr_led >= strip.numPixels()) {
    string_lit = true;
    return;
  }

  if (bounce && direction == UP && curr_led >= strip.numPixels()) {
    direction = DOWN;
  }

  if (bounce && direction == DOWN &&
      (curr_led == 0 || curr_led > strip.numPixels())) {
    direction = UP;
  }

  if (direction == UP) {
    curr_led++;
    next_led = curr_led + 1;
  } else {
    curr_led--;
    next_led = curr_led - 1;
  }
}

rgb colorToRGB(uint32_t color) {
  rgb _color;
  uint8_t w = (color >> 24) & 0xFF;
  _color.red = (color >> 16) & 0xFF;
  _color.green = (color >> 8) & 0xFF;
  _color.blue = color & 0xFF;
  return _color;
}

uint32_t dimColor(rgb color) {
  rgb _color = color;
  _color.red /= 16;
  _color.green /= 16;
  _color.blue /= 16;
  return strip.Color(_color.red, _color.green, _color.blue);
}

uint32_t hex2color(String color) {
  long hex_value = strtol(color.c_str(), NULL, 16);
  uint8_t r = hex_value >> 16;
  uint8_t g = hex_value >> 8 & 0xFF;
  uint8_t b = hex_value & 0xFF;

  return strip.Color(r, g, b);
}

void flash()
{
	uint32_t color;

	for (int i=0; i<256; i+=8) {
		color = strip.Color(i,i,i);
		colorSet(color);
		strip.show();
	}
	for (int i=255; i>=0; i-=8) {
		color = strip.Color(i,i,i);
		colorSet(color);
		strip.show();
	}
	strip.clear();
	delay(25);
}

void colorSet(uint32_t c) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
			httpServer.handleClient();
			ArduinoOTA.handle();
			webSocket.loop();
			yield();
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
			httpServer.handleClient();
			ArduinoOTA.handle();
			webSocket.loop();
			yield();
    }
    strip.show();
    delay(wait);
  }
}

// Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { // do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, c); // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) { // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q,
                            Wheel((i + j) % 255)); // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void updateFire() {
  int flicker = random(0,150);
	uint16_t led = random(0, strip.numPixels());
  int r1 = r-flicker;
  int g1 = g-flicker;
  int b1 = b-flicker;

  if(g1<0) g1=0;
  if(r1<0) r1=0;
  if(b1<0) b1=0;
  strip.setPixelColor(led,r1,g1,b1);

  strip.show();
}

void rolling() {
	for (int r=0; r < random(2,10); r++) {
		for (int i=0; i<strip.numPixels(); i++) {
			if (random(0,100) > 90) {
				strip.setPixelColor(i, white);
			} else {
				strip.setPixelColor(i, black);
			}
			httpServer.handleClient();
			ArduinoOTA.handle();
			webSocket.loop();
			yield();
		}
		strip.show();
		delay(random(5,100));
		strip.clear();
	}
}

void thunderburst()
{
	int rs1 = random(0, strip.numPixels()/2);
	int rl1 = random(0,5);
	int rs2 = random(rs1+rl1, strip.numPixels());
	int rl2 = random(0,5);

	for (int r=0; r<random(3,6); r++) {
		for (int i=0; i<rl1; i++) {
			strip.setPixelColor(i+rs1, white);
		}

		httpServer.handleClient();
		ArduinoOTA.handle();
		webSocket.loop();
		yield();
		if (rs2+rl2 < strip.numPixels()) {
			for (int i=0; i<rl2; i++) {
				strip.setPixelColor(i+rs2, white);
			}
		}

		strip.show();
		delay(random(10,50));
		strip.clear();
		delay(random(10,50));
	}
}

void lightning() {
	switch(random(1,10)) {
	case 1:
		thunderburst();
		delay(random(10,500));
		break;
	case 2:
		rolling();
		break;
	case 3:
		flash();
		delay(random(50,250));
		break;
	}
}
