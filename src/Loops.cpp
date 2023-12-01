#include <Arduino.h>                // Arduino framework
#include "Config.h"
#include "FS.h"
#include <EEPROM.h>




// -- Variables
int dir = 0, count = 0, maxCount, newCount = 0, lastSendPercentage = 0;
float percentage = 0;
bool remote = false, needReset = false, posCertain = false, calMode = false, calUp = false, calDown = false, lastSendPercentageInit = false, newCountInit = false, maxCountInit = false, countRetrived = false, counted = false, posChange = false,posRunUp = false, posRunDown = false;
byte value;


/************************************************************************************
      SETUP
*************************************************************************************/

void setupPins()  // -- Pin declaration
{
  pinMode(PULSECOUNTER, INPUT);       // Pulse counter
  pinMode(MOTOR_UP, INPUT_PULLUP);    // Motor drives up
  pinMode(MOTOR_DOWN, INPUT_PULLUP);  // Motor drives down
  pinMode(BUTTON_UP, OUTPUT);         // Button for up
  digitalWrite(BUTTON_UP, HIGH);      // Set up button to HIGH
  pinMode(BUTTON_DOWN, OUTPUT);       // Button for down
  digitalWrite(BUTTON_DOWN, HIGH);    // Set down button to HIGH
}

/************************************************************************************
      Belt winder program
*************************************************************************************/

void up() // -- function shutter up
{
  if (dir != -1){
    if(!calMode && !posCertain){
      posRunUp = true;
      DEBUGPRINTLN2("Shutter drives up");
    }
    //"activate button -> direction up"
    digitalWrite(BUTTON_UP, LOW);
    delay(300);
    digitalWrite(BUTTON_UP, HIGH);
    delay(500);
  }   
}


void down()  //  -- function shutter down
{ 
  if (dir != 1){
    if(!calMode && !posCertain){
      posRunDown = true;
      DEBUGPRINTLN2("Shutter drives down");
    }
    // "activate button -> direction down"
    digitalWrite(BUTTON_DOWN, LOW);
    delay(300);
    digitalWrite(BUTTON_DOWN, HIGH);
    delay(500);
  }
}


void setMaxCount() //  --  store lowest/closed position
{
  if(count>0){
    // store count-value
    EEPROM.write(10, count);
    //store maxCount init-Flag
    EEPROM.write(11, 19);
    EEPROM.write(12, 92);
    EEPROM.write(13, 42);
    EEPROM.commit();
    maxCount = EEPROM.read (10);

    posCertain = true;
    maxCountInit = true;
    DEBUGPRINTLN2("new max. count-value stored:"+String(maxCount)+"]");
  }
  else{
    DEBUGPRINTLN2("max. count-value error:"+String(maxCount)+"]");
  }
}

void countPosition() //  --  count position-pulse
{
  //sensorGuard();
  if (digitalRead(PULSECOUNTER) == LOW && digitalRead(MOTOR_UP) == HIGH && dir == 1 && !counted) {
    count++;
    DEBUGPRINTLN3("new pulse-count: " + String(count));
    if(!calMode && count > maxCount){
      count = maxCount;
    }
    counted = true;   //make sure that counting is done only once per LOW pulse
    posChange = true; //Capture that the position has changed
    
  } 
  else if (digitalRead(PULSECOUNTER) == LOW && digitalRead(MOTOR_UP) == HIGH && dir == -1 && !counted) {
    if (count > 0){
      count--;
    }
    DEBUGPRINTLN3("new pulse-count: " + String(count));
    counted = true;   //make sure that counting is done only once per LOW pulse
    posChange = true; //Capture that the position has changed
  } else if (digitalRead(PULSECOUNTER) == HIGH && counted == true) {
    counted = false;
  }
}


void sendCurrentPosition()  //  --  update current position
{
  //Send value only when changed and movement stopped
  if(!posChange || dir != 0){
    return;
  }
  //count value
  char ccount[4];
  itoa( count, ccount, 10 );
  DEBUGPRINTLN3("sendCurrentPosition: ccount = true: " + String(ccount));
  if(maxCountInit){
    // Percentage value
    // -- Calculate percentage values
    int percentage = count / (maxCount * 0.01);
    lastSendPercentage = percentage;
    char cpercentage[5];
    itoa( percentage, cpercentage, 10 );
    DEBUGPRINTLN3("sendCurrentPosition: cpercentage = false: " + String(cpercentage));
    lastSendPercentageInit = true;
  }
  posChange = false;
}


void setCurrentDirection() //  --  current (changed) direction
{
  if (digitalRead(MOTOR_DOWN) == LOW && dir != 1) {
    dir = 1;
    DEBUGPRINTLN2("New status: Closing");
  } else if (digitalRead(MOTOR_UP) == LOW && dir != -1) {
    dir = -1;
    DEBUGPRINTLN2("New status: Opening");
  } else if (digitalRead(MOTOR_DOWN) == HIGH && digitalRead(MOTOR_UP) == HIGH && dir != 0) {
    dir = 0;
    DEBUGPRINTLN2("New status: Inactive");
    delay(1000);
  }
}

void newPosition(int newPercentage)  //  --  Logic for position requests in %.
{
    //Ignores inputs when calibration is in progress, there is no maxCount yet, or a new position is being approached.
    if(calMode || !maxCountInit || remote){
      return;
      DEBUGPRINTLN3("calMode is active, no maxCount is set or a now position is approched");
    }
    //Sanity check
    if(newPercentage<0 || newPercentage>100){
      return;
      DEBUGPRINTLN3("Sanity check: newPercentage error");
    }
    //set new target value
    newCount = int ((float (newPercentage)/100*maxCount)+0.5);
    newCountInit = true;
    if(newCount==maxCount){
      down();
      DEBUGPRINTLN2("New percentage value is different, drive down");
    }
    else if(newCount==0){
      up();
      DEBUGPRINTLN2("New percentage value is different, drive up");
    }
    else{
      //Checks whether an end stop has been approached since boot
      if(!posCertain){
        //shortest distance to new point with stop at end position
        //(maxCount-Count):Distance down
        //(maxCount-NewCount):Distance from thebottom to new position
        //count: Distance up
        //newCount: Distance from the top to new position
        if((2*maxCount-count-newCount) >= count+newCount){
          up();
          DEBUGPRINTLN2("Use the shortest route, drive up");
        }
        else{
          down();
          DEBUGPRINTLN2("Use the shortest route, drive down");
        }
      }
      remote = true;
    }
}

void moveToNewPos() //  --  Moves to new position value
{
  //Will not be executed if a position move is in progress and a new position has been defined.
  if(!remote || posRunUp || posRunDown){
    return;
    DEBUGPRINTLN3("remote is active, or a now position is approched");
  }
  //Ensures that a value has been received
  if(!newCountInit){
    remote = false;
    return;
  }
  if(newCount > count){
    //if new value is greater than current and shutter currently stopped, then start
    if(dir == 0){
      down(); // --start
      DEBUGPRINTLN2("New percentage value differs, drive down");
    }
    //if current value is smaller than new and is currently driven up, then stop
    else if(dir == -1){
      down(); // -- stopp
      DEBUGPRINTLN2("New percentage value is equal, stop now");
      remote = false;
    }
  }
  else if(newCount < count){
    // -- if new value is smaller than current and shutter currently stopped, then start
    if(dir == 0){
      up(); // -- start
      DEBUGPRINTLN2("New percentage value differs, drive up");
    }
    // -- if current value is greater than new and is currently being driven down, then stop
    else if(dir == 1){
      up(); // -- stopp
      DEBUGPRINTLN2("New percentage value is equal, stop now");
      remote = false;
    }
  }
  else{
    remote = false;
    if(dir ==1){
      up();  //stop
      DEBUGPRINTLN2("Stop newCount = count"+ String(count));
    }
    else if(dir ==-1){
      down();  //stop
      DEBUGPRINTLN2("Stop newCount = count"+ String(count));
    }
  }
}

void calibrationRun()  //  --  Initialization drive
{
  calMode = true;
  calUp = true;
  up();
  DEBUGPRINTLN2("Calibration started, Drive up");
}


/*
*Calibration in loop
*1. calUp -goes all the way to the top and zeros counters
*2. calDown -goes all the way down and saves new maximum
*Must be executed after setDir and started by calibrationRun
*/
void handleCalibration(){
  if(calMode){
    if(dir==0 && calUp){
      delay(1000);
      count = 0;
      calUp = false;
      calDown = true;
      down();
      delay(1000);
    }
    else if(dir==0 &&calDown){
      calDown = false;
      calMode = false;
      posRunUp = false;
      posRunDown = false;
      setMaxCount();
      DEBUGPRINTLN2("Set new MaxCount = maxCount"+ String(maxCount));
    }
  }
}

void handlePosCertainty(){
  if(calMode||!maxCountInit||dir!=0){
    return;
  }
  if(posRunUp){
    count = 0;
    posRunUp = false;
    posCertain = true;
    posChange = true;
    DEBUGPRINTLN2("posCertain");
  }
  if(posRunDown){
    count = maxCount;
    posRunDown = false;
    posCertain = true;
    posChange = true;
    DEBUGPRINTLN2("posCertain");
  } 
}

char * addIntToChar(char* text, int i){
  char * temp = new char [200];
  strcpy (temp,text);
  itoa(i, temp+strlen(temp), 10);
  return temp;
}

void loadMaxCount()  //  --  Load values from the EEPROM
{
    //Load lower position (maxCount) from EEPROM
  maxCount = EEPROM.read (10);
  //Sanity check
  if(maxCount>0){
    DEBUGPRINTLN1("Geladene Konfig 'maxcount'= "+String(maxCount));
    //Load checks from the EEPROM
    int check1 = EEPROM.read(11);
    int check2 = EEPROM.read(12);
    int check3 = EEPROM.read(13);
    if(check1 == 19 && check2 == 92 && check3 == 42){
      maxCountInit=true;
      DEBUGPRINTLN1("Loaded config 'maxcount initFlag'= "+String(maxCountInit));
      DEBUGPRINTLN3("Loaded config 'maxcount check1'= "+String(check1));
      DEBUGPRINTLN3("Loaded config 'maxcount check2'= "+String(check2));
      DEBUGPRINTLN3("Loaded config 'maxcount check3'= "+String(check3));
    }
  }
}