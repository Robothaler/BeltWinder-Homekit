
#include <nvs_flash.h>
#include <Matter.h>
#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
static const char *TAG = "app_main";
constexpr auto k_timeout_seconds = 300;
static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type)
    {
    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;
case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;
case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;
case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;
    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
    {
        ESP_LOGI(TAG, "Fabric removed successfully");
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
        {
            chip::CommissioningWindowManager &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
            constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
            if (!commissionMgr.IsCommissioningWindowOpen())
            {
                /* After removing last fabric, this example does not remove the Wi-Fi credentials
                 * and still has IP connectivity so, only advertising on DNS-SD.
                 */
                CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds,
                                                                            chip::CommissioningWindowAdvertisement::kDnssdOnly);
                if (err != CHIP_NO_ERROR)
                {
                    ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
                }
            }
        }
        break;
    }
    default:
        break;
    }
}
extern "C" void app_main()
{
    nvs_flash_init();
    esp_matter::node::config_t node_config;
    esp_matter::node_t *node = esp_matter::node::create(&node_config, NULL, NULL);
    esp_matter::endpoint::window_covering_device::config_t window_config;
    esp_matter::endpoint_t *endpoint = esp_matter::endpoint::window_covering_device::create(node, &window_config, esp_matter::endpoint_flags::ENDPOINT_FLAG_NONE, NULL);
esp_matter::cluster_t *cluster = esp_matter::cluster::get(endpoint, chip::app::Clusters::WindowCovering::Id);
    esp_matter::cluster::window_covering::feature::lift::config_t lift;
    esp_matter::cluster::window_covering::feature::position_aware_lift::config_t position_aware_lift;
nullable<uint8_t> percentage = nullable<uint8_t>(0);
    nullable<uint16_t> percentage_100ths = nullable<uint16_t>(0);
    position_aware_lift.current_position_lift_percentage = percentage;
    position_aware_lift.target_position_lift_percent_100ths = percentage_100ths;
    position_aware_lift.current_position_lift_percent_100ths = percentage_100ths;
esp_matter::cluster::window_covering::feature::absolute_position::config_t absolute_position;
    esp_matter::cluster::window_covering::feature::lift::add(cluster, &lift);
    esp_matter::cluster::window_covering::feature::position_aware_lift::add(cluster, &position_aware_lift);
    esp_matter::cluster::window_covering::feature::absolute_position::add(cluster, &absolute_position);
esp_matter::start(app_event_cb);
}
