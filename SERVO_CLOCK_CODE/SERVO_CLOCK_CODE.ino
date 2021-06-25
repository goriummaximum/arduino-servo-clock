#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "RTClib.h"

Adafruit_PWMServoDriver pwmA = Adafruit_PWMServoDriver(0x41);
Adafruit_PWMServoDriver pwmB = Adafruit_PWMServoDriver(0x40);

RTC_DS3231 rtc;

#define SERVO_FREQ 50
#define PULSE_MAX 525
#define PULSE_MIN 80

#define G A0
#define R A1
#define B A2

short keyboardColumn[4] = {10, 9, 8, 7};
short keyboardRow[5] = {2, 3, 4, 5, 6};

enum state
{
  ST_SHOW_HOUR_MINUTE,
  ST_SHOW_MONTH_DAY,
  ST_SHOW_YEAR,
  ST_COUNT_DOWN
};

struct timeFormat
{
  short yearThousands = 8;
  short yearHundreds = 8;
  short yearTens = 8;
  short yearUnits = 8;

  short monthTens = 8;
  short monthUnits = 8;

  short dayTens = 8;
  short dayUnits = 8;
  
  short hourTens = 8;                 //Create variables to store each 7 segment display numeral
  short hourUnits = 8;
  
  short minuteTens = 8;
  short minuteUnits = 8;
};

state st;

timeFormat currTime;
timeFormat prevTime;

short segmentAOn[14] = {70,90,0,5,95,10,10,70,70,5,10,70,10,10};  //Degree
short segmentAOff[14] = {20,30,60,60,40,65,60,15,15,60,60,15,70,60};
short segmentBOn[14] = {70,90,12,10,95,10,10,70,65,13,13,80,20,10};
short segmentBOff[14] = {15,25,65,70,30,70,65,15,5,75,70,15,80,60};

short digits[10][7] = {{1,1,1,1,1,1,0},{0,1,1,0,0,0,0},{1,1,0,1,1,0,1},{1,1,1,1,0,0,1},{0,1,1,0,0,1,1},
                     {1,0,1,1,0,1,1},{1,0,1,1,1,1,1},{1,1,1,0,0,0,0},{1,1,1,1,1,1,1},{1,1,1,1,0,1,1}}; 

//FUNCTIONS
void showHourMinute();
void showMonthDay();
void showYear();
void countdown();

short processKeyboard();
short processAngle(short ang);
void processTime();
void displayNumber(short curr_value, short prev_value, short display_position);
boolean runEvery(unsigned long interval);

void setup() {
  //Serial.begin(9600);
  //PCA
  pwmA.begin();
  pwmA.setOscillatorFrequency(27000000);
  pwmA.setPWMFreq(SERVO_FREQ);

  pwmB.begin();
  pwmB.setOscillatorFrequency(27000000);
  pwmB.setPWMFreq(SERVO_FREQ);
  yield();

  //RTC
  while(!rtc.begin());

  if (rtc.lostPower())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    //rtc.adjust(DateTime(2021, 2, 8, 13, 18, 0));
  }
  //LED
  pinMode(G, OUTPUT);
  pinMode(R, OUTPUT);
  pinMode(B, OUTPUT);

  //KEYBOARD
  for (short i = 0; i < 4; i++)
    pinMode(keyboardColumn[i], INPUT_PULLUP);

  for (short i = 0; i < 5; i++)
     pinMode(keyboardRow[i], OUTPUT);
  
  //START
  for(short i = 0 ; i <= 13 ; i++)    //Set all of the servos to on or up (88:88 displayed)
  {
    pwmA.setPWM(i, 0, processAngle(segmentAOn[i]));
  }
  delay(1000);

  for (short i = 0; i <= 13; i++)
  {
    pwmB.setPWM(i, 0, processAngle(segmentBOn[i]));
  }
  delay(1000);

  //Set initial state
  st = ST_SHOW_HOUR_MINUTE;
}

void loop() {
  processTime();
  
  switch (st)
  {
    case ST_SHOW_HOUR_MINUTE:
    showHourMinute();

    if (processKeyboard() == 0)
      st = ST_SHOW_MONTH_DAY;
    else if (processKeyboard == 1)
      st = ST_COUNT_DOWN;
      
    break;

    case ST_SHOW_MONTH_DAY:
    showMonthDay();

    if (processKeyboard() == 0)
      st = ST_SHOW_YEAR;
    else if (processKeyboard == 1)
      st = ST_COUNT_DOWN;
      
    break;

    case ST_SHOW_YEAR:
    showYear();

    if (processKeyboard() == 0)
      st = ST_SHOW_HOUR_MINUTE;
    else if (processKeyboard == 1)
      st = ST_COUNT_DOWN;
      
    break;

    case ST_COUNT_DOWN:
    countdown();
    break;

    default:
    break;
  }
}

void showHourMinute()
{
   if (runEvery(3000))
    {    
    if (prevTime.hourTens != currTime.hourTens) displayNumber(currTime.hourTens, prevTime.hourTens, 0);
    if (prevTime.hourUnits != currTime.hourUnits) displayNumber(currTime.hourUnits, prevTime.hourUnits, 1);
    if (prevTime.minuteTens != currTime.minuteTens) displayNumber(currTime.minuteTens, prevTime.minuteTens, 2);
    if (prevTime.minuteUnits != currTime.minuteUnits)  displayNumber(currTime.minuteUnits, prevTime.minuteUnits, 3);

    prevTime.hourTens = currTime.hourTens;
    prevTime.hourUnits = currTime.hourUnits;
    prevTime.minuteTens = currTime.minuteTens;
    prevTime.minuteUnits = currTime.minuteUnits;
  }
}

void showMonthDay()
{
  if (runEvery(3000))
    {
    if (prevTime.dayTens != currTime.dayTens) displayNumber(currTime.dayTens, prevTime.dayTens, 0);
    if (prevTime.dayUnits != currTime.dayUnits)  displayNumber(currTime.dayUnits, prevTime.dayUnits, 1);
    if (prevTime.monthTens != currTime.monthTens) displayNumber(currTime.monthTens, prevTime.monthTens, 2);
    if (prevTime.monthUnits != currTime.monthUnits) displayNumber(currTime.monthUnits, prevTime.monthUnits, 3);

    prevTime.dayTens = currTime.dayTens;
    prevTime.dayUnits = currTime.dayUnits;
    prevTime.monthTens = currTime.monthTens;
    prevTime.monthUnits = currTime.monthUnits;
  }
}

void showYear()
{
  if (runEvery(3000))
    {
    if (prevTime.yearThousands != currTime.yearThousands) displayNumber(currTime.yearThousands, prevTime.yearThousands, 0);
    if (prevTime.yearHundreds != currTime.yearHundreds)  displayNumber(currTime.yearHundreds, prevTime.yearHundreds, 1);
    if (prevTime.yearTens != currTime.yearTens) displayNumber(currTime.yearTens, prevTime.yearTens, 2);
    if (prevTime.yearUnits != currTime.yearUnits) displayNumber(currTime.yearUnits, prevTime.yearUnits, 3);

    prevTime.yearThousands = currTime.yearThousands;
    prevTime.yearHundreds = currTime.yearHundreds;
    prevTime.yearTens = currTime.yearTens;
    prevTime.yearUnits = currTime.yearUnits;
    }
}

void countdown()
{
  
}

short processKeyboard()
{
  short result = -1;
  for (short i = 0; i < 5; i++)
  {
    digitalWrite(keyboardRow[i], HIGH);
  }

  for (short i = 0; i < 5; i++)
  {
    digitalWrite(keyboardRow[i], LOW);
    for (short j = 0; j < 4; j++)
    {
      result++;
      if (digitalRead(keyboardColumn[j]) == LOW)
      {
        return result;
      }
    }
    
    digitalWrite(keyboardRow[i], HIGH);
  }

  return ++result;
}

short processAngle(short ang)
{
  return map(ang, 0, 180, PULSE_MIN, PULSE_MAX);
}

void processTime()
{ 
  DateTime now = rtc.now();
  currTime.yearThousands = now.year() / 1000;
  currTime.yearHundreds = now.year() % 1000 / 100;
  currTime.yearTens = now.year() % 1000 % 100 / 10;
  currTime.yearUnits = now.year() % 1000 % 100 % 10;

  currTime.monthTens = now.month() / 10;
  currTime.monthUnits = now.month() % 10;

  currTime.dayTens = now.day() / 10;
  currTime.dayUnits = now.day() % 10;
  
  currTime.hourTens = now.hour() / 10;
  currTime.hourUnits = now.hour() % 10;
  
  currTime.minuteTens = now.minute() / 10;
  currTime.minuteUnits = now.minute() % 10; 
}

void displayNumber(short curr_value, short prev_value, short display_position)
{
  /*
  if (display_position == 0)
  {
    for (int i = 13; i > 6; i--)
    {
      if (digits[curr_value][i - 7] != digits[prev_value][i - 7])
      {
        if (i == 13)
        {
          pwmH.setPWM(9, 0, processAngle(segmentHOff[9]));
          pwmH.setPWM(11, 0, processAngle(segmentHOff[11]));
          delay(70);
        }

        if (digits[curr_value][i - 7] == 0)
        {
          pwmH.setPWM(i, 0, processAngle(segmentHOff[i]));
        }
      
        else
        {
          pwmH.setPWM(i, 0, processAngle(segmentHOn[i]));
        }
      }
    }
  }
  
  else if (display_position == 1)
  {
    for (int i = 6; i > -1; i--)
    {
      if (digits[curr_value][i] != digits[prev_value][i])
      {
        if (i == 6)
        {
          pwmH.setPWM(2, 0, processAngle(segmentHOff[2]));
          pwmH.setPWM(4, 0, processAngle(segmentHOff[4]));
          delay(70);
        }

        if (digits[curr_value][i] == 0)
        {
          pwmH.setPWM(i, 0, processAngle(segmentHOff[i]));
        }
      
        else
        {
          pwmH.setPWM(i, 0, processAngle(segmentHOn[i]));
        }
      }
    }
  }

  else if (display_position == 2)
  {
    for (int i = 6; i > -1; i--)
    {
      if (digits[curr_value][i] != digits[prev_value][i])
      {
        if (i == 6)
        {
          pwmM.setPWM(2, 0, processAngle(segmentMOff[2]));
          pwmM.setPWM(4, 0, processAngle(segmentMOff[4]));
          delay(70);
        }

        if (digits[curr_value][i] == 0)
        {
          pwmM.setPWM(i, 0, processAngle(segmentMOff[i]));
        }
      
        else
        {
          pwmM.setPWM(i, 0, processAngle(segmentMOn[i]));
        }
      }
    }
  }

  else if (display_position == 3)
  {
    for (int i = 13; i > 6; i--)
    {
      if (digits[curr_value][i - 7] != digits[prev_value][i - 7])
      {
        if (i == 13)
        {
          pwmM.setPWM(9, 0, processAngle(segmentMOff[9]));
          pwmM.setPWM(11, 0, processAngle(segmentMOff[11]));
          delay(70);
        }

        if (digits[curr_value][i - 7] == 0)
        {
          pwmM.setPWM(i, 0, processAngle(segmentMOff[i]));
        }
      
        else
        {
          pwmM.setPWM(i, 0, processAngle(segmentMOn[i]));
        }
      }
    }
  }
  */
  if (display_position == 0)
  {
    if (digits[curr_value][6] != digits[prev_value][6])
      {
        pwmA.setPWM(9, 0, processAngle(segmentAOff[9]));
        pwmA.setPWM(11, 0, processAngle(segmentAOff[11]));
        delay(80);
        if (digits[curr_value][6] == 0)
        {
          pwmA.setPWM(13, 0, processAngle(segmentAOff[13]));
        }
        
        else
        {
          pwmA.setPWM(13, 0, processAngle(segmentAOn[13]));
        }
      }
 
    for (short i = 12; i > 6; i--)
    {
      if (digits[curr_value][i - 7] == 0)
      {
        pwmA.setPWM(i, 0, processAngle(segmentAOff[i]));
      }
      
      else
      {
        pwmA.setPWM(i, 0, processAngle(segmentAOn[i]));
      }
    }
  }
  
  else if (display_position == 1)
  {
    if (digits[curr_value][6] != digits[prev_value][6])
      {
        pwmA.setPWM(2, 0, processAngle(segmentAOff[2]));
        pwmA.setPWM(4, 0, processAngle(segmentAOff[4]));
      delay(80);
        if (digits[curr_value][6] == 0)
        {
          pwmA.setPWM(6, 0, processAngle(segmentAOff[6]));
        }
        
        else
        {
          pwmA.setPWM(6, 0, processAngle(segmentAOn[6]));
        }
      }
      
    for (short i = 5; i > -1; i--)
    {
      if (digits[curr_value][i] == 0)
      {
        pwmA.setPWM(i, 0, processAngle(segmentAOff[i]));
      }
      
      else
      {
        pwmA.setPWM(i, 0, processAngle(segmentAOn[i]));
      }
    }
  }
  
  else if (display_position == 2)
  {
    if (digits[curr_value][6] != digits[prev_value][6])
      {
        pwmB.setPWM(2, 0, processAngle(segmentBOff[2]));
        pwmB.setPWM(4, 0, processAngle(segmentBOff[4]));
    delay(80);
        if (digits[curr_value][6] == 0)
        {
          pwmB.setPWM(6, 0, processAngle(segmentBOff[6]));
        }
        
        else
        {
          pwmB.setPWM(6, 0, processAngle(segmentBOn[6]));
        }
      }
      
    for (short i = 5; i > -1; i--)
    {
      if (digits[curr_value][i] == 0)
      {
        pwmB.setPWM(i, 0, processAngle(segmentBOff[i]));
      }
      
      else
      {
        pwmB.setPWM(i, 0, processAngle(segmentBOn[i]));
      }
    }
  }
  
  else if (display_position == 3)
  {
    if (digits[curr_value][6] != digits[prev_value][6])
      {
        pwmB.setPWM(9, 0, processAngle(segmentBOff[2]));
        pwmB.setPWM(11, 0, processAngle(segmentBOff[4]));
    delay(80);
        if (digits[curr_value][6] == 0)
        {
          pwmB.setPWM(13, 0, processAngle(segmentBOff[13]));
        }
        
        else
        {
          pwmB.setPWM(13, 0, processAngle(segmentBOn[13]));
        }
      }
      
    for (short i = 12; i > 6; i--)
    {
      if (digits[curr_value][i - 7] == 0)
      {
        pwmB.setPWM(i, 0, processAngle(segmentBOff[i]));
      }
      
      else
      {
        pwmB.setPWM(i, 0, processAngle(segmentBOn[i]));
      }
    }
  }
}

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}
