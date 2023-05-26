// Displays a Basketball Scoreboard
// Marco Colli Jun 2023
// 
// This basketball scoreboard displays a running clock, shot clock, team fouls,
// timeouts, period and scores. All functions to manage the clock and the other
// counters are controlled from digital inputs connected to momentary on switches.
// 
// The basketball scoreboard incorporates graphical devices for period and timeout
// indicators. Numeric indicators are maintained by the scoreboard class while the
// graphic indicators are updated by the application directly.
// 
// Digital outputs are set to signal end of period (clock to zero) and end of shot 
// clock (clock to zero).
// 
// Disclaimer: the scoreboard is not a complete implementation of a basketball 
// scoreboard.
//
// Libraries used
// ==============
// MD_MAX72XX available from https://github.com/MajicDesigns/MD_MAX72XX or IDE Library Manager
// MD_UISWitch available from https://github.com/MajicDesigns/MD_MAX72XX or IDE Library Manager
//

#include <MD_MAXPanel.h>
#include <MD_UISwitch.h>
#include "Font5x3.h"
#include "scoreboard.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

// Turn on debug statements to the serial output
#define  DEBUG  0

#if  DEBUG
#define PRINT(s, x)  { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x)    { Serial.print(F(x)); }
#define PRINTD(x)    { Serial.print(x, DEC); }

#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTD(x)

#endif

// Hardware pin definitions. 
// All momentary on switches are initialised INPUT_PULLUP
const uint8_t CLK_CTL_PIN = 2;          // press to start/stop, long press to reset
const uint8_t SHOT_RST_PIN = 3;         // press to reset
const uint8_t SHOT_PSE_PIN = 4;         // press to start/stop
const uint8_t SCORE1_ADD_PIN = 5;       // press to add 1 to score1
const uint8_t SCORE1_DELRST_PIN = 6;    // press to del 1 from score1, long press to reset
const uint8_t SCORE2_ADD_PIN = 7;       // press to add 1 to score2
const uint8_t SCORE2_DELRST_PIN = 8;    // press to del 1 from score2, long press to reset
const uint8_t FOUL1_ADD_PIN = 9;        // press to add 1 to foul1
const uint8_t FOUL2_ADD_PIN = A5;       // press to add 1 to foul2
const uint8_t PERIOD_ADD_PIN = A4;      // press to add to period number
const uint8_t TOUT1_ADD_PIN = A3;       // press to add timout to team 1
const uint8_t TOUT2_ADD_PIN = A2;       // press to add timeout to team 2

const uint8_t CLK_SIREN_PIN = A0;   // activated high when the clock runs out
const uint8_t CLK_SHOT_PIN = A1;    // activated high when the shot clock runs out

// Define pin numbers for individual switches
const uint8_t SW_PIN[] = 
{
  CLK_CTL_PIN,
  SHOT_RST_PIN, SHOT_PSE_PIN,
  SCORE1_ADD_PIN, SCORE1_DELRST_PIN,
  SCORE2_ADD_PIN, SCORE2_DELRST_PIN,
  FOUL1_ADD_PIN, FOUL2_ADD_PIN,
  PERIOD_ADD_PIN,
  TOUT1_ADD_PIN, TOUT2_ADD_PIN, 
};

MD_UISwitch_Digital* sw[ARRAY_SIZE(SW_PIN)];

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

// Scoreboard field identifiers
const uint8_t FLD_CLOCK = 1;
const uint8_t FLD_SHOTCLK = 2;
const uint8_t FLD_SCORE1 = 3;
const uint8_t FLD_SCORE2 = 4;
const uint8_t FLD_FOUL1 = 5;
const uint8_t FLD_FOUL2 = 6;

// Assorted stuff
const uint32_t SHOT_TIME = 24;          // in seconds
const uint32_t PERIOD_TIME = 8 * 60;    // in seconds
const uint32_t MAX_PERIOD = 4;
const uint32_t MAX_FOUL = 9;
const uint32_t MAX_SCORE = 999;
const uint32_t MAX_TIMEOUT = 2;

uint8_t curPeriod = 0;
uint8_t curTimeout[2] = { 0, 0 };

cScoreboard sb(&mp);

void showPeriod(void)
// Period is displayed as 4 empty/filled in rectangles
{
  const uint8_t width = 3;
  const uint8_t height = 2;

  uint8_t x = 9, y = 21;

  for (uint8_t i = 0; i < MAX_PERIOD; i++)
  {
    mp.drawRectangle(x, y, x + width, y - height);
    mp.drawFillRectangle(x + 1, y - 1, x + width - 1, y - height + 1, (curPeriod >= i + 1));
    x += width;
  }
}

void showTimeout(void)
// Show the timeout per team as a bar indicator
{
  const uint8_t startX[ARRAY_SIZE(curTimeout)] = { 4, 20 };
  const uint8_t len = 3;

  for (uint8_t t = 0; t < ARRAY_SIZE(curTimeout); t++)
  {
    for (uint8_t i = 0; i < MAX_TIMEOUT; i++)
      mp.drawHLine(15, startX[t] + (len * i) + i, startX[t] + (len * (i + 1)) + i - 1, curTimeout[t] >= i + 1);
  }
}

void processUI(void)
// Process the UI switches and act according to their function.
{
  MD_UISwitch::keyResult_t state;

  for (uint8_t i = 0; i < ARRAY_SIZE(sw); i++)
  {
    state = sw[i]->read();

    if (state != MD_UISwitch::KEY_NULL)
    {
      switch (SW_PIN[i])
      {
      case CLK_CTL_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
          sb.clockToggle(FLD_CLOCK);
        else if (state == MD_UISwitch::KEY_LONGPRESS)
          sb.clockReset(FLD_CLOCK);
        break;

      case SHOT_RST_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
          sb.clockReset(FLD_SHOTCLK, true);
        break;

      case SHOT_PSE_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
          sb.clockToggle(FLD_SHOTCLK);
        break;

      case SCORE1_ADD_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
        {
          sb.fieldValueAdd(FLD_SCORE1, 1);
          if (sb.fieldGetValue(FLD_SCORE1) > MAX_SCORE)
            sb.fieldSetValue(FLD_SCORE1, 0);
        }
        break;
        
      case SCORE1_DELRST_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
            sb.fieldValueAdd(FLD_SCORE1, -1);
        else if (state == MD_UISwitch::KEY_LONGPRESS)
          sb.fieldSetValue(FLD_SCORE1, 0);
        break;

      case SCORE2_ADD_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
        {
          sb.fieldValueAdd(FLD_SCORE2, 1);
          if (sb.fieldGetValue(FLD_SCORE2) > MAX_SCORE)
            sb.fieldSetValue(FLD_SCORE2, 0);
        }
        break;
        
      case SCORE2_DELRST_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
            sb.fieldValueAdd(FLD_SCORE2, -1);
        else if (state == MD_UISwitch::KEY_LONGPRESS)
          sb.fieldSetValue(FLD_SCORE2, 0);
        break;

      case FOUL1_ADD_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
        {
          sb.fieldValueAdd(FLD_FOUL1, 1);
          if (sb.fieldGetValue(FLD_FOUL1) > MAX_FOUL)
            sb.fieldSetValue(FLD_FOUL1, 0);
        }
        break;

      case FOUL2_ADD_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
        {
          sb.fieldValueAdd(FLD_FOUL2, 1);
          if (sb.fieldGetValue(FLD_FOUL2) > MAX_FOUL)
            sb.fieldSetValue(FLD_FOUL2, 0);
        }
        break;

      case PERIOD_ADD_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
        {
          curPeriod++;
          if (curPeriod > MAX_PERIOD)
            curPeriod = 0;
          showPeriod();
        }
        break;

      case TOUT1_ADD_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
        {
          curTimeout[0]++;
          if (curTimeout[0] > MAX_TIMEOUT)
            curTimeout[0] = 0;
          showTimeout();
        }
        break;

      case TOUT2_ADD_PIN:
        if (state == MD_UISwitch::KEY_PRESS)
        {
          curTimeout[1]++;
          if (curTimeout[1] > MAX_TIMEOUT)
            curTimeout[1] = 0;
          showTimeout();
        }
        break;

      }
    }
  }
}

void setup(void)
{
#if  DEBUG || DEBUG_CLASS
  Serial.begin(57600);
#endif
  PRINTS("\n[MD_MAXPanel_Scoreboard]");

  // display setup
  if (mp.begin())
  {
    mp.setFont(_Fixed_5x3);
    mp.setIntensity(4);
    mp.clear();

    // scoreboard setup
    mp.drawRectangle(4, 38, 26, 23);
    mp.drawHLine(17, 0, 31);
    mp.drawVLine(15, 0, 17);
  }
  else
    PRINTS("\nMD_MAXPanel library failed to initialize.");

  // Display fields setup
  sb.fieldCreate(FLD_CLOCK, 7, 36, cScoreboard::MMSS, 5, false);
  sb.clockCreate(FLD_CLOCK, PERIOD_TIME, false);
  sb.fieldCreate(FLD_SHOTCLK, 12, 29, cScoreboard::SS, 2, true);
  sb.clockCreate(FLD_SHOTCLK, SHOT_TIME, false);

  sb.fieldCreate(FLD_SCORE1, 2, 12, cScoreboard::NUMBER, 3, true);
  sb.fieldCreate(FLD_SCORE2, 18, 12, cScoreboard::NUMBER, 3, true);

  sb.fieldCreate(FLD_FOUL1, 6, 5, cScoreboard::NUMBER, 1, true);
  sb.fieldCreate(FLD_FOUL2, 22, 5, cScoreboard::NUMBER, 1, true);

  showPeriod();

  // Switches setup
  for (uint8_t i = 0; i < ARRAY_SIZE(sw); i++)
  {
    sw[i] = new MD_UISwitch_Digital(SW_PIN[i]);
    sw[i]->begin();
  }

  // Other I/O setup
  pinMode(CLK_SIREN_PIN, OUTPUT);
  pinMode(CLK_SHOT_PIN, OUTPUT);

  PRINTS("\nInitialization complete");
}

void loop(void)
{
  // Main processing
  processUI();
  sb.update();

  // Other stuff here
  digitalWrite(CLK_SHOT_PIN, sb.fieldGetValue(FLD_SHOTCLK) == 0 ? HIGH : LOW);
  digitalWrite(CLK_SIREN_PIN, sb.fieldGetValue(FLD_CLOCK) == 0 ? HIGH : LOW);
}
