#pragma once

// Turn on debug statements to the serial output
#define  DEBUG_CLASS  0

#if  DEBUG_CLASS
#define DBG(s, x)  { Serial.print(F(s)); Serial.print(x); }
#define DBGS(x)    { Serial.print(F(x)); }
#define DBGD(x)    { Serial.print(x, DEC); }

#else
#define DBG(s, x)
#define DBGS(x)
#define DBGD(x)

#endif

// Class to manage scoreboard displayed fields and clocks
class cScoreboard
{
public:
  /**
  * Field type enumerated type specification.
  *
  * Used to define the type of field to the creating functions (eg, number of clock).
  */
  enum fieldType_t
  {
    NUMBER,   ///< a simple number
    MMMSS,    ///< time MMM:SS displayed
    MMSS,     ///< time MM:SS displayed
    SS        ///< time SS display only
  };

/**
 * Class Constructor.
 *
 * Instantiate a new instance of the class.
 *
 * \param mp  pointer to the MD_MAXPanel object used to display the scoreboard
 */
  cScoreboard(MD_MAXPanel* mp) : _mp(mp), _changed(false), _fieldList(nullptr)
  {
    for (auto i = 0; i < MAX_CLOCKS; i++)
      _clock[i].f = nullptr;
  }

  /**
   * Class Destructor.
   *
   * Released allocated memory and does the necessary to clean up once the object is
   * no longer required.
   */
  ~cScoreboard(void)
  {
    while (_fieldList != nullptr)
    {
      field_t *f = _fieldList;

      _fieldList = _fieldList->next;
      delete f;
    }
  }

 //--------------------------------------------------------------
 /** \name Methods for scoreboard management.
  * @{
  */

 /**
  * Update the display.
  *
  * Update the display if there have been any changes. The bForce parameter can be used
  * to force the update even if no changes.
  * 
  * This method should be invoked very frequently as field/clock updates do not automatically 
  * update the display. Ideally this method if called every time through loop().
  *
  * \param bForce if true the display will be updated even if no changes.
  */
  void update(bool bForce = false)
  {
    processClocks();

    if (_changed or bForce)
    {
      field_t* f = _fieldList;

      while (f != nullptr)
      {
        char sz[f->size+10];

        switch (f->type)
        {
        case MMMSS: formatTime(sz, f->value, 3, f->leadZero);   break;
        case MMSS:  formatTime(sz, f->value, 2, f->leadZero);   break;
        default:    formatNum(sz, f->value, f->size, f->leadZero); break;
        }
        _mp->drawText(f->x, f->y, sz);

        f = f->next;
      }
      _mp->update();
      _changed = false;
    }
  }

  /** @} */


  //--------------------------------------------------------------
  /** \name Methods for field management.
   * @{
   */

   /**
    * Define a display field.
    *
    * Add a field to the list for the class to manage. All the relevalnt parameers are passed
    * to this method for the field to be defined and added.
    * 
    * The field identifier supplied arbitrary but must be uique to identify this field in future 
    * transactions.
    * 
    * Clock fields will need to be associated with a clock using the clockCreate() method.
    *
    * \sa clockCreate(), fieldSetLeadZero(), fieldSetLength()
    *
    * \param id       the field unique identifier.
    * \param x        the x coordinate for the top left corner of the field in the display.
    * \param y        the y coordinate for the top left corner of the field in the display.
    * \param type     the type of field. One of the fieldType_t values.
    * \param size     the length of the field in characters.
    * \param leadZero true if the field displays with leading zeroes.
    * \return true if the field was created successfully, false otherwise.
    */
  bool fieldCreate(uint8_t id, uint8_t x, uint8_t y, fieldType_t type, uint8_t size, bool leadZero)
  {
    field_t* f = nullptr;

    if (findField(id) == nullptr)
    {
      field_t* f = new(field_t);

      if (f != nullptr)
      {
        // adjust linked list stuff
        f->next = _fieldList;
        _fieldList = f;

        // initialise the fields
        f->id = id;
        f->type = type;
        f->x = x;
        f->y = y;
        f->value = 0;
        f->leadZero = leadZero;
        f->size = size;
        _changed = true;
      }
    }

    return(f != nullptr);
  }

  /**
   * Set if field has leading zeros.
   *
   * Set isf the field is displayed with leading zeroes when displayed. The field
   * will be padded out with zeroes to the size of the field.
   *
   * \sa fieldCreate(), fieldSetLength()
   *
   * \param id    the field identifier.
   * \param state true if the field has leadig zeroes.
   * \return true if the change was successful.
   */
  bool fieldSetLeadZero(uint8_t id, bool state = true)
  {
    field_t* f = findField(id);

    if (f != nullptr)
    {
      f->leadZero = state;
      _changed = true;
    }

    return(f != nullptr);
  }

  /**
   * Set the field display length.
   *
   * Set the length of the field in characters when displayed.
   *
   * \sa fieldCreate(), fieldSetLeadZero()
   *
   * \param id    the field identifier.
   * \param size  the length of the field in characters.
   * \return true if the change was successful.
   */
  bool fieldSetSize(uint8_t id, uint8_t size)
  {
    field_t* f = findField(id);

    if (f != nullptr)
    {
      f->size = size;
      _changed = true;
    }

    return(f != nullptr);
  }

  /**
   * Adjust a field value.
   *
   * Adjust the specified field by the amount specified.
   *
   * \sa fieldSetValue(), fieldGetValue()
   *
   * \param id    the field identifier.
   * \param delta the amount to add to the field. DEfault 1 and can be negative.
   */
  void fieldValueAdd(uint8_t id, int8_t delta = 1)
  {
    field_t* f = findField(id);

    if (f != nullptr)
    {
      if (delta < 0 && abs(delta) > f->value)
        f->value = 0;
      else
        f->value += delta;
      _changed = true;
    }
  }

  /**
   * Set a field value.
   *
   * Set the current value of the specified field.
   *
   * \sa fieldGetValue(), fieldValueAdd()
   *
   * \param id  the field identifier.
   */
  void fieldSetValue(uint8_t id, uint32_t v)
  {
    field_t* f = findField(id);

    if (f != nullptr)
    {
      f->value = v;
      _changed = true;
    }
  }

  /**
   * Get a field value.
   *
   * Return the current value of the specified field.
   *
   * \sa fieldSetValue()
   *
   * \param id  the field identifier.
   * \return the field value
   */
  uint32_t fieldGetValue(uint8_t id)
  {
    uint32_t v = 0;
    field_t* f = findField(id);

    if (f != nullptr) v = f->value;

    return(v);
  }

  /** @} */

  //--------------------------------------------------------------
  /** \name Methods for clock management.
   * @{
   */

 /**
  * Define a field as a clock.
  *
  * Associate an existing field with a clock. Up to MAX_CLOCKS can be defined.
  * 
  * The clock is managed by the class separately and updated into the specified 
  * field.
  * For a clock that counts up, the clock is initialized to 0 and counts until limit is reached.
  * For a clock that counts down, the clock is initialized to limit and stops when it reaches 0.
  *
  * \sa fieldCreate(), clockReset()
  *
  * \param fieldId  the field identifier for this clock.
  * \param limit    the limit for the clock in seconds.
  * \param countUp  true to start at zero and count up (default), false to start at limit and count down.
  * \return true if the clock was created successfully, false otherwise.
  */
  bool clockCreate(uint8_t fieldId, uint32_t limit, bool countUp = true)
  {
    uint8_t idx;
    field_t *f;

    DBG("\nclockCreate ", fieldId);

    // find an empty slot
    for (idx = 0; idx < MAX_CLOCKS; idx++)
      if (_clock[idx].f == nullptr) break;
    if (idx >= MAX_CLOCKS)
    {
      DBGS(" - no slots");
      return(false);    // no slots left
    }

    // find the field using id
    f = findField(fieldId);
    if (f == nullptr)
    {
      DBGS(" - no field id");
      return(false);    // no field id
    }

    // set up the clock structure
    _clock[idx].f = f;
    _clock[idx].timeLimit = limit;
    _clock[idx].countUp = countUp;
    _clock[idx].stopped = true;
    clockReset(fieldId);

    return(true);
  }

  /**
   * Toggle clock running state.
   *
   * Toggle the specified clock between running/stopped states.
   *
   * \sa clockCreate(), clockStart(), clockStop(), isClockStopped()
   *
   * \param fieldId  the field identifier for this clock.
   */
  void clockToggle(uint8_t fieldId)
  {
    int8_t idx;

    DBG("\nclockToggle ", fieldId);
    if ((idx = findClock(fieldId)) == -1)
    {
      DBGS(" - clock not found");
      return;
    }

    if (_clock[idx].stopped)
      clockStart(fieldId);
    else
      clockStop(fieldId);
  }

  /**
   * Start clock running.
   *
   * Start the specified clock running.
   *
   * \sa clockCreate(), clockToggle(), clockStop(), isClockStopped()
   *
   * \param fieldId  the field identifier for this clock.
   */
  void clockStart(uint8_t fieldId)
  {
    int8_t idx;

    DBG("\nclockStart ", fieldId);
    if ((idx = findClock(fieldId)) == -1)
    {
      DBGS(" - no field id");
      return;
    }

    _clock[idx].timeLast = millis();
    _clock[idx].stopped = false;
  }

  /**
   * Stop clock running.
   *
   * Stop the specified clock.
   *
   * \sa clockCreate(), clockToggle(), clockStart(), isClockStopped()
   *
   * \param fieldId  the field identifier for this clock.
   */
  void clockStop(uint8_t fieldId)
  {
    int8_t idx;

    DBG("\nclockStop ", fieldId);
    if ((idx = findClock(fieldId)) == -1)
    {
      DBGS(" - no field id");
      return;
    }

    _clock[idx].timeToGo = _clock[idx].timeToGo - (millis() - _clock[idx].timeLast);
    _clock[idx].stopped = true;
  }

  /**
   * Return clock run/stop state.
   *
   * Return the specified clock's run/stop state.
   *
   * \sa clockCreate(), clockToggle(), clockStart(), clockStop()
   *
   * \param fieldId  the field identifier for this clock.
   * \return true if the clock is stopped.
   */
  bool isClockStopped(uint8_t fieldId)
  {
    int8_t idx;

    if ((idx = findClock(fieldId)) == -1)
    {
      DBGS(" - no field id");
      return(false);
    }

    return(_clock[idx].stopped);
  }

  /**
   * Reset a clock.
   *
   * Reset the specified clock to its initial values and stops the clock unless overridden
   * by maintainRunMode.
   *
   * For a clock that counts up, the clock is initialized to 0.
   * For a clock that counts down, the clock is initialized to limit.
   *
   * \sa clockCreate()
   *
   * \param fieldId          the field identifier for this clock.
   * \param maintainRunMode  false to stop the clock after reste (default), true to keep it running.
   */
  void clockReset(uint8_t fieldId, bool maintainRunMode = false)
  {
      int8_t idx;

      DBG("\nclockReset ", fieldId);
      if ((idx = findClock(fieldId)) == -1)
      {
        DBGS(" - no field id");
        return;
      }

      _clock[idx].f->value = _clock[idx].countUp ? 0 : _clock[idx].timeLimit;
      _clock[idx].timeToGo = CLOCK_PERIOD;
      if (maintainRunMode)
        _clock[idx].timeLast = millis();    // restart the period and keep running
      else
        _clock[idx].stopped = true;         // stop the clock
      _changed = true;
  }

  /** @} */

private:
  static const uint8_t  MAX_CLOCKS = 3;       ///< maximum number of closks for this class
  static const uint32_t CLOCK_PERIOD = 1000;  ///< 1 second in milliseconds

  // Define data to keep track of fields
  struct field_t
  {
    uint8_t id;       ///< identifier
    fieldType_t type; ///< field type
    uint8_t x, y;     ///< field coordinates
    uint32_t value;   ///< current value of the field
    bool leadZero;    ///< field has leading zeroes
    uint8_t size;     ///< field size in characters/numbers
    field_t* next;    ///< next in the list
  };

  struct clock_t
  {
    field_t* f;           ///< field associated with this timer
    uint32_t timeLimit;   ///< upper limit for time
    uint32_t timeLast;    ///< last time displayed/updated
    uint32_t timeToGo;    ///< time to go before next update
    bool countUp;         ///< counting up if true
    bool stopped;         ///< true if stopped
  };

  MD_MAXPanel *_mp;           ///< MD_MAXPanel object used for display
  bool _changed;              ///< true if there has been a change to the display since last update()
  field_t* _fieldList;        ///< root pointer of the field linked list
  clock_t _clock[MAX_CLOCKS]; ///< array of clocks available for use

  //--------------------------------------------------------------
  // Helper functions

  int8_t findClock(uint8_t id)
  // find the clock associated with field id
  // return _clock array index or -1 if none found
  {
    int8_t idx;

    for (idx = 0; idx < MAX_CLOCKS; idx++)
      if (_clock[idx].f != nullptr)
        if (_clock[idx].f->id == id) break;
    if (idx >= MAX_CLOCKS) idx = -1;  // no such field

    return(idx);
  }

  // ----- Field List Management Methods

  field_t* findField(uint8_t id)
  // Find the first list item with the id specified
  // Return a pointer to the structure or nullptr if not found
  {
    field_t* f = _fieldList;

    while (f != nullptr)
    {
      if (f->id == id)
        break;
      f = f->next;
    }

    return(f);
  }

  // ----- Helper functions
  void processClocks(void)
  {
    uint32_t now = millis();

    for (auto i = 0; i < MAX_CLOCKS; i++)
    {
      if (_clock[i].f != nullptr && !_clock[i].stopped)    // a valid clock entry and one we should process
      {
        if (now - _clock[i].timeLast >= _clock[i].timeToGo)   // check if the time has expired
        {
          DBG("\nClock[", i);
          DBGS("] tick");
          // adjust clock value by time period
          _clock[i].f->value += (_clock[i].countUp ? 1 : -1);

          // check clock boundaries and pause when reached
          _clock[i].stopped = (_clock[i].countUp && _clock[i].f->value == _clock[i].timeLimit) ||
            (!_clock[i].countUp && _clock[i].f->value == 0);
          DBG(" pause ", _clock[i].stopped);

          // adjust timeToGo for next period, accounting 
          // for potential millis() overruns this current period
          _clock[i].timeToGo = CLOCK_PERIOD + (_clock[i].timeLast - now + _clock[i].timeToGo);
          DBG(" to go ", _clock[i].timeToGo);
          _clock[i].timeLast = now;
          _changed = true;
        }
      }
    }
  }

  void formatNum(char* sz, uint32_t n, uint8_t size, bool leadZero)
  { 
    int8_t idx = size - 1;

    // initialise the buffer
    memset(sz, (leadZero ? '0' : ' '), size);
    sz[size] = '\0';

    do
    {
      sz[idx--] = (n % 10) + '0';
      n /= 10;
    }
    while (n != 0 && idx >= 0);
  }

  void formatTime(char* sz, uint32_t n, uint8_t m, bool leadZero)
  {
    formatNum(sz, n / 60, m, leadZero);
    sz[m] = ':';
    formatNum(&sz[m+1], n % 60, 2, true);
    sz[m+3] = '\0';
  }
};