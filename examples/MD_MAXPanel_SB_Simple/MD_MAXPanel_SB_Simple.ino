// Displays a Simple Scoreboard
// Marco Colli Jun 2023
//
// This simple scoreboard displays a running clock and two scores. All functions to 
// manage the clock and the score are controlled from digital inputs connected to 
// momentary on switches.
// 
// Libraries used
// ==============
// MD_MAX72XX available from https://github.com/MajicDesigns/MD_MAX72XX or IDE Library Manager
// MD_UISWitch available from https://github.com/MajicDesigns/MD_UISwitch or IDE Library Manager
//

#include <MD_MAXPanel.h>
#include <MD_UISwitch.h>
#include "Font7x5.h"
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
const uint8_t SCORE1_ADD_PIN = 5;       // press to add 1 to score1
const uint8_t SCORE1_DELRST_PIN = 6;    // press to del 1 from score1, long press to reset
const uint8_t SCORE2_ADD_PIN = 7;       // press to add 1 to score2
const uint8_t SCORE2_DELRST_PIN = 8;    // press to del 1 from score2, long press to reset

// Define pin numbers for individual switches
const uint8_t SW_PIN[] = 
{
  CLK_CTL_PIN,
  SCORE1_ADD_PIN, SCORE1_DELRST_PIN,
  SCORE2_ADD_PIN, SCORE2_DELRST_PIN,
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
const uint8_t FLD_SCORE1 = 2;
const uint8_t FLD_SCORE2 = 3;

// Assorted stuff
const uint32_t MAX_TIME = (999L*60)+99L;    // 999:99 in seconds
const uint32_t MAX_SCORE = 999;

cScoreboard sb(&mp);

void processUI(void)
// Process the switches and act according to their function
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

  // Display setup
  if (mp.begin())
  {
    mp.setFont(_Fixed_7x5);
    mp.setRotation(MD_MAXPanel::ROT_90);
    mp.setIntensity(4);
    mp.clear();

    // Scoreboard lines setup
    mp.drawHLine(17, 0, 39);
    mp.drawVLine(19, 0, 17);
    mp.drawVLine(20, 0, 17);
  }
  else
    PRINTS("\nMD_MAXPanel library failed to initialize.");

  // Display field setup
  sb.fieldCreate(FLD_CLOCK, 1, 28, cScoreboard::MMMSS, 6, false);
  sb.fieldCreate(FLD_SCORE1,  4, 11, cScoreboard::NUMBER, 2, true);
  sb.fieldCreate(FLD_SCORE2, 25, 11, cScoreboard::NUMBER, 2, true);

  sb.clockCreate(FLD_CLOCK, MAX_TIME);

  // Switches setup
  for (uint8_t i = 0; i < ARRAY_SIZE(sw); i++)
  {
    sw[i] = new MD_UISwitch_Digital(SW_PIN[i]);
    sw[i]->begin();
  }

  PRINTS("\nInitialization complete");
}

void loop(void)
{
  processUI();
  sb.update();
}
