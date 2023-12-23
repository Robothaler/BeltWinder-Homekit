#ifndef CONFIG_H
#define CONFIG_H

#pragma once

// -- Variables
int dir = 0, count = 0, maxCount, newCount = 0, lastSendPercentage = 0, lastTargetPosition = 0, bufferedPercentage = 0, filteredPercentage = 0;
const int filterDuration = 800;
float percentage = 0;
bool calButton = false, remote = false, posCertain = false, calMode = false, calUp = false, calDown = false, lastSendPercentageInit = false, newCountInit = false, maxCountInit = false, counted = false, posChange = false,posRunUp = false, posRunDown = false;
unsigned long lastUpdateTime = 0;


// Pin definitions
#define PULSECOUNTER    18    // Pulse counter
#define MOTOR_UP        17    // Motor drives up
#define MOTOR_DOWN      16    // Motor drives down
#define BUTTON_UP       21    // Button for up
#define BUTTON_DOWN     22    // Button for down

#endif // CONFIG_H