/*
 * config.c
 *
 *  Created on: 11-July-2024
 *      Author: aman
 */

#include "config.h"
#include <ti/drivers/Power.h>
#include "ti/bleapp/util/sw_update/sw_update.h"
#include "ti/bleapp/profiles/oad/oad_profile.h"




static NVS_Handle nvsHandle;
static NVS_Attrs regionAttrs;
static NVS_Params nvsParams;


uint8_t nvsWriteFlag = 0;
extern UART2_Handle uart;

configurableData_t nvsConfigData;

//It will store the config data with swaped endianness big-endian to little endian
configurableData_t swapedConfigData;


// Function to swap endianness of a 32-bit integer
uint32_t swapBytes(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8) & 0x0000FF00)  |
           ((value << 8) & 0x00FF0000)  |
           ((value << 24) & 0xFF000000);
}


// Function to parse and handle configuration data from UART
void parseConfigData(uint8_t *Data) {
        // Determine the command type and parse accordingly
    uint8_t commandType = *(Data + 1); // Get the command type
    uint8_t dataLength = *(Data + 4);

    nvsHandle = NVS_open(CONFIG_NVSINTERNAL1, &nvsParams);
    if (nvsHandle == NULL) {
        while (1); // Handle NVS open failure
    }

    switch (commandType) {
    bStatus_t status = SUCCESS;
    uint8_t sData;
        case 0x11: // Set BLE name
        {
            memset(nvsConfigData.deviceName, 0, sizeof(nvsConfigData.deviceName));
            memcpy(nvsConfigData.deviceName, (Data + 5), sizeof(nvsConfigData.deviceName));
            if (strlen(nvsConfigData.deviceName) == dataLength) {
                UART2_write(uart, "Ack", 3, NULL);
                NVS_write(nvsHandle, MEM_BYTES, (void*)&nvsConfigData, sizeof(nvsConfigData), NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
            } else {
                UART2_write(uart, "Nack", 4, NULL);
            }
            break;
        }
        case 0x0E: // Command to go to image reception mode over UART
        {
            // Invalidate the current image
//            status = SwUpdate_RevokeImage(INT_PRIMARY_SLOT);
//            oadResetDevice(0);
            uint8_t flashStat = 0xFF;
            struct image_header emptyHeader = {0};

           if(NVS_write(nvsHandle, 0x0, (uint8_t *)&(emptyHeader), 32, NVS_WRITE_PRE_VERIFY | NVS_WRITE_POST_VERIFY)
                     == NVS_STATUS_SUCCESS)
           {
               flashStat = SUCCESS;
           }

            if(flashStat != SUCCESS){
                uint8_t errorMsg[] = {0x02, 0x00, 0x04, 0xCC, 0x00, 0x00, 0x03};
                UART2_write(uart, errorMsg, sizeof(errorMsg), NULL);
            } else {
//                uint8_t infoMsg[] = {0x02, 0xAA, 0xBB, 0xCC, 0x03};
//                UART2_write(uart, infoMsg, sizeof(infoMsg), NULL);
            }
            break;
        }
        case 0x04: // Switch the ble role to Central
        {
            nvsConfigData.bleRole = 0xBB;
            status = NVS_write(nvsHandle, MEM_BYTES, (void*)&nvsConfigData, sizeof(nvsConfigData), NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
            if(status != SUCCESS){
                uint8_t errorMsg[] = {0x02, 0xCC, 0xBB, 0xCC, 0x03};
                UART2_write(uart, errorMsg, sizeof(errorMsg), NULL);
            } else {
                uint8_t infoMsg[] = {0x02, 0xAA, 0xBB, 0xCC, 0x03};
                UART2_write(uart, infoMsg, sizeof(infoMsg), NULL);
            }
            break;
        }
        case 0x05: // Switch the 05 role to peripheral
        {
            nvsConfigData.bleRole = 0xAA;
            NVS_write(nvsHandle, MEM_BYTES, (void*)&nvsConfigData, sizeof(nvsConfigData), NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
            if(status != SUCCESS){
                uint8_t errorMsg[] = {0x02, 0xCC, 0xBB, 0xCC, 0x03};
                UART2_write(uart, errorMsg, sizeof(errorMsg), NULL);
            } else {
                uint8_t infoMsg[] = {0x02, 0xAA, 0xBB, 0xCC, 0x03};
                UART2_write(uart, infoMsg, sizeof(infoMsg), NULL);
            }
            break;
        }
        case 0x31: // Set passkey
        {
            memcpy(&nvsConfigData.passKey, (Data + 5), sizeof(nvsConfigData.passKey));
            if(dataLength == 4){
                UART2_write(uart, "Ack", 3, NULL);
                NVS_write(nvsHandle, MEM_BYTES, (void*)&nvsConfigData, sizeof(nvsConfigData), NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
            } else {
                UART2_write(uart, "Nack", 4, NULL);
            }
            break;
        }
        default:
        {
            NVS_close(nvsHandle);
            return;
        }
    }
    NVS_close(nvsHandle);
    Power_reset();
}


//Initialize and read NVS
void Config_init(void)
{
//    NVS_init();

    NVS_Params_init(&nvsParams);
    nvsHandle = NVS_open(CONFIG_NVSINTERNAL1, &nvsParams);
    if (nvsHandle == NULL)
    {
        while(1);
    }

    NVS_getAttrs(nvsHandle, &regionAttrs);
    //NVS_erase(nvsHandle, 0, regionAttrs.sectorSize);
    //NVS_write(nvsHandle, 0, (configurableData_t *)&nvsConfigData, sizeof(nvsConfigData), NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
    NVS_read(nvsHandle, MEM_BYTES,(configurableData_t *)&nvsConfigData, sizeof(nvsConfigData));
    NVS_close(nvsHandle);

    swapedConfigData.passKey=swapBytes(nvsConfigData.passKey);
    swapedConfigData.bleRole=nvsConfigData.bleRole;
}

