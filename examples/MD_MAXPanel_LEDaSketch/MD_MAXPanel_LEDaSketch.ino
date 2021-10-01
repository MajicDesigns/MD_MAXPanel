// Implements an Etch-a-sketch type drawing game using MD_MAXPanel library
//
// Hardware used
// =============
// Momentary On push switch on SWITCH_PIN to clear the display. The digital I/O  
// will be initialize INPUT_PULLUP.
// Either 2 pots (or PS2 joystick) connected to XAXIS_PIN, YAXIS_PIN
//
// Libraries used
// ==============
// MD_MAX72XX available from https://github.com/MajicDesigns/MD_MAX72XX
//
// Rules of the Game
// =================
// Moving the controls to draw a path of lit LEDS. Press the SWITCH_PIN to clear
// the display. This is modelled on the Etch-a-Sketch game.
//

#include <MD_MAXPanel.h>

// Turn on debug statements to the serial output
#define  DEBUG  0

#if  DEBUG
#define PRINT(s, x)   { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x)     { Serial.print(F(x)); }
#define PRINTD(x)     { Serial.print(x, DEC); }
#define PRINTXY(s, x, y) { Serial.print(s); Serial.print(F("(")); Serial.print(x); Serial.print(F(",")); Serial.print(y); Serial.print(F(")")); }

#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTD(x)
#define PRINTXY(s, x, y)

#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
const MD_MAX72XX::moduleType_t HARDWARE_TYPE = MD_MAX72XX::FC16_HW;
const uint8_t X_DEVICES = 4;
const uint8_t Y_DEVICES = 5;

const uint8_t CLK_PIN = 13;   // or SCK
const uint8_t DATA_PIN = 11;  // or MOSI
const uint8_t CS_PIN = 10;    // or SS

// SPI hardware interface
MD_MAXPanel mp = MD_MAXPanel(HARDWARE_TYPE, CS_PIN, X_DEVICES, Y_DEVICES);
// Arbitrary pins
// MD_MAXPanel mx = MD_MAXPanel(HARWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, X_DEVICES, Y_DEVICES);

const uint8_t XAXIS_PIN = A0;
const uint8_t YAXIS_PIN = A1;
const uint8_t SWITCH_PIN = A2;

// We always wait a bit between updates of the display
const uint16_t TICK_TIME = 300;  // in milliseconds

void setup(void)
{
#if  DEBUG
  Serial.begin(57600);
#endif
  PRINTS("\n[MD_MAXPanel LED-a-Sketch]");

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(XAXIS_PIN, INPUT);
  pinMode(YAXIS_PIN, INPUT);

  mp.begin();
  mp.clear();
}

void loop(void)
{
  static uint32_t timeLastRun = 0;
  static uint16_t x, y;

  if (digitalRead(SWITCH_PIN) == LOW)
    mp.clear();     // mark the end of the display ...

  // Check if next generation time
  if (millis() - timeLastRun >= TICK_TIME)
  {
    timeLastRun = millis();
    nextPixel(x, y);
  }
}

void nextPixel(uint16_t &x, uint16_t &y)
// Read the user input and move the head of the sketch
{
  int16_t dx = analogRead(XAXIS_PIN);
  int16_t dy = analogRead(YAXIS_PIN);

  // PS2 joystick Analog value returned is 0..1024. Remap this to be -1, 0 or 1
  const uint16_t RANGE = 1024;
  const uint16_t LTHRESHOLD = RANGE/3;
  const uint16_t HTHRESHOLD = (2*RANGE)/3;

  if (dx <= LTHRESHOLD) dx = -1;
  else if (dx >= HTHRESHOLD) dx = 1;
  else dx = 0;  
  if (dy <= LTHRESHOLD) dy = -1;
  else if (dy >= HTHRESHOLD) dy = 1;
  else dy = 0;  

  PRINT("\n-- Next pixel dx:", dx);
  PRINT(" dy:", dy);

  if ((x != 0 && dx < 0) || (x < mp.getXMax() && dx > 0))
    x += dx;
  if ((y != 0 && dy < 0) || (y < mp.getYMax() && dy > 0))
    y += dy;

  mp.setPoint(x, y);
}
