#ifndef BELTWINDER_H
#define BELTWINDER_H

#pragma once

#include <Arduino.h>
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <app/clusters/window-covering-server/window-covering-server.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include "DelegateStopMotion.h"

//Define debug level in Config.h
#include <DebugUtils.h>

#include <esp_log.h>
#include <esp_err.h>

using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace esp_matter::attribute;
using namespace esp_matter::cluster;
using namespace chip::app::Clusters::WindowCovering;

//Function prototypes
extern void setupPins();
extern void loadCount();
extern void loadMaxCount();
extern void up();
extern void down();
extern void saveCount(int count);
extern void setMaxCount();
extern void newPosition(int newPercentage);
extern void countPosition();
extern void calibrationRun();
extern void setCurrentDirection();
extern void sendCurrentPosition(int count, int maxCount);
extern void handlePosCertainty();
extern void moveToNewPos();
extern void handleCalibration();
extern void bufferNewPosition();
extern void set_window_covering_position(esp_matter_attr_val_t *current);
extern void set_onoff_attribute_value(esp_matter_attr_val_t *onoff_value, uint16_t switch_endpoint_id_1);
extern void set_operational_state(chip::EndpointId endpoint, OperationalState newState);
extern esp_matter_attr_val_t get_onoff_attribute_value();

// -- Variables
extern int dir, count, lastCount, maxCount, newCount, newMaxCount, lastSendPercentage, lastTargetPosition, bufferedPercentage, filteredPercentage, newPercentage;
extern const int filterDuration;
extern float percentage;
extern bool maxCountReloaded, newPercentageReceived, bufferedValueSaved, calButton, remote, posCertain, calMode, calUp, calDown, lastSendPercentageInit, newCountInit, maxCountInit, counted, posChange, posRunUp, posRunDown;
extern unsigned long lastUpdateTime;

// Custom keys for your data
extern const char * kMaxCountKey;
extern const char * kCountKey;

extern uint16_t window_covering_endpoint_id;
extern uint16_t switch_endpoint_id_1;
extern attribute_t *attribute_ref_target;
extern attribute_t *attribute_ref_1;

extern CoveringDelegate covering_delegate; 

#endif // BELTWINDER_H