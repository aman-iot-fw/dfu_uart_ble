/*
 * config.h
 *
 *  Created on: 11-July-2024
 *      Author: aman
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <time.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART2.h>
#include <app_main.h>
#include <ti/drivers/NVS.h>
#include "ti_drivers_config.h"

//From bytes config data will be stored in NVS Internal 1. Memory location 0x7BC00
#define MEM_BYTES 302080
#define MAX_NAME_LENGTH 20

//#pragma pack(push,1)
typedef struct{
    uint32_t bleRole;
    uint32_t passKey;
    uint8_t  deviceName[MAX_NAME_LENGTH];
}configurableData_t;
//#pragma pack(pop)



void parseConfigData(uint8_t *Data);
void Config_init(void);
uint32_t swapBytes(uint32_t value);

#endif /* APP_CONFIG_H_ */
