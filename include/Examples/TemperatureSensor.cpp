#include <Arduino.h>
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;

// Cluster and attribute ID used by Matter Temperature device
const uint32_t CLUSTER_ID_TEMP = TemperatureMeasurement::Id;
const uint32_t ATTRIBUTE_ID_TEMP = TemperatureMeasurement::Attributes::MeasuredValue::Id;
uint16_t temperature_endpoint_id = 1;

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

static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
if (type == attribute::PRE_UPDATE) {
Serial.print("Update on endpoint: ");
Serial.print(endpoint_id);
Serial.print(" cluster: ");
Serial.print(cluster_id);
Serial.print(" attribute: ");
Serial.println(attribute_id);
}
return ESP_OK;
}

void print_endpoint_info(String clusterName, endpoint_t *endpoint) {
uint16_t endpoint_id = endpoint::get_id(endpoint);
Serial.print(clusterName);
Serial.print(" has endpoint: ");
Serial.println(endpoint_id);
}

// Sets temperature attribute value
void Set_temp_Attribute_Value(esp_matter_attr_val_t *temperature) {
attribute::update(temperature_endpoint_id, CLUSTER_ID_TEMP, ATTRIBUTE_ID_TEMP, temperature);

}

void setup() {
Serial.begin(115200);

esp_log_level_set("*", ESP_LOG_DEBUG);

node::config_t node_config;
node_t *node = node::create(&node_config, on_attribute_update, on_identification);

endpoint_t *endpoint;
cluster_t *cluster;

temperature_sensor::config_t temperature_sensor_config;
temperature_sensor_config.temperature_measurement.measured_value = 22;
endpoint = temperature_sensor::create(node, &temperature_sensor_config, ENDPOINT_FLAG_NONE, NULL);
print_endpoint_info("temperature_sensor", endpoint);

// Setup DAC (this is good place to also set custom commission data, passcodes etc.)
esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

esp_matter::start(on_device_event);

PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
}

void loop() {

int16_t Temper = 23 * 100; //example of a measured value
esp_matter_attr_val_t temperature = esp_matter_int16(Temper);
Set_temp_Attribute_Value(&temperature);
delay(5000);
}