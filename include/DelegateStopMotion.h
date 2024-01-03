#ifndef DELEGATESTOPMOTION_H
#define DELEGATESTOPMOTION_H

#pragma once

#include "Beltwinder.h"
#include <DebugUtils.h>

// -- Variables
extern int dir;
extern bool remote,posRunUp, posRunDown;

extern void up();
extern void down();

class CoveringDelegate : public chip::app::Clusters::WindowCovering::Delegate{
    public:

        CHIP_ERROR HandleMovement(chip::app::Clusters::WindowCovering::WindowCoveringType type){
            return CHIP_NO_ERROR;
        }

        CHIP_ERROR HandleStopMotion(){
            //Handle stopping of covering here
            DEBUGPRINTLN2("[BELTWINDER] Received StopMotionCommand"); 
            if(posRunUp || posRunDown){
            return CHIP_ERROR_IN_PROGRESS;
            }
            remote = false;
            if (dir == -1) {
                down();    
            } else if (dir == 1) {
                up();
            }
            return CHIP_ERROR_IN_PROGRESS;
        }

        ~CoveringDelegate() = default;
};

#endif // DELEGATESTOPMOTION_H