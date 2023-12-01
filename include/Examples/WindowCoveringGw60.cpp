#include <Arduino.h>
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;

#include "FS.h"
#include <EEPROM.h>
//Define debug level
#define DEBUGLEVEL 3
#include <DebugUtils.h>

// -- Configuration version. This value should be changed if something has changed in the configuration structure.
#define CONFIG_VERSION "BW-M_v0.1"

#define PULSECOUNTER    18    // Pulse counter
#define MOTOR_UP	      17    // Motor drives up
#define MOTOR_DOWN      16    // Motor drives down
#define BUTTON_UP       21    // Button for up
#define BUTTON_DOWN     22    // Button for runter

// Cluster and attribute ID used by Matter window covering device
const uint32_t CLUSTER_ID = WindowCovering::Id;
const uint32_t ATTRIBUTE_ID = WindowCovering::Attributes::CurrentPositionLiftPercentage::Id;

// Endpoint and attribute ref that will be assigned to Matter device
uint16_t window_covering_id = 0;
attribute_t *attribute_ref;

// -- Callback declarations
void up();
void down();
bool isNumeric(String str);
void setMaxCount();
void newPosition(int newPercentage);
void calibrationRun();
char * addIntToChar(char* text, int i);

// -- Variables
int dir = 0, count = 0, maxCount, newCount = 0, lastSendPercentage = 0;
float percentage = 0;
bool remote = false, needReset = false, posCertain = false, calMode = false, calUp = false, calDown = false, lastSendPercentageInit = false, newCountInit = false, maxCountInit = false, countRetrived = false, counted = false, posChange = false,posRunUp = false, posRunDown = false;
byte value;

static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}
static esp_err_t on_identification(identification::callback_type_t type, uint16_t endpoint_id,
                                   uint8_t effect_id, void *priv_data) {
  return ESP_OK;
}

// Listener on attribute update requests.
// In this example, when update is requested, path (endpoint, cluster and attribute) is checked
// if it matches Window covering attribute.
static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                     uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
  if (type == attribute::PRE_UPDATE && endpoint_id == window_covering_id &&
      cluster_id == CLUSTER_ID && attribute_id == ATTRIBUTE_ID)  {
    // We got an Window Covering attribute update!
    boolean new_state = val->val.b;
    percentage = new_state;
    DEBUGPRINT3("Update on endpoint: ");
    DEBUGPRINT3(endpoint_id);
    DEBUGPRINT3(" cluster: ");
    DEBUGPRINT3(cluster_id);
    DEBUGPRINT3(" attribute: ");
    DEBUGPRINTLN3(attribute_id);
  }
  return ESP_OK;
}

void print_endpoint_info(String clusterName, endpoint_t *endpoint) {
  uint16_t endpoint_id = endpoint::get_id(endpoint);
  DEBUGPRINT3(clusterName);
  DEBUGPRINT3(" has endpoint: ");
  DEBUGPRINTLN3(endpoint_id);
}



/************************************************************************************
      Belt winder program
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


void setup() {
  Serial.begin(115200);
  // while the serial stream is not open, do nothing:
  // Only required for debugging
  if(DEBUGLEVEL >0){
    while (!Serial) ;
    delay(1000);
  }
  DEBUGPRINTLN1("Start");

  setupPins();

  //read stored maxCount value from memory
  loadMaxCount();

  esp_log_level_set("*", ESP_LOG_DEBUG);

    // Setup Matter node
  node::config_t node_config;
  node_t *node = node::create(&node_config, on_attribute_update, on_identification);

  endpoint_t *endpoint;
  cluster_t *cluster;

  // Setup Window covering endpoint / cluster / attributes with default values
  window_covering_device::config_t window_covering_device_config;
  window_covering_device_config.window_covering.current_position_lift_percentage = false;
  window_covering_device_config.window_covering.mode = false;
  
  

    // Save on/off attribute reference. It will be used to read attribute value later.
  attribute_ref = attribute::get(cluster::get(endpoint, CLUSTER_ID), ATTRIBUTE_ID);

  // Save generated endpoint id
  window_covering_id = endpoint::get_id(endpoint);

  // Start Matter device
  esp_matter::start(on_device_event);

  endpoint = window_covering_device::create(node, &window_covering_device_config, ENDPOINT_FLAG_NONE, NULL);
  cluster = cluster::get(endpoint, WindowCovering::Id);
  cluster::window_covering::config_t window_covering;
  cluster::window_covering::feature::lift::config_t lift;
  //cluster::window_covering::feature::tilt::config_t tilt;
  //cluster::window_covering::feature::position_aware_lift::config_t position_aware_lift;
  //cluster::window_covering::feature::position_aware_tilt::config_t position_aware_tilt;
  cluster::window_covering::feature::absolute_position::config_t absolute_position;
  cluster::window_covering::feature::lift::add(cluster, &lift);
  //cluster::window_covering::feature::tilt::add(cluster, &tilt);
  //cluster::window_covering::feature::position_aware_lift::add(cluster, &position_aware_lift);
  //cluster::window_covering::feature::position_aware_tilt::add(cluster, &position_aware_tilt);
  cluster::window_covering::feature::absolute_position::add(cluster, &absolute_position);
  print_endpoint_info("window_covering_device", endpoint);

  esp_matter::start(on_device_event);

  PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

    // -- So, all done... can go
  DEBUGPRINTLN2("Initialization completed");
}

  // Reads attribute values of Window covering
esp_matter_attr_val_t get_absolute_position_value() {
  esp_matter_attr_val_t absolute_position_value = esp_matter_invalid(NULL);
  attribute::get_val(attribute_ref, &absolute_position_value);
  return absolute_position_value;
}

// Sets Window covering to attribute values
void set_absolute_position_value(esp_matter_attr_val_t* absolute_position_value) {
  attribute::update(window_covering_id, CLUSTER_ID, ATTRIBUTE_ID, absolute_position_value);
}


void loop(){

  //Count current position
  countPosition();

  //Determine Current Direction
  setCurrentDirection();

  //Determine if current position is unique
  handlePosCertainty();

  //update current position
  sendCurrentPosition();

  //Approach new position
  moveToNewPos();

  //Perform calibration drive
  handleCalibration();
}