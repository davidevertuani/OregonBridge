#ifndef _DEVICES_H
#define _DEVICES_H

#define DEVICES_NUM 2

#include "OregonDevice_v1.h"
#include "OregonDevice_v2.h"

#define INCL_DEV(devClass) addDevice<devClass>();

#define INCLUDE_ALL_DEVICES \
    INCL_DEV(OregonDevice_v1) \
    INCL_DEV(OregonDevice_v2) \
    
#endif