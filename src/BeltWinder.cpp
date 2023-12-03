#include <Arduino.h>
#include "Config.h"
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace esp_matter::attribute;
using namespace esp_matter::cluster;

#include <string>

#include <esp_log.h>
#include <esp_err.h>

// -- Callback declarations
void up();
void down();
bool isNumeric(String str);
void setMaxCount();
void newPosition(int newPercentage);
void calibrationRun();
char * addIntToChar(char* text, int i);

// Functions prototypes
void setupPins();
void sendCurrentPosition();
void setCurrentDirection();
void newPosition();
void moveToNewPos();
void handleCalibration();
void handlePosCertainty();
void loadMaxCount();
void countPosition();

// Cluster and attribute ID used by Matter WindowCovering device
const uint32_t CLUSTER_ID = WindowCovering::Id;
const uint32_t ATTRIBUTE_ID = WindowCovering::Attributes::Type::Id;

// Endpoint and attribute ref that will be assigned to Matter device
uint16_t windowcovering_endpoint_id = 0;
attribute_t *attribute_ref;

/**
   This program presents many Matter devices and should be used only
   for debug purposes (for example checking which devices types are supported
   in Matter controller).

   Keep in mind that it IS NOT POSSIBLE to run all those endpoints due to
   out of memory. There is need to manually comment out endpoints!
   Running about 4 endpoints at the same time should work.
*/

static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}
static esp_err_t on_identification(identification::callback_type_t type, uint16_t endpoint_id,
                                   uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
  return ESP_OK;
}

// Listener on attribute update requests.
// In this example, when update is requested, path (endpoint, cluster and
// attribute) is checked if it matches plugin unit attribute. If yes, LED changes
// state to new one.
static esp_err_t on_attribute_update(attribute::callback_type_t type,
                                     uint16_t endpoint_id, uint32_t cluster_id,
                                     uint32_t attribute_id,
                                     esp_matter_attr_val_t *val,
                                     void *priv_data) {
  if (type == attribute::PRE_UPDATE && cluster_id == CLUSTER_ID && attribute_id == ATTRIBUTE_ID) {
    // We got an windowcovering attribute update!
    bool newPercentage = val->val.b;
    if (endpoint_id == windowcovering_endpoint_id) {
      return newPercentage;
      ESP_LOGI(TAG, " newPercentage: %d", newPercentage);
  }
  }
  return ESP_OK;
}

void print_endpoint_info(String clusterName, endpoint_t *endpoint) {
  uint16_t endpoint_id = endpoint::get_id(endpoint);
  ESP_LOGI(TAG, " Clustername: %d", clusterName);
  ESP_LOGI(TAG, " has endpoint: %d", endpoint_id);
}

void setup() {
  Serial.begin(115200);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  // while the serial stream is not open, do nothing:
  // Only required for debugging
  if(DEBUGLEVEL >0){
    while (!Serial) ;
    delay(1000);
  }
  ESP_LOGD(TAG, "Start");


  setupPins();

  //read stored maxCount value from memory
  loadMaxCount();

/**
 * @brief This function creates a temperature sensor endpoint and associated cluster. The created endpoint and cluster are 
 * stored in the returned matter_config_t structure. The function also sets the name and attribute_id fields of 
 * the matter_config_t structure. 
 * 
 * @param node  A pointer to the node_t structure
 * 
 * @return  A structure that contains information about the created temperature sensor endpoint and cluster
 */
  node::config_t node_config;
  node_t *node = node::create(&node_config, on_attribute_update, on_identification);

  endpoint_t *endpoint;
  cluster_t *cluster;

  // Save windowcovering attribute reference. It will be used to read attribute value later.
  attribute_ref =
    attribute::get(cluster::get(endpoint, CLUSTER_ID), ATTRIBUTE_ID);
    

  // Save generated endpoint id
  windowcovering_endpoint_id = endpoint::get_id(endpoint);
  ESP_LOGD(TAG, "Window covering endpoint created with id %d", windowcovering_endpoint_id);

  // !!!
  // USE ONLY ABOUT 4 ENDPOINTS TO AVOID OUT OF MEMORY ERRORS
  // MANUALLY COMMENT REST OF ENDPOINTS
  // !!!
  
  window_covering_device::config_t wcd_config(static_cast<uint8_t>(chip::app::Clusters::WindowCovering::EndProductType::kRollerShutter));
  window_covering_device::config_t window_covering_device_config;
  endpoint = window_covering_device::create(node, &window_covering_device_config, ENDPOINT_FLAG_NONE, NULL);
  /* Add additional control clusters */
  cluster = cluster::get(endpoint, WindowCovering::Id);
  ESP_LOGD(TAG, "window_covering_device cluster created with cluster_id %d", cluster);
  cluster::window_covering::feature::lift::config_t lift;
  cluster::window_covering::feature::position_aware_lift::config_t position_aware_lift;
  nullable<uint8_t> percentage = nullable<uint8_t>(0);
    nullable<uint16_t> percentage_100ths = nullable<uint16_t>(0);
    position_aware_lift.current_position_lift_percentage = percentage;
    position_aware_lift.target_position_lift_percent_100ths = percentage_100ths;
    position_aware_lift.current_position_lift_percent_100ths = percentage_100ths;
  cluster::window_covering::feature::absolute_position::config_t absolute_position;
  cluster::window_covering::feature::lift::add(cluster, &lift);
  cluster::window_covering::feature::position_aware_lift::add(cluster, &position_aware_lift);
  cluster::window_covering::feature::absolute_position::add(cluster, &absolute_position);
  print_endpoint_info("window_covering_device", endpoint);
  ESP_LOGD(TAG, "window_covering_device %d", endpoint);

  // Setup DAC (this is good place to also set custom commission data, passcodes etc.)
  esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

  esp_matter::start(on_device_event);

  PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
}

// Reads plugin unit lift attribute value
esp_matter_attr_val_t get_lift_attribute_value(esp_matter::attribute_t *attribute_ref) {
  esp_matter_attr_val_t newPercentage = esp_matter_invalid(NULL);
  attribute::get_val(attribute_ref, &newPercentage);
  return newPercentage;
}

// Sets plugin unit lift attribute value
void set_lift_attribute_value(esp_matter_attr_val_t *newPercentage, uint16_t windowcovering_endpoint_id) {
  attribute::update(windowcovering_endpoint_id, CLUSTER_ID, ATTRIBUTE_ID, newPercentage);
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