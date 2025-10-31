// Program to exercise the MD_MAXPanel library
//
// Pixel elements are attracted and move to one of the attaction points 
// defined in the attractor array. The attractor point is selected by a 
// digital input switch to move the position between locations.
//
// Libraries used
// ==============
// MD_MAX72XX available from https://github.com/MajicDesigns/MD_MAX72XX
// TrueRandom available from https://github.com/sirleech/TrueRandom
//

#include <MD_MAXPanel.h>
#include <TrueRandom.h>

#define DEBUG 0

#ifdef DEBUG
#define PRINTS(s)     do { Serial.print(F(s)); } while(false);
#define PRINT(s, v)   do { Serial.print(F(s)); Serial.print(v); } while(false);
#define PRINTP(s, p)  do { Serial.print(F(s)); Serial.print(F(" [")); Serial.print(p.x); Serial.print(F(",")); Serial.print(p.y); Serial.print(F("]")); } while(false);
#else
#define PRINTS(s)
#define PRINT(s, v)
#define PRINTP(s, p)
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
// MD_MAXPanel mx = MD_MAXPanel(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, X_DEVICES, Y_DEVICES);

// Miscellaneous
const uint8_t SWITCH_PIN = 9;     // pin number for attractor change switch
const uint32_t STEP_TIME = 200;   // step time in milliseconds

// Define a point structure and tracking data
const uint16_t MAX_ELEMENTS = 100;

struct point_t
{
  int16_t xOld, yOld;     // current coordinates
  int16_t x, y;           // new coordinates
};

point_t elements[MAX_ELEMENTS];   // dynamically allocated in setup()

// define the attractor array and tracking variables
const uint8_t NUM_ATTRACTORS = 4;       // number of attractor points
point_t attractor[NUM_ATTRACTORS];      // variable constant values initialise in setup();
uint8_t attractorId = 0;                // current active attractor point

int32_t d2(point_t p, point_t q)
// work out the distance^2 between 2 points
{
  int32_t dx, dy;

  dx = (p.x - q.x);
  dx *= dx;
  dy = (p.y - q.y);
  dy *= dy;

  return (dx+dy);
}

int16_t findElement(point_t p)
// Return the element index for the point p in the element array.
// If not found return -1.
{
  for (uint16_t i = 0; i < MAX_ELEMENTS; i++)
    if (elements[i].x == p.x && elements[i].y == p.y)
      return(i);

  return(-1);
}

void moveElement(point_t &p)
{
  point_t q;

  // set up the minimum to be the current point
  int32_t dMin = d2(p, attractor[attractorId]);
  int8_t dxMin = 0, dyMin = 0;

  for (int8_t dx = -1; dx <= 1; dx++)
  {
    for (int8_t dy = -1; dy <= 1; dy++)
    {
      // new coordinates to test
      q.x = p.x + dx;
      q.y = p.y + dy;

      // avoid out of bounds conditions
      if (q.x >= 0 && q.x <= mp.getXMax() && q.y >= 0 && q.y <= mp.getYMax())
      {
        //work out the distance to the attractor
        int32_t d = d2(q, attractor[attractorId]);

        // check for this being a new minimum and empty space
        if (d < dMin && findElement(q) == -1)
        {
          dxMin = dx;
          dyMin = dy;
          dMin = d;
        }
      }
    }
  }

  // save the minimum (which could be the same point)
  p.x += dxMin;
  p.y += dyMin;
}

void moveAll(void)
{
  for (uint16_t i = 0; i < MAX_ELEMENTS; i++)
    moveElement(elements[i]);
}

void updateDisplay(void)
// redraw the pixels on screen
{
  mp.update(false);
  for (uint16_t i = 0; i < MAX_ELEMENTS; i++)
  {
    mp.setPoint(elements[i].xOld, elements[i].yOld, false);
    mp.setPoint(elements[i].x, elements[i].y, true);
    elements[i].xOld = elements[i].x;
    elements[i].yOld = elements[i].y;
  }
  mp.update(true);
}

void processSwitch(void)
{
  static uint8_t lastState = HIGH;
  uint8_t state = digitalRead(SWITCH_PIN);

  if (state == LOW && lastState == HIGH)
  {
    attractorId++;
    if (attractorId >= NUM_ATTRACTORS) attractorId = 0;
    delay(50);
  }
  lastState = state;
}

void setup(void)
{
#if DEBUG
  Serial.begin(57600);
  PRINTS("\n[MAXPanel Attractor Debug]\n")
#endif

  // initialise digital input
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // initialize panel
  mp.begin();
  mp.clear();

  // initialise the attractor array
  attractor[0].x = mp.getXRange()/2;  attractor[0].y = 0;
  attractor[1].x = 0;                 attractor[1].y = mp.getYRange()/2;
  attractor[2].x = mp.getXRange()/2;  attractor[2].y = mp.getYMax();
  attractor[3].x = mp.getXMax();      attractor[3].y = mp.getYRange()/2;

  // initialize the elements array
  // first to empty value
  for (uint16_t i = 0; i < MAX_ELEMENTS; i++)
    elements[i].x = elements[i].y = -1;
    
  // then to random values
  PRINTS("\n");
  for (uint16_t i = 0; i < MAX_ELEMENTS; i++)
  {
    point_t p;
    do
    {
      p.x = TrueRandom.random(mp.getXRange());
      p.y = TrueRandom.random(mp.getYRange());
    } while (findElement(p) != -1);

    elements[i].xOld = elements[i].x = p.x;
    elements[i].yOld = elements[i].y = p.y;
    //PRINTP("", elements[i]);
  }

  updateDisplay();
}

void loop(void)
{
  static uint32_t timeLast = 0;

  if (millis() - timeLast >= STEP_TIME)
  {
    PRINTS("\n--> TICK");
    moveAll();
    updateDisplay();
    timeLast += STEP_TIME;
  }
  processSwitch();
}
