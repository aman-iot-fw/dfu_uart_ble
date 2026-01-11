/******************************************************************************

@file  app_data.c

@brief This file contains the application data functionality

Group: WCS, BTS
Target Device: cc23xx

******************************************************************************

 Copyright (c) 2022-2024, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************


*****************************************************************************/

//*****************************************************************************
//! Includes
//*****************************************************************************
#include <FreeRTOS.h>
#include <task.h>

#include <string.h>
#include <ti/bleapp/ble_app_util/inc/bleapputil_api.h>
#include <ti/bleapp/menu_module/menu_module.h>
#include <app_main.h>


//*****************************************************************************
//! Defines
//*****************************************************************************
#define SIMPLESTREAMSERVER_DATAIN_UUID 0x0002
#define SIMPLESTREAMSERVER_DATAOUT_UUID 0x0003
//*****************************************************************************
//! Globals
extern UART2_Handle uart;
uint16_t serviceStartHandle = 0;
uint16_t serviceEndHandle = 0;
uint16_t dataInCharHandle = 0;
uint16_t dataOutCharHandle = 0;
extern uint16_t connHandle;
extern uint8_t uartReadBuffer[UART_MAX_READ_SIZE];
//*****************************************************************************
uint8_t charVal;
static void GATT_EventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData);
void initiateMTUExchange(uint16_t connHandle, uint16_t mtuSize);

// Events handlers struct, contains the handlers and event masks
// of the application data module
BLEAppUtil_EventHandler_t dataGATTHandler =
{
    .handlerType    = BLEAPPUTIL_GATT_TYPE,
    .pEventHandler  = GATT_EventHandler,
    .eventMask      = BLEAPPUTIL_ATT_MTU_UPDATED_EVENT |
                      BLEAPPUTIL_ATT_READ_RSP |
                      BLEAPPUTIL_ATT_WRITE_RSP |
                      BLEAPPUTIL_ATT_EXCHANGE_MTU_RSP |
                      BLEAPPUTIL_ATT_ERROR_RSP |
                      BLEAPPUTIL_ATT_HANDLE_VALUE_NOTI |
                      BLEAPPUTIL_ATT_FIND_BY_TYPE_VALUE_RSP |
                      BLEAPPUTIL_ATT_READ_BY_TYPE_RSP
};

//*****************************************************************************
//! Functions
//*****************************************************************************

/*********************************************************************
 * @fn      GATT_EventHandler
 *
 * @brief   The purpose of this function is to handle GATT events
 *          that rise from the GATT and were registered in
 *          @ref BLEAppUtil_RegisterGAPEvent
 *
 * @param   event - message event.
 * @param   pMsgData - pointer to message data.
 *
 * @return  none
 */
static void GATT_EventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData) {
    gattMsgEvent_t *gattMsg = (gattMsgEvent_t *)pMsgData;
    char buffer[50]; // Buffer for UART logs
    switch (gattMsg->method) {
        case ATT_FIND_BY_TYPE_VALUE_RSP:
            if (gattMsg->msg.findByTypeValueRsp.numInfo > 0) {
                serviceStartHandle = ATT_ATTR_HANDLE(gattMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);
                serviceEndHandle = ATT_GRP_END_HANDLE(gattMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);

                if (serviceStartHandle != 0 && serviceEndHandle != 0) {
//                    snprintf(buffer, sizeof(buffer), "Start: 0x%04X, End: 0x%04X\n", serviceStartHandle, serviceEndHandle);
//                    UART2_write(uart, buffer, strlen(buffer), NULL);

                    discoverCharacteristics();

                    initiateMTUExchange(connHandle, 200);
                    // Proceed to characteristic discovery
                } else {
                    UART2_write(uart, "Invalid service handles\n", strlen("Invalid service handles\n"), NULL);
                }
            }
            break;

        case ATT_READ_BY_TYPE_RSP:
            if (gattMsg->msg.readByTypeRsp.numPairs > 0) {
                for (uint8_t i = 0; i < gattMsg->msg.readByTypeRsp.numPairs; i++) {
                    uint8_t *attrData = gattMsg->msg.readByTypeRsp.pDataList + (i * gattMsg->msg.readByTypeRsp.len);
                    uint16_t handle = BUILD_UINT16(attrData[3], attrData[4]);
                    uint16_t uuid = BUILD_UINT16(attrData[17], attrData[18]);

//                    snprintf(buffer, sizeof(buffer), "UUID: 0x%x, Handle: 0x%x\n", uuid, handle);
//                    UART2_write(uart, buffer, strlen(buffer), NULL);
                    // Check for specific characteristics by UUID
                    if (uuid == SIMPLESTREAMSERVER_DATAIN_UUID) {
                        dataInCharHandle = 0x25;
//                        UART2_write(uart, "DataIn characteristic found\n", strlen("DataIn characteristic found\n"), NULL);

                    } else if (uuid == SIMPLESTREAMSERVER_DATAOUT_UUID) {
                        dataOutCharHandle = handle;
//                        UART2_write(uart, "DataOut characteristic found\n", strlen("DataOut characteristic found\n"), NULL);

//                        uint8_t *data  = "Writing data to gatt data Out characteristic";
//                        writeToServerCharacteristic(data, 10);
                        enableNotifications(connHandle); // Enable notifications after discovery
                    }
                }
                }
            break;

        case ATT_WRITE_RSP:

//            ClockP_sleep(1);
//            UART2_write(uart, "Write completed successfully\n", strlen("Write completed successfully\n"), NULL);
            break;

        case ATT_HANDLE_VALUE_NOTI:
            {
                // Handle incoming notification data
                uint16_t handle = gattMsg->msg.handleValueNoti.handle;
                uint8_t *pValue = gattMsg->msg.handleValueNoti.pValue;
                uint16_t len = gattMsg->msg.handleValueNoti.len;
                uint16_t bytesWritten = 0;
                uint16_t chunkSize = 200;  // Define the max UART write size (200 bytes in this case)

//                snprintf(buffer, sizeof(buffer), "Size : %d\n", len);
//                UART2_write(uart, buffer, strlen(buffer), NULL);

                // Loop to write data to UART in manageable chunks
                while (bytesWritten < len) {
                    uint16_t bytesToWrite = (len - bytesWritten) > chunkSize ? chunkSize : (len - bytesWritten);

                    // Write a chunk to UART
                    UART2_write(uart, pValue + bytesWritten, bytesToWrite, NULL);
//                    vTaskDelay(pdMS_TO_TICKS(5));

                    // Update the bytes written
                    bytesWritten += bytesToWrite;
                    GATT_bm_free(&gattMsg->msg,gattMsg->method);

                }
            }
            break;

        case ATT_MTU_UPDATED_EVENT:
//            snprintf(buffer, sizeof(buffer), "MTU updated to %d\n", gattMsg->msg.mtuEvt.MTU);
//            UART2_write(uart, buffer, strlen(buffer), NULL);
            break;

        case ATT_EXCHANGE_MTU_RSP:
            {
//                uint16_t clientRxMTU = gattMsg->msg.exchangeMTURsp.serverRxMTU;
//                snprintf(buffer, sizeof(buffer), "MTU Exchange Response: Client RX MTU = %d\n", clientRxMTU);
//                UART2_write(uart, buffer, strlen(buffer), NULL);
////                uint8_t hexData[] = {0x24, 0x45, 0x45, 0x50, 0x67, 0x73, 0x72, 0x23, 0x0D};
////                UART2_write(uart, hexData, sizeof(hexData), NULL);
//                vTaskDelay(pdMS_TO_TICKS(10));
            }
            break;

        case ATT_ERROR_RSP:
            snprintf(buffer, sizeof(buffer), "GATT error: %d\n", gattMsg->msg.errorRsp.errCode);
            UART2_write(uart, buffer, strlen(buffer), NULL);
            break;

        default:
            snprintf(buffer, sizeof(buffer), "Unhandled GATT method: 0x%02X\n", gattMsg->method);
            UART2_write(uart, buffer, strlen(buffer), NULL);
            break;
    }
}


void initiateMTUExchange(uint16_t connHandle, uint16_t mtuSize)
{
    attExchangeMTUReq_t mtuReq;
    bStatus_t status;
    char buffer[40];

    // Set the MTU size to request
    mtuReq.clientRxMTU = mtuSize;

    // Send the MTU exchange request to the server
    status = GATT_ExchangeMTU(connHandle, &mtuReq, BLEAppUtil_getSelfEntity());
    ClockP_sleep(1); // Small delay before discovery

    if (status == SUCCESS)
    {
        //earlier log to intiate mtu exchange
        //Write command to read meter serial number after connection
//        UART2_write(uart, "$EEPgsr#<CR>", strlen("$EEPgsr#<CR>"), NULL);
//        uint8_t hexData[] = {0x24, 0x45, 0x45, 0x50, 0x67, 0x73, 0x72, 0x23, 0x0D};
//        UART2_write(uart, hexData, sizeof(hexData), NULL);
//        vTaskDelay(pdMS_TO_TICKS(10));
    }
    else
    {
//        snprintf(buffer, sizeof(buffer), "Failed MTU exchange: Reason: 0x%x\n", status);
//        UART2_write(uart, buffer, strlen(buffer), NULL);
//        vTaskDelay(pdMS_TO_TICKS(10));
//        UART2_write(uart, "Failed to initiate MTU exchange\n", strlen("Failed to initiate MTU exchange\n"), NULL);
    }
}


/*********************************************************************
 * @fn      Data_start
 *
 * @brief   This function is called after stack initialization,
 *          the purpose of this function is to initialize and
 *          register the specific events handlers of the data
 *          application module
 *
 * @return  SUCCESS, errorInfo
 */
bStatus_t Data_start( void )
{
  bStatus_t status = SUCCESS;

  // Register the handlers
  status = BLEAppUtil_registerEventHandler( &dataGATTHandler );

  // Return status value
  return( status );
}
