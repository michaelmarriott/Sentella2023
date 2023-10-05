// BasicTest example to demonstrate how to use FastLED with OctoWS2811

#include <OctoWS2811.h>
#define USE_OCTOWS2811
#include <FastLED.h>
#include "Math3D.h"
#include "Noise.h"

#include <ArduinoJson.h>

#define DEPTH 50
#define WIDTH 5   // X
#define HEIGHT 5  // Y
#define SENSOR_PER_ROW 4
#define SENSOR_ROWS 4

class Point {
public:  // Access specifier
  int X;
  int Y;
  Point(int x, int y) {  // Constructor with parameters
    X = x;
    Y = y;
  }
};

const int NUM_LEDS = DEPTH * 8 * HEIGHT;  // We works eight's lovey.
const int NUM_SENSORS = SENSOR_PER_ROW * SENSOR_ROWS;

CRGB leds[NUM_LEDS];
struct CRGB temp_leds[NUM_LEDS];
int sensors[NUM_SENSORS];
CRGB cube[2][WIDTH][HEIGHT][DEPTH];

bool isStarted = false;
int saturation = 200;  // 255 is max
int hsvcolor = 40;     // 255 is max
int brightness = 250;  // 255 is max
int loopCounter = 0;   // ALWAYS RESET TO 0 WHEN PATTERN CHANGES
int motionBlur = 240;

int mainDelay = 100;
int pattern = 5;
int maxPatterns = 5;
int loopPatternUntilChange = 1000;
int beginDelay = 500;
int sensorSpeed = 5;

// source buffer for original value
struct CRGB colors[WIDTH][HEIGHT][DEPTH];
// time the pixel is activated
float timed[WIDTH][HEIGHT][DEPTH];

bool debug = false;

int greenRedStrips[] = { 0, 5 };
int sizeOfGreenRedStrips = 0;

void setup() {
  Serial.begin(9600);
  // println("setup");
  FastLED.addLeds<OCTOWS2811, RGB>(leds, HEIGHT * DEPTH);
  FastLED.setBrightness(brightness);

  Serial.setTimeout(50);
  Serial.flush();
}

void loop() {
  serialRead();
  EVERY_N_MILLIS_I(thisTimer, beginDelay) {
    thisTimer.setPeriod(mainDelay);
    int microsec = 6000 / NUM_LEDS;
    rundownSensor(sensorSpeed);
    if (pattern == 0) {
      //calibrate();
      twinkle(loopCounter, 1, 0);
      if (loopCounter == 0) {
        mainDelay = 100;
        sensorSpeed = 5;
        loopPatternUntilChange = 2000;
      }
    } else if (pattern == 1) {
      twinkle(loopCounter, 1, 10);
      if (loopCounter == 0) {
        mainDelay = 100;
        sensorSpeed = 25;
        loopPatternUntilChange = 1000;
      }
    } else if (pattern == 2) {

      wipeout(loopCounter, 0, 20);
      if (loopCounter == 0) {
        mainDelay = 100;
        sensorSpeed = 5;
        loopPatternUntilChange = 2000;
      }
    } else if (pattern == 3) {
      wipeout(loopCounter, 1, 130);
      if (loopCounter == 0) {
        mainDelay = 200;
        sensorSpeed = 10;
        loopPatternUntilChange = 1000;
      }
    } else if (pattern == 4) {
      plasma(loopCounter);
      if (loopCounter == 0) {
        mainDelay = 100;
        sensorSpeed = 25;
        loopPatternUntilChange = 500;
      }
    } else if (pattern == 5) {
      RippleWrapper(10);
      if (loopCounter == 0) {
        mainDelay = 400;
        sensorSpeed = 25;
        loopPatternUntilChange = 1000;
      }
    }

    // Rain. matrix

    print("loopCounter");
    print(loopCounter);
    print(":pattern");
    println(pattern);
    //starfield(loopCounter, 1);
    //calibrate();
    //calibrate_connective();
    loopCounter = loopCounter + 1;

    if (loopPatternUntilChange < loopCounter) {
      loopCounter = 0;
      pattern = pattern + 1;
      if (pattern >= maxPatterns) {
        pattern = 0;
      }
    }
  }
}

CRGBPalette16 blackWhitePalette = CRGBPalette16(
  CRGB(10, 10, 200), CRGB::Black, CRGB(10, 10, 200), CRGB::Black,
  CRGB::White, CRGB::Black, CRGB::Black, CRGB(80, 80, 200),
  CRGB::Black, CRGB(80, 80, 200), CRGB(40, 40, 60), CRGB::Black,
  CRGB::White, CRGB::Black, CRGB::Black, CRGB(10, 10, 200));

void spottedFlickersFire(int loopCounter) {
  int a = millis();

  for (int i = 0; i < NUM_LEDS; i++) {

    // 3D noise, x is constant, we move through time on y and z axis
    // The 60 here will change the scale of the effect, lower is smoother
    // higher is more flickery. The time value for z was in the original code
    // as that was a 2D matrix version. I've left it in here as it looks
    // nice in 1D too!
    uint8_t noise = inoise8(0, i * 60 + a, a / 3);

    // Divides 255 by (NUM_LEDS - 1) and subtracts that amount away from 255 to return
    // a decreasing number each time e.g. for NUM_LEDS = 18, difference between
    // each point is 15, so:
    // i = 0, math = 255
    // i = 1, math = 240
    // i = 2, math = 225
    // ...
    // i = NUM_LEDS, math =  0
    uint8_t math = abs8(i - (NUM_LEDS - 1)) * 255 / (NUM_LEDS - 1);

    // Take the noise function from earlier and subtract math from it,
    // so the further along the strip you are, the higher palette index you
    // are likely to get. This results in the base of the fire (white, yellow)
    // being at the far end of the strip
    uint8_t index = qsub8(noise, math);

    // Set the LED color from the palette
    temp_leds[i] = ColorFromPalette(blackWhitePalette, index, 255);
  }

  voxelBlendDisplay(10);
}

int ripple_color;
int ripple_center = 0;
int ripple_step = -1;
int ripple_maxSteps = 74;
float ripple_fadeRate = 0.95;
int ripple_diff;

//background color
uint32_t ripple_currentBg = random(256);
uint32_t ripple_nextBg = ripple_currentBg;

void RippleWrapper(int speed) {
  Ripple(speed);
}

void Ripple(int speed) {

  if (ripple_currentBg == ripple_nextBg) {
    ripple_nextBg = random(256);
  } else if (ripple_nextBg > ripple_currentBg) {
    ripple_currentBg++;
  } else {
    ripple_currentBg--;
  }
  for (int x = 0; x < WIDTH * HEIGHT; x++) {
    for (uint16_t i = 0; i < DEPTH; i++) {
      temp_leds[(x * DEPTH) + i] = CHSV(50, ripple_currentBg, 30);  // strip.setPixelColor(l, Wheel(currentBg, 0.1));
    }
  }

  if (ripple_step == -1) {
    ripple_center = random(DEPTH);
    ripple_color = random(256);
    ripple_step = 0;
  }

  if (ripple_step == 0) {
    temp_leds[ripple_center] = CHSV(ripple_color, 255, 255);  // strip.setPixelColor(center, Wheel(color, 1));
    ripple_step++;
  } else {
    if (ripple_step < ripple_maxSteps) {
      //Serial.println(sin(ripple_fadeRate, ripple_step));
      println(pow(ripple_fadeRate, ripple_step));
      for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
          int start = ((x * HEIGHT) + y) * DEPTH;
          temp_leds[wrap(start + ripple_center + ripple_step)] = CHSV(ripple_color, 255, pow(ripple_fadeRate, ripple_step) * 255);  //   strip.setPixelColor(wrap(center + step), Wheel(color, pow(fadeRate, step)));
          temp_leds[wrap(ripple_center - ripple_step)] = CHSV(ripple_color, 255, pow(ripple_fadeRate, ripple_step) * 255);          //   strip.setPixelColor(wrap(center - step), Wheel(color, pow(fadeRate, step)));
          if (ripple_step > 3) {
            temp_leds[wrap(start + ripple_center + ripple_step - 3)] = CHSV(ripple_color, 255, pow(ripple_fadeRate, ripple_step - 2) * 255);  //   strip.setPixelColor(wrap(center + step - 3), Wheel(color, pow(fadeRate, step - 2)));
            temp_leds[wrap(start + ripple_center - ripple_step + 3)] = CHSV(ripple_color, 255, pow(ripple_fadeRate, ripple_step - 2) * 255);  //   strip.setPixelColor(wrap(center - step + 3), Wheel(color, pow(fadeRate, step - 2)));
          }
        }
      }
      ripple_step++;
    } else {
      ripple_step = -1;
    }
  }
  voxelBlendDisplay(0);
}

int wrap(int step) {
  if (step < 0) return DEPTH + step;
  if (step > NUM_LEDS - 1) return step - DEPTH;
  return step;
}


// Twinkle

// amount of time before adding a new pixel
//Timer timer_interval = 0.1f;
// amount of seconds it takes to fade a pixel to max
float fade_in_speed = 10.0f;
// amount of seconds it takes to fade a pixel to min
float fade_out_speed = 80.0f;
// single color for single color mode
CRGB single_color;
// different animation modes
boolean mode_single_color;
boolean mode_fade_out;
int twinkle_sensor = 1;

void twinkle(int counter, int dt, int blur) {
  if (counter == 0) {
    clear_temp();
  }
  single_color = CRGB(200, 50, 200);
  twinkleSequence(1, blur);
}

void twinkleSequence(int dt, int blur) {
  uint8_t bright = brightness;
  uint16_t pixels_active = 0;

  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      for (uint8_t z = 0; z < DEPTH; z++) {
        if (!colors[x][y][z] == CRGB::Black) {
          // printXYZ(x, y, z);
          if (timed[x][y][z] < fade_in_speed) {

            float t = timed[x][y][z] / fade_in_speed;
            CRGB colored = colors[x][y][z];
            if (x == 0 && y == 0 && z <= 50) {
              print("FadeIn:");
              print(t);
              print("-");
              print(timed[x][y][z]);
              print("-");
              print(t);
              print("-");
              println(getLedValue(colored));
            }
            voxel(x, y, z, colored.nscale8(bright * t));  // .nscale8(bright *(1- t))  // .scaled(bright * t));
            timed[x][y][z] += dt;
            pixels_active++;
          } else if (timed[x][y][z] < (fade_in_speed + fade_out_speed)) {

            float t = (timed[x][y][z] - fade_in_speed) / fade_out_speed;
            CRGB colored = colors[x][y][z];
            voxel(x, y, z, colored.nscale8(bright * (1 - t)));  // .scaled(bright * (1 - t)));

            if (x == 0 && y == 0 && z >= 10) {
              print("FadeOut");
              print(timed[x][y][z]);
              print("-");
              println(t);
            }
            timed[x][y][z] += dt;
            pixels_active++;
          } else {
            if (x == 0 && y == 0 && z >= 10) {
              println("Make Black ");
            }

            timed[x][y][z] = 0;
            colors[x][y][z] = CRGB::Black;
            voxel(x, y, z, CRGB::Black);
          }
        } else {
          timed[x][y][z] = 0;
        }
      }
    }
  }

  print("pixels_active");
  println(pixels_active);
  voxelDisplay(blur);

  // Create next twinkle
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t x = random(0, WIDTH);
    uint8_t y = random(0, HEIGHT);
    uint8_t z = random(0, DEPTH);
    // println(timed[x][y][z]);
    if (timed[x][y][z] == 0) {
      colors[x][y][z] = CRGB(random(30, 100), random(10, 50), random(60, 255));
    }
  }
  //twinkle by sensor
  for (int x = 0; x < SENSOR_ROWS; x++) {
    for (int y = 0; y < SENSOR_PER_ROW; y++) {
      int sensor_number = (x * SENSOR_PER_ROW) + y;
      if (sensors[sensor_number] > 0) {
        int sensor_value = sensors[sensor_number];
        print("sensor");
        print(sensor_number);

        for (int xs = 0; xs <= 1; xs++) {
          for (int ys = 0; ys <= 1; ys++) {
            Point point = getXYForSensor(sensor_number, xs, ys);
            for (uint8_t i = 0; i < twinkle_sensor; i++) {
              uint8_t z = random(0, DEPTH);
              print("point");
              print(point.X);
              print("-");
              print(point.Y);
              print("-");
              print(z);
              print("=");
              print(timed[point.X][point.Y][z]);
              print(":");
              int pixel = gitPixelNumber(point.X, point.Y, z);
              if (timed[point.X][point.Y][z] == 0) {
                colors[point.X][point.Y][z] = CRGB(random(100, 250), random(10, 70), random(20, 80));
              } else if (timed[point.X][point.Y][z] > fade_in_speed) {
                timed[point.X][point.Y][z] = fade_in_speed;
              }
            }
          }
        }
        sensors[sensor_number] = sensor_value - 1;
      }
    }
  }
}
////////////////////////////////////////////////////////////

CRGB SetColor(int x, int y, int red, int green, int blue) {
  //int strip = getStrip(x, y);
  return CRGB(red, green, blue);
}

uint8_t noise_map[WIDTH][HEIGHT][DEPTH];
Noise noise = Noise();
// start somewhere in the noise map
float noise_x = noise.nextRandom(0, 255);
float noise_y = noise.nextRandom(0, 255);
float noise_z = noise.nextRandom(0, 255);
float noise_w = noise.nextRandom(0, 255);

float speed_offset = 0;
float speed_offset_speed = 0.02f;
// scale_p represents the distance between each pixel in the noise map
float scale_p = 0.15f;
// speed is how fast the movement is over the axis in the noise map
float speed_x = 0.1f;
float speed_y = 0.2f;
float speed_z = 0.3f;
float speed_w = 0.4f;


void plasma(int dt) {
  plasma_sequence(dt);
}

void plasma_sequence(int dt) {
  updateNoise(dt);
  makeNoise();
  drawNoise(brightness);
}

// Create the 3D noise data matrix[][][]
void makeNoise() {
  for (int x = 0; x < WIDTH; x++) {
    float xoffset = noise_x + scale_p * x;
    for (int y = 0; y < HEIGHT; y++) {
      float yoffset = noise_y + scale_p * y;
      for (int z = 0; z < DEPTH; z++) {
        float zoffset = noise_z + scale_p * z;
        noise_map[x][y][z] =
          noise.noise4(xoffset, yoffset, zoffset, noise_w) * 255;
      }
    }
  }
}

// Draw the 3D noise data matrix[][][]
void drawNoise(uint8_t brightness) {
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      for (int z = 0; z < DEPTH; z++) {
        // The index at (x,y,z) is the index in the color palette
        uint8_t index = noise_map[x][y][z];
        // The value at (y,x,z) is the overlay for the brightness
        voxel(x, y, z, ColorFromPalette(OceanColors_p, index).nscale8(index));
        //  voxel(x, y, z,
        //        Color((hue16 >> 8) + index, LavaPalette)
        //            .scale(noise_map[y][x][z])
        //           .scale(brightness));
      }
    }
  }
  voxelBlendDisplay(160);
}


static inline uint8_t map8(uint8_t scalar, uint8_t outMax) {
  return (((outMax + 1) * scalar) >> 8);
}

void updateNoise(float dt) {
  speed_offset += dt * speed_offset_speed;
  // use same speed offset, but offset each in the noise map
  speed_x = 2 * (noise.noise1(speed_offset + 000) - 0.5);    //  -1 to 1
  speed_y = 2 * (noise.noise1(speed_offset + 050) - 0.5);    //  -1 to 1
  speed_z = 2 * (noise.noise1(speed_offset + 100) - 0.5);    //  -1 to 1
  speed_w = 2 * (noise.noise1(speed_offset + 150) - 0.5);    //  -1 to 1
  scale_p = .15 + (noise.noise1(speed_offset + 200) / 6.6);  // .15 to .30

  noise_x += speed_x * dt;
  noise_y += speed_y * dt;
  noise_z += speed_z * dt;
  noise_w += speed_w * dt;
}



///////////////////////////////////////////////////////////
// Postition in color palette << 8 for more resolution
int16_t hue16;
int16_t hue16_speed;

float phase;
float phase_speed;

static const int numStars = 200;
Vector3 stars[numStars];
bool initialized = false;

void starfield(int loopCounter, int dt) {
  if (loopCounter == 0) {
    for (int i = 0; i < numStars; i++) {
      stars[i] = Vector3(noise.nextRandom(-1, 1), noise.nextRandom(-1, 1),
                         noise.nextRandom(-1, 1));
    }
    initialized = true;
  }
  starfieldSequence(dt);
}

void starfieldSequence(int dt) {

  phase += dt * phase_speed;
  hue16 += dt * hue16_speed;

  Quaternion q = Quaternion(000.0f * phase, Vector3(0, 1, 0));
  for (int i = 0; i < numStars; i++) {
    float r = (stars[i] * 3 - Vector3(random(1, WIDTH), random(0, HEIGHT), -2.0f)).magnitude();
    stars[i].z += sinf(phase) * 1.75f * dt * r;
    if (stars[i].z > 1) {
      stars[i] = Vector3(random(1, WIDTH), random(0, HEIGHT), random(0, DEPTH));
    } else if (stars[i].z < -1) {
      stars[i] = Vector3(random(1, WIDTH), random(0, HEIGHT), random(0, DEPTH));
    }
    println(stars[i].x);
    CRGB c = ColorFromPalette(RainbowColors_p, (hue16 >> 8) + (int8_t)(r * 6));  // //CRGB((hue16 >> 8) + (int8_t)(r * 6), RainbowColors_p);
    // Multiply by sqrt(3) * radius = 12.99 => 13
    voxelv(stars[i]);  //, c.nscale8(brightness)
  }
  voxelBlendDisplay(40);
}
////////////////////////////////////////////////////////////////////////

class Ball {
public:  // Access specifier
  int X = 0;
  int Y = 0;
  int Z = 0;
  int X_M = 1;
  int Y_M = 1;
  int Z_M = 1;

  Ball(int x, int y, int z, int xm, int ym, int zm) {  // Constructor with parameters
    X = x;
    Y = y;
    Z = z;
    X_M = xm;
    Y_M = ym;
    Z_M = zm;
  }

  void moveX() {
    ;
    X_M = setMovement(X, X_M, WIDTH);
    X = X + X_M;
  }

  void moveY() {
    Y_M = setMovement(Y, Y_M, HEIGHT);
    Y = Y + Y_M;
  }

  void moveZ() {
    Z_M = setMovement(Z, Z_M, DEPTH);
    Z = Z + Z_M;
  }

  int setMovement(int c, int m, int max) {
    if (c + m > max - 1) {
      return -1;
    } else if (c + m < 1) {
      return 1;
    } else {
      return m;
    }
  }
};

Ball ball1 = Ball(random(1, WIDTH - 2), random(1, HEIGHT - 2), random(1, DEPTH - 5), 1, 1, 1);
Ball ball2 = Ball(random(2, WIDTH - 1), random(2, HEIGHT - 1), random(5, DEPTH - 1), 1, 1, 1);
CRGB colorBall1 = CRGB::Black;
CRGB colorBall2 = CRGB::Black;

void wipeout(int loopCounter, int keepSpace, int blur) {
  wipeoutSequence(loopCounter, keepSpace, blur);
}

void wipeoutSequence(int loopCounter, int keepSpace, int blur) {
  if (loopCounter == 0) {
    int g = random(5, 55);
    int g2 = 255 - g;
    colorBall1 = CRGB(random(5, 20), g, random(25, 250));
    colorBall2 = CRGB(random(105, 250), g, random(20, 80));
    //FastLED.setBrightness(50);
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
  }

  if (keepSpace == 1) {
    clear_temp();
  }


  int direction = random(8);
  if (direction == 0) {
    ball1.moveX();
  } else if (direction == 1) {
    ball1.moveY();
  } else {
    ball1.moveZ();
  }


  direction = random(8);
  if (direction == 0) {
    ball2.moveX();
  } else if (direction == 1) {
    ball2.moveY();
  } else {
    ball2.moveZ();
  }

  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      for (int z = -1; z <= 1; z++) {
        voxel(ball1.X + x, ball1.Y + y, ball1.Z + z, colorBall1);
      }
    }
  }

  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      for (int z = -1; z <= 1; z++) {
        voxel(ball2.X + x, ball2.Y + y, ball2.Z + z, colorBall2);
      }
    }
  }

  voxelBlendDisplay(blur);
}


//////////////
void voxel(int x, int y, int z, CRGB color) {
  int strip = getStrip(x, y);
  temp_leds[(x * HEIGHT * DEPTH) + (y * DEPTH) + z] = color;
}

void voxelv(Vector3 v) {
  // print("v.x");
  // print(v.x);
  int x = v.x;
  int y = v.y;
  int z = v.z;
  if (x != 0 && y != 0 && z != 0) {
    // println(v.x);
  }
  // printLed(v.x );
  temp_leds[(x * HEIGHT * DEPTH) + (y * DEPTH) + z] = CRGB::Blue;  // color;
}

int gitPixelNumber(int x, int y, int z) {
  return (x * HEIGHT * DEPTH) + (y * DEPTH) + z;
}

void printXYZ(int x, int y, int z) {
  print(x);
  print(y);
  print(z);
  println("");
}

void printLedValues() {

  for (uint8_t x = 2; x < 4; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      for (uint8_t z = 0; z < DEPTH; z++) {

        if (x == 2 && y == 0 && z == 0) {
          // print("i=");
          // print(getLedValue(leds[(x * HEIGHT * DEPTH) + (y * DEPTH) + z]));
          // print(":");
        }
      }
    }
  }
  //println("");
}

int getLedValue(CRGB led) {
  int i = led.red + led.green + led.blue;
  return i;
}

void printLed(CRGB led) {
  int v = getLedValue(led);
  // println(v);
}


void clear_temp() {
  for (int x = 0; x < NUM_LEDS; x++) {
    temp_leds[x] = CRGB::Black;
  }
}
void voxelDisplay(int blur) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = temp_leds[i];
  }
  blur1d(leds, NUM_LEDS, blur);
  //blur2d(leds, 1,1, blur);
  FastLED.show();
}

void voxelBlendDisplay(int blur) {
  println("voxelBlendDisplay");
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      for (uint8_t z = 0; z < DEPTH; z++) {
        int i = gitPixelNumber(x, y, z);
        uint8_t strip = getStrip(x, y);

        if (inGreenRedStrips(strip)) {
          leds[i] = CRGB(temp_leds[i].green, temp_leds[i].red, temp_leds[i].blue);
        } else {
          leds[i] = temp_leds[i];
        }
      }
    }
  }
  blendSensor();
  blur1d(leds, NUM_LEDS, blur);
  FastLED.show();
}


bool inGreenRedStrips(int e) {
  int size = sizeOfGreenRedStrips;
  for (int i = 0; i < size; i++) {
    if (greenRedStrips[i] == e) {
      return true;
    }
  }
  return false;
}


void rundownSensor(int dt) {
  for (int x = 0; x < SENSOR_ROWS; x++) {
    for (int y = 0; y < SENSOR_PER_ROW; y++) {
      int sensor_number = (x * SENSOR_PER_ROW) + y;
      if (sensors[sensor_number] > 1) {
        sensors[sensor_number] = sensors[sensor_number] - dt;
      }
    }
  }
}


void blendSensor() {
  println("blendSensor");
  for (int x = 0; x < SENSOR_ROWS; x++) {
    for (int y = 0; y < SENSOR_PER_ROW; y++) {
      int sensor_number = (x * SENSOR_PER_ROW) + y;
      if (sensors[sensor_number] > 0) {
        int sensor_value = sensors[sensor_number];
        // print("sensor_value");
        // println(sensor_value);
        for (int x = 0; x <= 1; x++) {
          for (int y = 0; y <= 1; y++) {
            int p = getFirstPixelForSensor(sensor_number, x, y);
            // print("p");
            //println(p);
            for (int z = 0; z <= DEPTH; z++) {
              leds[p + z] = blend(temp_leds[p + z], CRGB::White, sensor_value);
            }
          }
        }
        sensors[sensor_number] = sensor_value - 1;
      }
    }
  }
}

//
// 00  01  02  03  04
//   00  01  02  03
// 05  06  07  08  09
//   04  05  06  07
// 10  11  12  13  14
//   08  09  10  11
// 15  16  17  18  19
//   12  13  14  15
// 20  21  22  23  24

// 00  01  02  03  04
//   00  04  08  12
// 05  06  07  08  09
//   01  05  09  13
// 10  11  12  13  14
//   02  06  10  14
// 15  16  17  18  19
//   03  07  11  15
// 20  21  22  23  24
int getFirstPixelForSensor(int sensor, int x, int y) {
  return getStripForSensor(sensor, x, y) * DEPTH;
}

uint8_t getStrip(uint8_t x, uint8_t y) {
  return (x * WIDTH) + y;
}

//int getStripForSensor(int sensor, int x, int y) {
//  int row = (sensor / SENSOR_PER_ROW);
//  int strip = (row + x + sensor) + (y * (SENSOR_PER_ROW + 1));
//  return strip;
//}

int getStripForSensor(int sensor, int x, int y) {
  int row = (sensor % SENSOR_PER_ROW);
  int col = (sensor / SENSOR_PER_ROW);
  int strip = ((row * (SENSOR_PER_ROW + 1)) + col + x) + (y * (SENSOR_PER_ROW + 1));
  print("strip");
  println(strip);
  return strip;
}


Point getXYForSensor(int sensor, int x, int y) {
  int row = (sensor / SENSOR_PER_ROW);
  int col = (sensor % SENSOR_PER_ROW);
  return Point(row + x, col + y);
}

////Reading serial

void serialRead() {
  int startChar = Serial.read();

  if (startChar == '{') {
    // println("SerialRead");
    String json = "{" + Serial.readStringUntil("}") + "}";
    //println(json);
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, json);
    // print("here1");
    //   JsonArray arr = doc.as<JsonArray>();
    // print("here3");
    //println(arr.size());
    // for (int i = 0; i < arr.size(); i++) {

    int sensor = doc["sensor"];
    String distance = doc["distance"];
    print("Sensor");
    println(sensor);
    sensors[sensor] = 255;
    // }
  } else if (startChar == '%') {
    //  pattern += 1;
    //  String result = Serial.readStringUntil('\n');
  } else if (startChar == '?') {
    // Check to see if TEENSY
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.println();
    isStarted = true;
    brightness = 150;
  } else if (startChar >= 0) {
    // discard unknown characters
  }
}

void calibrate() {
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      for (uint8_t z = 0; z < DEPTH; z++) {
        int i = gitPixelNumber(x, y, z);
        if (x == 0) {
          temp_leds[i] = CRGB::Red;
        } else if (x == 1) {
          temp_leds[i] = CRGB::Red;
        } else if (x == 2) {
          temp_leds[i] = CRGB::Red;
        } else if (x == 3) {
          temp_leds[i] = CRGB::Red;
        } else {
          temp_leds[i] = CRGB::DarkRed;
        }
      }
    }
  }

  voxelBlendDisplay(0);
  FastLED.show();
}

// 1 sec. frequency
unsigned long c_interval = 5000;     // the time we need to wait
unsigned long c_previousMillis = 0;  // millis() returns an unsigned long

void calibrate_connective() {
  if ((unsigned long)(millis() - c_previousMillis) >= c_interval) {
    c_previousMillis = millis();
    CRGB color = CRGB(random(60, 250), random(10, 70), random(60, 255));
    for (uint8_t x = 0; x < WIDTH; x++) {
      for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t z = 0; z < DEPTH; z++) {
          int i = gitPixelNumber(x, y, z);
          if (x == 0) {
            leds[i] = color;
          } else if (x == 1) {
            leds[i] = color;
          } else if (x == 2) {
            leds[i] = color;
          } else if (x == 3) {
            leds[i] = color;
          } else {
            leds[i] = color;
          }
        }
      }
    }
  }
  FastLED.show();
}


void print(String s) {
  if (debug) {
    Serial.print(s);
  }
}

void println(String s) {
  if (debug) {
    Serial.println(s);
  }
}


//{"sensor":0, "distance":100}
//{"sensor":1, "distance":100}
//{"sensor":2, "distance":100}