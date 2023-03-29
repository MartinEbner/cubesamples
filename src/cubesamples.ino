SYSTEM_MODE(MANUAL);

#include "L3D.h"
#include <stdint.h>
#include <math.h>

#define SAMPLE 2

#define N 8
#define C 50
#define min(a,b) (a < b ? a : b)
#define CSHIFT(c,s) (unsigned char)((c + s)%C + C_MIN)
#define PULSE_NORM(c,pulse) ((uint8_t)((uint32_t)(c) * (uint32_t)(pulse) / C))
#define NORM(c) ((uint8_t)((uint32_t)(c) * (uint32_t)C / (uint32_t)UINT8_MAX))
#define TABLE_SIZE 1024
#define PI 3.14159265358979323846
#define C_MIN 5
#define HZ_TO_US(f) (uint32_t)((float)1000000.0 / (float)(f))
#define T_SCHUMANN_RESONANCE HZ_TO_US(7.83)

Cube cube=Cube(N,C);
static int32_t statSineTable[TABLE_SIZE];

class Pulse
{
  private:
  uint32_t n;
  public:
  Pulse(void):n(0){}
  uint8_t intensity(uint32_t dn)
  {
    int32_t const i32to8 = UINT32_MAX / (C - C_MIN);
    int32_t sinN = statSineTable[n];
    uint8_t sinN8 = (uint8_t)((sinN / i32to8) + (C / 2) + C_MIN/2);
    n = (n + dn) % TABLE_SIZE;
    return sinN8;
  }
};


void setup()
{
  Serial.begin(9600);
  cube.begin();
	cube.background(black);
  for (int i = 0; i < TABLE_SIZE; i++)
  {
    double x = i * 2.0 * PI / TABLE_SIZE;
    double sin_x = sin(x);
    statSineTable[i] = (int32_t) (sin_x * (double) INT32_MAX);
  }
}

static void sample1()
{
  int const blinkRate = 6; /* ms */
  static int x0 = 0;
  static int y0 = 3 * N;
  static int z0 = 6 * N;
  int const D = 15;
  static Color color       = Color(0x00, 0x00, 0x00);
  static Pulse pulse;
  uint8_t slowPulseIntensity = pulse.intensity(1);

  /* Pulsing background: R/G/B pulse with different frequencies, total intensity also has different frequency (multipe of coler frequencies) */
  static Pulse pulseIntensity;
  static Pulse pulseRed;
  static Pulse pulseGreen;
  static Pulse pulseBlue;
  uint8_t intensity = pulseIntensity.intensity(3*7)/8;
  Color bgcolor       = Color(PULSE_NORM(pulseRed.intensity(1), intensity),
                            PULSE_NORM(pulseGreen.intensity(3), intensity),
                            PULSE_NORM(pulseBlue.intensity(7), intensity));
  for (int x = 0; x < N; x++)
    for (int y = 0; y < N; y++)
      for (int z = 0; z < N; z++)
        if ((x < 2) || (x >= (N - 2)) || (y < 2) || (y >= (N - 2)) || (z < 2) || (z >= (N - 2)))
          cube.setVoxel(x,y,z,bgcolor);

  /* shifting xyz space coordinates and color, 3-phase color emphasizing */
  for (int x = 2; x < N - 2; x++)
    for (int y = 2; y < N - 2; y++)
      for (int z = 2; z < N - 2; z++)
      {
	      cube.setVoxel(
          (x0/9+x)%N,
          (y0/9+y)%N,
          (z0/9+z)%N,
          Color(
            PULSE_NORM(CSHIFT(color.red,(x0+D*x)/D), slowPulseIntensity),
            PULSE_NORM(CSHIFT(color.green,(y0+D*y)/D), slowPulseIntensity),
            PULSE_NORM(CSHIFT(color.blue,(z0+D*z)/D), slowPulseIntensity)));
      }

	cube.show();
	delay(blinkRate);
  x0 = (x0+1)%(N*9);
  y0 = (y0+1)%(N*9);
  z0 = (z0+1)%(N*9);
}

static void sample2()
{
  static Pulse pulseIntensity;
  static Pulse pulseRed;
  static Pulse pulseGreen;
  static Pulse pulseBlue;
  static Color const* colors[] = {&blue, &grey, &yellow, &magenta, &orange, &teal, &red, &brown, &pink, &blue, &green, &purple, &white};
  static uint8_t numColors = sizeof(colors) / sizeof(colors[0]);
  static uint32_t colorIdx = 0;
  Color color;
  for (int x = 0; x < N; x++)
    for (int y = 0; y < N; y++)
      for (int z = 0; z < N; z++)
      {
        if (((x + y + z) % 2) == 0)
        {
          Color const* c = colors[colorIdx % numColors];
          color = Color(NORM(c->red), NORM(c->green), NORM(c->blue));
        }
        else
        {
          Color const* c = colors[(colorIdx + 1) % numColors];
          color = Color(NORM(c->red), NORM(c->green), NORM(c->blue));
        }
        cube.setVoxel(x, y, z, color);
      }
	cube.show();
  /* Color const* c = colors[colorIdx % numColors];
  Serial.printlnf("color %lu, RGB %02x %02x %02x ", colorIdx % numColors, NORM(c->red), NORM(c->green), NORM(c->blue));
  Serial.printlnf("T_SCHUMANN_RESONANCE = %lu", T_SCHUMANN_RESONANCE); */
	delayMicroseconds(T_SCHUMANN_RESONANCE);
  colorIdx++;
}

static void sample()
{
  switch(SAMPLE)
  {
    case 1:
    sample1();
    break;
    case 2:
    sample2();
    break;
  }
}

void loop()
{
  static bool isFirstCycle = true;

  if (isFirstCycle)
  {
    isFirstCycle = false;
    delay(1000);
    Serial.printlnf("cubesample SAMPLE=%d, built at %s", SAMPLE, __DATE__ " " __TIME__);
  }
  sample();
}
