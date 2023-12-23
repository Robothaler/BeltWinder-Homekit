#include <Arduino.h>
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <app/clusters/window-covering-server/window-covering-server.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include "Config.h"

//Define debug level
#define DEBUGLEVEL 0
#include <DebugUtils.h>

#include <esp_log.h>
#include <esp_err.h>

// Custom keys for your data
const char * kMaxCountKey = "MaxCount";

using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace esp_matter::attribute;
using namespace esp_matter::cluster;

using namespace chip::app::Clusters::WindowCovering;

// Cluster and attribute ID used by Matter WindowCovering Device
const uint32_t WINDOW_COVERING_CLUSTER_ID = WindowCovering::Id;
const uint32_t WINDOW_COVERING_ATTRIBUTE_ID_TARGET = WindowCovering::Attributes::TargetPositionLiftPercent100ths::Id;
const uint32_t WINDOW_COVERING_ATTRIBUTE_ID_CURRENT = WindowCovering::Attributes::CurrentPositionLiftPercent100ths::Id;
const uint32_t WINDOW_COVERING_ATTRIBUTE_ID_OPERATE = WindowCovering::Attributes::OperationalStatus::Id;
const uint32_t WINDOW_COVERING_ATTRIBUTE_ID_ENDTYPE = WindowCovering::Attributes::EndProductType::Id;
const uint32_t WINDOW_COVERING_ATTRIBUTE_ID_TYPE = WindowCovering::Attributes::Type::Id;
const uint32_t WINDOW_COVERING_ATTRIBUTE_ID_CONFIG = WindowCovering::Attributes::ConfigStatus::Id;
const uint32_t WINDOW_COVERING_ATTRIBUTE_ID_MODE = WindowCovering::Attributes::Mode::Id;
const uint32_t WINDOW_COVERING_ATTRIBUTE_ID_SAFETY = WindowCovering::Attributes::SafetyStatus::Id;
// Cluster for CalibrationSwitch
const uint32_t CLUSTER_ID = OnOff::Id;
const uint32_t ATTRIBUTE_ID = OnOff::Attributes::OnOff::Id;

uint16_t window_covering_endpoint_id = 0;
uint16_t switch_endpoint_id_1 = 0;
attribute_t *attribute_ref_target;
attribute_t *attribute_ref_1;

static const char TAG[] = "WindowCovering-App";


//Function prototypes
void setupPins();
void loadMaxCount();
void up();
void down();
void setMaxCount();
void newPosition(int newPercentage);
void countPosition();
void calibrationRun();
void setCurrentDirection();
void sendCurrentPosition();
void handlePosCertainty();
void moveToNewPos();
void handleCalibration();
void set_window_covering_position(esp_matter_attr_val_t *current);
void set_onoff_attribute_value(esp_matter_attr_val_t *onoff_value, uint16_t plugin_unit_endpoint_id_1);
void set_operational_state(chip::EndpointId endpoint, OperationalState newState);


/*********************************************************************************
 * * * MATTER FUNCTIONS
 *********************************************************************************/

static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}

static esp_err_t on_identification(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
  return ESP_OK;
}


// Sets the value of the WindowCovering attribute for the target position from the app
// and reads the On/Off attribute value Calibration mode ON/OFF
static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                     uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
  if (type == attribute::PRE_UPDATE) {
    if (endpoint_id == window_covering_endpoint_id && cluster_id == WINDOW_COVERING_CLUSTER_ID && attribute_id == WINDOW_COVERING_ATTRIBUTE_ID_TARGET) {
      // Extract new target position from the attribute value
      int new_target_position = val->val.i;

      // Conversion from the transfer value 10000 to 100% percent
      int new_position_in_counts = new_target_position / 100;

      // Transfer of the new position value to the existing function
      newPosition(new_position_in_counts);

      DEBUGPRINTLN2("[BELTWINDER] newPosition function called with new target position: " + String(new_position_in_counts));
    } else if (cluster_id == CLUSTER_ID && attribute_id == ATTRIBUTE_ID) {
      // We got a Calibration Switch on/off attribute update!
      calButton = val->val.b;

      if (calButton) {
        DEBUGPRINTLN2("[BELTWINDER] Calibration On attribute value read successfully");
        calibrationRun();

      } else {
        // If the calibration switch is switched off (OFF), output a debug message
        DEBUGPRINTLN2("[BELTWINDER] Calibration Off attribute value read successfully");
        calButton = false;
        calMode = false;
        calUp = false;
      }
    }
  }
  return ESP_OK;
}

// Reads calibration switch on/off attribute value
esp_matter_attr_val_t get_onoff_attribute_value() {
  esp_matter_attr_val_t onoff_value = esp_matter_invalid(NULL);
  attribute::get_val(attribute_ref_1, &onoff_value);
  return onoff_value;
}

// Sets the value of the WindowCovering attribute for the current position (both target and current attributes must be sent!)
void set_window_covering_position(esp_matter_attr_val_t *current)
{    
    attribute::update(window_covering_endpoint_id, WINDOW_COVERING_CLUSTER_ID, WINDOW_COVERING_ATTRIBUTE_ID_CURRENT, current);
    attribute::update(window_covering_endpoint_id, WINDOW_COVERING_CLUSTER_ID, WINDOW_COVERING_ATTRIBUTE_ID_TARGET, current);
}

// Sends the on / off command to the app
void set_onoff_attribute_value(esp_matter_attr_val_t* onoff_value) {
  attribute::update(switch_endpoint_id_1, CLUSTER_ID, ATTRIBUTE_ID, onoff_value);
}

void set_operational_state(chip::EndpointId endpoint, OperationalState newState)
{
    esp_matter_attr_val_t stateValue;
    stateValue = esp_matter_int8(static_cast<int8_t>(newState));

    esp_err_t result = attribute::update(window_covering_endpoint_id, WINDOW_COVERING_CLUSTER_ID, WINDOW_COVERING_ATTRIBUTE_ID_OPERATE, &stateValue);

    if (result == ESP_OK)
    {
        DEBUGPRINTLN2("[BELTWINDER] Operational State updated successfully");
    }
    else
    {
        DEBUGPRINTLN2("[BELTWINDER] Failed to update Operational State");
        // Handle error if needed
    }
}



/*********************************************************************************
 * * * MATTER SETUP FUNCTIONS
 *********************************************************************************/

void set_window_covering_mode(chip::EndpointId endpoint, chip::app::Clusters::WindowCovering::Mode newMode)
{
    esp_matter_attr_val_t modeValue;
    modeValue = esp_matter_int8(static_cast<int8_t>(newMode));

    esp_err_t result = attribute::update(window_covering_endpoint_id, WINDOW_COVERING_CLUSTER_ID, WINDOW_COVERING_ATTRIBUTE_ID_MODE, &modeValue);

    if (result == ESP_OK)
    {
        DEBUGPRINTLN2("[BELTWINDER] Window covering Mode updated successfully");
        chip::DeviceLayer::PlatformMgr().LockChipStack();
        DEBUGPRINTLN2("[BELTWINDER] Current Mode: "); ModePrint(ModeGet(window_covering_endpoint_id));
        chip::DeviceLayer::PlatformMgr().UnlockChipStack();
    }
    else
    {
        DEBUGPRINTLN2("[BELTWINDER] Failed to update window covering Mode");
        // Handle error if needed
    }
}


void set_window_covering_configstatus(chip::EndpointId endpoint, ConfigStatus newConfigStatus)
{
    esp_matter_attr_val_t ConfigStatusValue;
    ConfigStatusValue = esp_matter_int8(static_cast<int8_t>(newConfigStatus));

    esp_err_t result = attribute::update(window_covering_endpoint_id, WINDOW_COVERING_CLUSTER_ID, WINDOW_COVERING_ATTRIBUTE_ID_CONFIG, &ConfigStatusValue);

    if (result == ESP_OK)
    {
        DEBUGPRINTLN2("[BELTWINDER] Window covering ConfigStatus updated successfully");

        // Handle Type attribute change
        chip::DeviceLayer::PlatformMgr().LockChipStack();
        ConfigStatusSet(endpoint, static_cast<ConfigStatus>(newConfigStatus));
        chip::DeviceLayer::PlatformMgr().UnlockChipStack();
    }
    else
    {
        DEBUGPRINTLN2("[BELTWINDER] Failed to update window covering ConfigStatus");
        // Handle error if needed
    }
}


void set_window_covering_type(chip::EndpointId endpoint, Type newType)
{
    esp_matter_attr_val_t typeValue;
    typeValue = esp_matter_int8(static_cast<int8_t>(newType));

    esp_err_t result = attribute::update(window_covering_endpoint_id, WINDOW_COVERING_CLUSTER_ID, WINDOW_COVERING_ATTRIBUTE_ID_TYPE, &typeValue);

    if (result == ESP_OK)
    {
        DEBUGPRINTLN2("[BELTWINDER] Window covering Type updated successfully");

        // Handle Type attribute change
        chip::DeviceLayer::PlatformMgr().LockChipStack();
        TypeSet(endpoint, newType);
        chip::DeviceLayer::PlatformMgr().UnlockChipStack();
    }
    else
    {
        DEBUGPRINTLN2("[BELTWINDER] Failed to update window covering Type");
        // Handle error if needed
    }
}


void set_window_covering_endtype(chip::EndpointId endpoint, EndProductType newEndType)
{
    esp_matter_attr_val_t endTypeValue;
    endTypeValue = esp_matter_int8(static_cast<int8_t>(newEndType));

    esp_err_t result = attribute::update(window_covering_endpoint_id, WINDOW_COVERING_CLUSTER_ID, WINDOW_COVERING_ATTRIBUTE_ID_ENDTYPE, &endTypeValue);

    if (result == ESP_OK)
    {
        DEBUGPRINTLN2("[BELTWINDER] Window covering Endtype updated successfully");

        // Handle Type attribute change
        chip::DeviceLayer::PlatformMgr().LockChipStack();
        EndProductTypeSet(endpoint, static_cast<EndProductType>(newEndType));
        chip::DeviceLayer::PlatformMgr().UnlockChipStack();
    }
    else
    {
        DEBUGPRINTLN2("[BELTWINDER] Failed to update window covering Endtype");
        // Handle error if needed
    }
}

void set_window_covering_safetystatus(chip::EndpointId endpoint, SafetyStatus newSafetyStatus)
{
    esp_matter_attr_val_t safetyStatusValue;
    safetyStatusValue = esp_matter_int8(static_cast<int8_t>(newSafetyStatus));

    esp_err_t result = attribute::update(window_covering_endpoint_id, WINDOW_COVERING_CLUSTER_ID, WINDOW_COVERING_ATTRIBUTE_ID_SAFETY, &safetyStatusValue);

    if (result == ESP_OK)
    {
        DEBUGPRINTLN2("[BELTWINDER] Window covering SafetyStatus updated successfully");

        // Handle Type attribute change
        chip::DeviceLayer::PlatformMgr().LockChipStack();
        SafetyStatusSet(endpoint, static_cast<SafetyStatus>(newSafetyStatus));
        chip::DeviceLayer::PlatformMgr().UnlockChipStack();
    }
    else
    {
        DEBUGPRINTLN2("[BELTWINDER] Failed to update window covering SafetyStatus");
        // Handle error if needed
    }
}


/*********************************************************************************
 * * * SETUP
 *********************************************************************************/
void setup() {
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_NONE);

  DEBUGPRINTLN1("[BELTWINDER] Start");

  setupPins();

  //read stored maxCount value from the memory
  loadMaxCount();

  // Create Matter node
  node::config_t node_config;
  on_off_light::config_t light_config;
  node_t *node = node::create(&node_config, on_attribute_update, on_identification);

  // Create Window Covering device
  window_covering_device::config_t window_config;
  endpoint_t *endpoint = window_covering_device::create(node, &window_config, ENDPOINT_FLAG_NONE, NULL);
  endpoint_t *plugin_unit_endpoint_id_1 = on_off_light::create(node, &light_config, ENDPOINT_FLAG_NONE, NULL);

  // Get attribute reference
  attribute_ref_target = attribute::get(cluster::get(endpoint, WINDOW_COVERING_CLUSTER_ID), WINDOW_COVERING_ATTRIBUTE_ID_TARGET);
  attribute_ref_1 = attribute::get(cluster::get(endpoint, CLUSTER_ID), ATTRIBUTE_ID);

  // Get endpoint ID
  window_covering_endpoint_id = endpoint::get_id(endpoint);

  // Configure Window Covering cluster features
  cluster_t *cluster = cluster::get(endpoint, WindowCovering::Id);
  cluster::window_covering::feature::lift::config_t lift;
  cluster::window_covering::feature::position_aware_lift::config_t position_aware_lift;
  nullable<uint8_t> percentage = nullable<uint8_t>(0);
    nullable<uint16_t> percentage_100ths = nullable<uint16_t>(0);
    //position_aware_lift.current_position_lift_percentage = percentage;
    position_aware_lift.target_position_lift_percent_100ths = percentage_100ths;
    position_aware_lift.current_position_lift_percent_100ths = percentage_100ths;
  cluster::window_covering::feature::lift::add(cluster, &lift);
  cluster::window_covering::feature::position_aware_lift::add(cluster, &position_aware_lift);

  // Setup DAC (this is good place to also set custom commission data, passcodes etc.)
  //esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());
   
  // Start Matter
  esp_matter::start(on_device_event);

  // Attribute: Id  0 Type
  //set_window_covering_type(window_covering_endpoint_id, Type::kShutter);

  // Attribute: Id  7 ConfigStatus
  //set_window_covering_configstatus(window_covering_endpoint_id, ConfigStatus::kLiftEncoderControlled);

  // Attribute: Id 13 EndProductType
  set_window_covering_endtype(window_covering_endpoint_id, EndProductType::kRollerShutter);

  // Attribute: Id 23 Mode
  set_window_covering_mode(window_covering_endpoint_id, Mode::kMotorDirectionReversed);
  //set_window_covering_mode(window_covering_endpoint_id, Mode::kCalibrationMode);
  //set_window_covering_mode(window_covering_endpoint_id, Mode::kMaintenanceMode);
  //set_window_covering_mode(window_covering_endpoint_id, Mode::kLedFeedback);

  // Attribute: Id 26 SafetyStatus (Optional)
  chip::BitFlags<SafetyStatus> safetyStatus(0x00); // 0 is no issues;
  //set_window_covering_safetystatus(window_covering_endpoint_id, SafetyStatus::kPositionFailure);

  // Print onboarding codes
  PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

  // -- So, alles erledigt... kann los gehen
  DEBUGPRINTLN1("[BELTWINDER] Initialization completed.");
}


/*********************************************************************************
 * * * LOOP
 *********************************************************************************/
void loop() {

  //Count current position
  countPosition();

  //Determine current direction
  setCurrentDirection();

  //Set whether current position is unique
  handlePosCertainty();

  //Send current position to App
  sendCurrentPosition();

  //Move to new position
  moveToNewPos();

  //Execute calibration run
  handleCalibration();

}


/*********************************************************************************
 * * * GURTWICKLER PROGRAMM
 *********************************************************************************/

// Function for up
void up() {
  DEBUGPRINTLN2("[BELTWINDER] Entering up() function.");

  if (dir != -1) {
    DEBUGPRINTLN2("[BELTWINDER] Direction is not -1.");

    if (!calMode && !posCertain) {
      DEBUGPRINTLN2("[BELTWINDER] Not in calibration mode and position is not certain.");

      posRunUp = true;
    }

    // Simulated button press "up"
    DEBUGPRINTLN2("[BELTWINDER] Simulating button press for 'up'.");
    digitalWrite(BUTTON_UP, LOW);
    delay(300);
    digitalWrite(BUTTON_UP, HIGH);
    delay(500);
  } else {
    DEBUGPRINTLN2("[BELTWINDER] Direction is -1, 'up()' function skipped.");
  }

  DEBUGPRINTLN2("[BELTWINDER] Exiting up() function.");
}


// Function for down
void down() {
  DEBUGPRINTLN2("[BELTWINDER] Entering down() function.");

  if (dir != 1) {
    DEBUGPRINTLN2("[BELTWINDER] Direction is not 1.");

    if (!calMode && !posCertain) {
      DEBUGPRINTLN2("[BELTWINDER] Not in calibration mode and position is not certain.");

      posRunDown = true;
    }

    // Simulated button press "down"
    DEBUGPRINTLN2("[BELTWINDER] Simulating button press for 'down'.");
    digitalWrite(BUTTON_DOWN, LOW);
    delay(300);
    digitalWrite(BUTTON_DOWN, HIGH);
    delay(500);
  } else {
    DEBUGPRINTLN2("[BELTWINDER] Direction is 1, 'down()' function skipped.");
  }

  DEBUGPRINTLN2("[BELTWINDER] Exiting down() function.");
}

// Saving the lower position
void setMaxCount()
{
    if (count > 0)
    {
        // Save maxCount value
        CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kMaxCountKey, &count, sizeof(count));
        if (err == CHIP_NO_ERROR)
        {
            maxCount = count;

            posCertain = true;
            maxCountInit = true;
            DEBUGPRINTLN2("New maximum count set: " + String(maxCount) + "]");
            posChange = true;
            sendCurrentPosition();

            // Calibration completed, send "false" to the app
            esp_matter_attr_val_t onoff_value = get_onoff_attribute_value();
            onoff_value.val.b = !onoff_value.val.b;
            set_onoff_attribute_value(&onoff_value);
        }
        else
        {
            
        }
    }
    else
    {
        DEBUGPRINTLN2("Maximum count incorrect: " + String(maxCount) + "]");
    }
}


void countPosition(){
  if (digitalRead(PULSECOUNTER) == LOW && digitalRead(MOTOR_UP) == HIGH && dir == 1 && !counted) {
    count++;
    DEBUGPRINTLN2("[BELTWINDER] new count (MOTOR_DOWN): " + String(count));
    if(!calMode && count > maxCount){
      count = maxCount;
    }
    counted = true; //sure that counting only takes place once per LOW pulse
    posChange = true; //Detect that the position has changed
  } 
  else if (digitalRead(PULSECOUNTER) == LOW && digitalRead(MOTOR_DOWN) == HIGH && dir == -1 && !counted) {
    if (count > 0){
      count--;
    }
    DEBUGPRINTLN2("[BELTWINDER] new count (MOTOR_UP): " + String(count));
    counted = true; //sure that counting only takes place once per LOW pulse
    posChange = true; //Detect that the position has changed
  } else if (digitalRead(PULSECOUNTER) == HIGH && counted == true) {
    counted = false;
  }
}


void sendCurrentPosition()
{
    //Send value only if changed and standstill
    if (!posChange || dir != 0)
    {
        return;
    }

    if (maxCountInit)
    {
        //Percentage value
        // -- Calculate percentage values
        int percentage = count / (maxCount * 0.01);
        lastSendPercentage = percentage;

        DEBUGPRINTLN2("[BELTWINDER] sendCurrentPosition: Send current position to the app: " + String(percentage) + "%");

        // Update the attribute with the current percentage value
        int16_t current_position = percentage * 100;
        esp_matter_attr_val_t current = esp_matter_int16(current_position);

        // Call the function to set the WindowCovering attribute for the position
        set_window_covering_position(&current);

        lastSendPercentageInit = true;
        DEBUGPRINTLN2("[BELTWINDER] sendCurrentPosition: Last sent percentage initialized.");
    }
    posChange = false;
    DEBUGPRINTLN2("[BELTWINDER] sendCurrentPosition: PosChange reset.");
}


// Transmission of the current (changed) direction
void setCurrentDirection() {
  if (digitalRead(MOTOR_DOWN) == LOW && dir != 1) {
    dir = 1;
    set_operational_state(window_covering_endpoint_id, OperationalState::MovingDownOrClose);
    DEBUGPRINTLN2("[BELTWINDER] Direction (DOWN) updated: Close");
  } else if (digitalRead(MOTOR_UP) == LOW && dir != -1) {
    dir = -1;
    set_operational_state(window_covering_endpoint_id, OperationalState::MovingUpOrOpen);

    DEBUGPRINTLN2("[BELTWINDER] Direction (UP) updated: Open");
  } else if (digitalRead(MOTOR_DOWN) == HIGH && digitalRead(MOTOR_UP) == HIGH && dir != 0) {
    dir = 0;
    delay(1000);
    set_operational_state(window_covering_endpoint_id, OperationalState::Stall);
    DEBUGPRINTLN2("[BELTWINDER] Direction updated: Inactive");
  }
}


void newPosition(int newPercentage) {
    unsigned long currentTime = millis();

    bufferedPercentage = newPercentage;
    DEBUGPRINTLN2("[BELTWINDER] newPosition: New percentage received - " + String(newPercentage) + "%");

    if (lastUpdateTime == 0) {
        lastUpdateTime = currentTime;
        DEBUGPRINTLN2("[BELTWINDER] lastUpdateTime: New timestamp - " + String(lastUpdateTime) + "ms");
    }

    // Buffer the value when enough time has passed
    if (currentTime - lastUpdateTime < filterDuration) {
      DEBUGPRINTLN2("[BELTWINDER] currentTime: New timestamp - " + String(currentTime) + "ms");
        bufferedPercentage = newPercentage;
         DEBUGPRINTLN2("[BELTWINDER] Buffering the value during the buffer time - Value: " + String(bufferedPercentage) + ", Time: " + String(currentTime - lastUpdateTime) + " ms");
         return;
    } else {
        // Time has expired, use the last buffered value
        DEBUGPRINTLN2("[BELTWINDER] Time limit for buffering exceeded. Use last buffered value.");
        filteredPercentage = bufferedPercentage;
        
        DEBUGPRINTLN2("[BELTWINDER] newPosition: Filtered percentage - " + String(filteredPercentage));

        lastUpdateTime = 0; // Set lastUpdateTime to 0 for the next update
    }

    // Filter for own publishes
    if (lastSendPercentageInit && lastSendPercentage == filteredPercentage) {
        return;
    }

    // Ignores inputs if calibration is taking place, there is no maxCount yet, or a new position is currently being approached
    if (calMode || !maxCountInit || remote) {
        return;
    }

    // Sanity check
    if (filteredPercentage < 0 || filteredPercentage > 100) {
        DEBUGPRINTLN2("[BELTWINDER] newPosition: Invalid percentage. Action is skipped.");
        return;
    }

    // Set new target value
    newCount = int((float(filteredPercentage) / 100 * maxCount) + 0.5);
    newCountInit = true;

    DEBUGPRINTLN2("[BELTWINDER] newPosition: New Count value - " + String(newCount));

    if (newCount == maxCount) {
        down();
        DEBUGPRINTLN2("[BELTWINDER] newPosition: New target value corresponds to the maximum counter value. Action - down()");
    } else if (newCount == 0) {
        up();
        DEBUGPRINTLN2("[BELTWINDER] newPosition: New target value is 0. action - up()");
    } else {
        // Checks whether an end stop has been approached since the boat
        if (!posCertain) {
            // Shortest distance to the new point with stop at end position
            // (maxCount-Count):Distance downwards
            // (maxCount-NewCount):Distance from the bottom to the new position
            // count: Distance upwards
            // newCount: Distance from the top to the new position
            if ((2 * maxCount - count - newCount) >= count + newCount) {
                up();
                DEBUGPRINTLN2("[BELTWINDER] newPosition: Shortest distance to the new position - up()");
            } else {
                down();
                DEBUGPRINTLN2("[BELTWINDER] newPosition: Shortest distance to the new position - down()");
            }
        }
        remote = true;
        DEBUGPRINTLN2("[BELTWINDER] newPosition: Remote control activated.");
    }
}



//Moves to new position value
void moveToNewPos() {
  //Will not be executed if a position run is in progress and a new position has been defined
  if(!remote || posRunUp || posRunDown){
    return;
  }
  //Secures that a value has been received
  if(!newCountInit){
    remote = false;
    return;
  }
  if(newCount > count){
    //if new value is greater than current value and roller shutter is currently stopped, then start
    if(dir == 0){
      down(); // --start
      DEBUGPRINTLN2("[BELTWINDER] New percentage value differs, drive down");
    }
    //if current value is less than new value and is currently being moved upwards, then stop
    else if(dir == -1){
      down(); // -- stope now
      DEBUGPRINTLN2("[BELTWINDER] New percentage value is equal, stop now");
      remote = false;
    }
  }
  else if(newCount < count){
    // -- if new value is less than current value and roller shutter is currently stopped, then start
    if(dir == 0){
      up(); // -- start
      DEBUGPRINTLN2("[BELTWINDER] New percentage value differs, drive up");
    }
    // -- if the current value is greater than the new value and is currently being lowered, then stop
    else if(dir == 1){
      up(); // -- stop now
      DEBUGPRINTLN2("[BELTWINDER] New percentage value is equal, stop now");
      remote = false;
    }
  }
  else{
    remote = false;
    if(dir ==1){
      up();  //stop
      DEBUGPRINTLN2("[BELTWINDER] Stop newCount = count");
    }
    else if(dir ==-1){
      down();  //stop
      DEBUGPRINTLN2("[BELTWINDER] Stop newCount = count");
    }
  }
}


//Initialization run
void calibrationRun(){
  calMode = true;
  calUp = true;
  DEBUGPRINTLN2("[BELTWINDER] Start calibration: Roller shutter moves all the way up, then all the way down");
  up();
}


/*
*Calibration in the loop
*1. calUp - moves all the way up and zeros counter
*2. calDown - moves all the way down and saves new maximum
*Must be executed after setDir and started by calibrationRun.
*/
void handleCalibration(){
  //set_window_covering_mode(window_covering_endpoint_id, Mode::kCalibrationMode);
  if(calMode){
    if(dir==0 && calUp){
      delay(1000);
      count = 0;
      calUp = false;
      calDown = true;
      down();
      delay(1000);
    }
    else if(dir==0 && calDown){
      calDown = false;
      calMode = false;
      posRunUp = false;
      posRunDown = false;
      setMaxCount();
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
    DEBUGPRINTLN2("[BELTWINDER] posCertain");
  }
  if(posRunDown){
    count = maxCount;
    posRunDown = false;
    posCertain = true;
    posChange = true;
    DEBUGPRINTLN2("[BELTWINDER] posCertain");
  } 
}

void setupPins(){
  // -- Define pins
  pinMode(PULSECOUNTER, INPUT);       // Pulse counter
  pinMode(MOTOR_UP, INPUT_PULLUP);    // Motor starts up
  pinMode(MOTOR_DOWN, INPUT_PULLUP);  // Motor moves down
  pinMode(BUTTON_DOWN, OUTPUT);       // Button for down
  digitalWrite(BUTTON_DOWN, HIGH);    // Set button to HIGH
  pinMode(BUTTON_UP, OUTPUT);         // Button for up
  digitalWrite(BUTTON_UP, HIGH);      // Set button to HIGH
}

/*********************************************************************************
 * * * MISCELLANEOUS
 *********************************************************************************/


void loadMaxCount()
{
    // Load lower position (maxCount) from the KeyValueStore
    size_t bytesRead;
    CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(kMaxCountKey, &maxCount, sizeof(maxCount), &bytesRead);

    if (err == CHIP_ERROR_KEY_NOT_FOUND)
    {
        // The value was not found, set it to the default value (e.g. 0)
        maxCount = 0;

        // Save the default value in the KeyValueStore
        err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kMaxCountKey, &maxCount, sizeof(maxCount));
        if (err != CHIP_NO_ERROR)
        {
            DEBUGPRINTLN2("Error when saving the value: " + String(err.Format()));
        }
    }
    else if (err != CHIP_NO_ERROR)
    {
        DEBUGPRINTLN2("Error loading the value: " + String(err.Format()));
    }

    DEBUGPRINTLN2("Loaded config 'maxcount' = " + String(maxCount));
}
