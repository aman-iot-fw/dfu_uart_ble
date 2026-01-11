// ble_central_client.c

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ti/drivers/UART2.h>
#include <ti/bleapp/ble_app_util/inc/bleapputil_api.h>
//#include "common/Services/data_stream/data_stream_server.h"

#include <app_main.h>
#include "gatt.h"
#include "gatt_uuid.h"
#include "ti_drivers_config.h"
#include "ti_ble_config.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// Constants for service and characteristic UUIDs
#define SIMPLESTREAMSERVER_SERV_UUID 0x0001
#define SIMPLESTREAMSERVER_DATAIN_UUID 0x0002
#define SIMPLESTREAMSERVER_DATAOUT_UUID 0x0003

// Function prototypes

static bool Central_findUuidAndManufacturerData(const uint8_t *pData, uint8_t dataLen);
static void uartReadCallback(UART2_Handle handle, void *buffer, size_t size, void *userArg, int_fast16_t status);

static void logError(const char *message);
void Central_ScanEventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData);
void connectionEventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData);
void discoverServices();
void discoverCharacteristics();

// UART handle and buffer
extern UART2_Handle uart;
//extern UART2_Handle uartHandle;
extern uartReadBuffer[UART_MAX_READ_SIZE];

// Connection and characteristic handles
uint16_t connHandle = LL_CONNHANDLE_INVALID;
extern uint16_t serviceStartHandle;
extern uint16_t serviceEndHandle;
extern uint16_t dataInCharHandle;
extern uint16_t dataOutCharHandle;
extern App_connInfo connectionConnList[MAX_NUM_BLE_CONNS];

uint8_t taskID;

// Scan and connection parameters
BLEAppUtil_ConnectParams_t centralConnParams =
{
    .phys = DEFAULT_INIT_PHY,
    .timeout = 3000
};

const BLEAppUtil_ScanInit_t centralScanInitParams =
{
    .primPhy                    = DEFAULT_SCAN_PHY,
    .scanType                   = DEFAULT_SCAN_TYPE,
    .scanInterval               = DEFAULT_SCAN_INTERVAL,
    .scanWindow                 = DEFAULT_SCAN_WINDOW,
    .advReportFields            = ADV_RPT_FIELDS,
    .scanPhys                   = DEFAULT_INIT_PHY,
    .fltPolicy                  = SCANNER_FILTER_POLICY,
    .fltPduType                 = SCANNER_FILTER_PDU_TYPE,
    .fltMinRssi                 = SCANNER_FILTER_MIN_RSSI,
    .fltDiscMode                = SCANNER_FILTER_DISC_MODE,
    .fltDup                     = 0
};

const BLEAppUtil_ConnParams_t centralConnInitParams =
{
    .initPhys              = DEFAULT_INIT_PHY,
    .scanInterval          = INIT_PHYPARAM_SCAN_INT,
    .scanWindow            = INIT_PHYPARAM_SCAN_WIN,
    .minConnInterval       = INIT_PHYPARAM_MIN_CONN_INT,
    .maxConnInterval       = INIT_PHYPARAM_MAX_CONN_INT,
    .connLatency           = INIT_PHYPARAM_CONN_LAT,
    .supTimeout            = INIT_PHYPARAM_SUP_TO
};

const BLEAppUtil_ScanStart_t centralScanStartParams =
{
    .scanPeriod     = DEFAULT_SCAN_PERIOD,
    .scanDuration   = 0,
    .maxNumReport   = 0
};

// Register event handlers and initialize scanning
BLEAppUtil_EventHandler_t scanHandler = {
    .handlerType = BLEAPPUTIL_GAP_SCAN_TYPE,
    .pEventHandler = Central_ScanEventHandler,
    .eventMask = BLEAPPUTIL_SCAN_ENABLED | BLEAPPUTIL_SCAN_DISABLED | BLEAPPUTIL_ADV_REPORT
};

BLEAppUtil_EventHandler_t connEventHandler = {
    .handlerType = BLEAPPUTIL_GAP_CONN_TYPE,
    .pEventHandler = connectionEventHandler,
    .eventMask = GAP_LINK_ESTABLISHED_EVENT | GAP_LINK_TERMINATED_EVENT
};


// Scan event handler with custom UUID and manufacturer data checking
void Central_ScanEventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData)
{
    BLEAppUtil_ScanEventData_t *scanMsg = (BLEAppUtil_ScanEventData_t *)pMsgData;

    switch (event)
    {
        case BLEAPPUTIL_SCAN_ENABLED:
            UART2_write(uart, "Scan Enabled\n", strlen("Scan Enabled\n"), NULL);
            break;

        case BLEAPPUTIL_SCAN_DISABLED:
            UART2_write(uart, "Scan Disabled\n", strlen("Scan Disabled\n"), NULL);
            vTaskDelay(pdMS_TO_TICKS(10));
            break;

        case BLEAPPUTIL_ADV_REPORT:
        {
            bleStk_GapScan_Evt_AdvRpt_t *pScanRpt = &scanMsg->pBuf->pAdvReport;
            if (pScanRpt->pData != NULL)
            {
                if (Central_findUuidAndManufacturerData(pScanRpt->pData, pScanRpt->dataLen))
                {
                    memcpy(centralConnParams.pPeerAddress, pScanRpt->addr, B_ADDR_LEN);
                    centralConnParams.peerAddrType = pScanRpt->addrType;
                    bStatus_t status = BLEAppUtil_connect(&centralConnParams);

                    if (status == SUCCESS)
                    {
                        UART2_write(uart, "Connecting to device\n", strlen("Connecting to device\n"), NULL);
//                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }
                else{
//                    vTaskDelay(pdMS_TO_TICKS(100));
//                    bStatus_t status = BLEAppUtil_scanStart(&centralScanStartParams);
//                    if (status == SUCCESS) {
//                        UART2_write(uart, "Scanning again to find mfg data\n", strlen("Scanning again to find mfg data\n"), NULL);
//                        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay (10 milliseconds)
//                    } else {
//                        logError("Failed to start scanning again for mfg data.");
//                    }
                }
            }
            break;
        }
        default:
            break;
    }
    UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);  // Set up UART for the next data reception

}

// Custom function to find UUID or manufacturer data in advertisement data
static bool Central_findUuidAndManufacturerData(const uint8_t *pData, uint8_t dataLen)
{
    const uint8_t targetUuid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x40, 0x51, 0x04, 0xC0, 0xC0, 0x00, 0xF0};
    const uint8_t targetManufData[3] = {0x0D, 0x00, 0xC0};
    uint8_t matchingIdAndUuid = 0;
    uint8_t adLen, adType;
    const uint8_t *pEnd = pData + dataLen - 1;

    while ((dataLen > 0) && (pData < pEnd))
    {
        adLen = *pData++;
        if (adLen > 0)
        {
            adType = *pData;
            if ((adType == GAP_ADTYPE_128BIT_MORE) || (adType == GAP_ADTYPE_128BIT_COMPLETE))
            {
                pData++;
                adLen--;

                while (adLen >= ATT_UUID_SIZE && pData < pEnd)
                {
                    if (!memcmp(pData, targetUuid, ATT_UUID_SIZE))
                    {
                        matchingIdAndUuid++;
                    }
                    pData += ATT_UUID_SIZE;
                    adLen -= ATT_UUID_SIZE;
                }
                if (adLen == 1)
                {
                    pData++;
                }
            }

            if (adType == GAP_ADTYPE_MANUFACTURER_SPECIFIC)
            {
                pData++;
                adLen--;

                uint8_t len = (adLen < sizeof(targetManufData)) ? adLen : sizeof(targetManufData);
                if (!memcmp(pData, targetManufData, len))
                {
                    matchingIdAndUuid++;
                    return true;
                }
            }

            if (matchingIdAndUuid < 2)
            {
                pData += adLen;
            }
            else
            {
                return true;
            }
        }
    }
    return false;
}


// Handle BLE notifications and write to UART
void handleBLENotification(gattMsgEvent_t *pMsg)
{
    if (pMsg->method == ATT_HANDLE_VALUE_NOTI && pMsg->msg.handleValueNoti.handle == dataOutCharHandle)
    {
        UART2_write(uart, pMsg->msg.handleValueNoti.pValue, pMsg->msg.handleValueNoti.len, NULL);
    }
}

// Write data to server characteristic
 void writeToServerCharacteristic(const uint8_t *data, size_t len)
{
    bStatus_t status;

    attWriteReq_t req;
    req.handle = dataInCharHandle;
    req.len = len;
    req.pValue  = GATT_bm_alloc(0x0, ATT_WRITE_REQ, len, NULL);
    req.sig =0;
    req.cmd=0;
    memcpy(req.pValue, data, len);

//    ClockP_sleep(1);
    status = GATT_WriteCharValue(0x0, &req, BLEAppUtil_getSelfEntity());

    char buffer[15];
    snprintf(buffer, sizeof(buffer), "Status:%d\n", status);
    UART2_write(uart, buffer, strlen(buffer), NULL);

    if( status != SUCCESS )
     {
//         GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
         UART2_write(uart, (const char *)"write fail...\r\n", strlen((const char *)"write fail...\r\n"), NULL);
     }
     else
     {
         UART2_write(uart, (const char *)"write ok...\r\n", strlen((const char *)"write ok...\r\n"), NULL);
     }
    UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);
}

// UART read callback
static void uartReadCallback(UART2_Handle handle, void *buffer, size_t size, void *userArg, int_fast16_t status) {

    writeToServerCharacteristic(buffer, size);
    UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);

}


//Discover the services on the seriver
void discoverServices(void) {
    uint8_t serviceUuid[ATT_UUID_SIZE] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E };
    bStatus_t status;

    // Initiate service discovery
    status = GATT_DiscPrimaryServiceByUUID(connHandle, serviceUuid, ATT_UUID_SIZE, BLEAppUtil_getSelfEntity());
    ClockP_sleep(1);
    if (status != SUCCESS) {
        logError("Failed to initiate service discovery.");
    } else {
//        UART2_write(uart, "Service discovery initiated\n", strlen("Service discovery initiated\n"), NULL);
    }
}

//Discover the characteristics inside the serivices
void discoverCharacteristics(void) {
    bStatus_t status;
    attReadByTypeReq_t req;  // Request structure for characteristic discovery

    // Ensure service handles are valid before proceeding
    if (serviceStartHandle != 0 && serviceEndHandle != 0) {
        // Discover the DataIn characteristic by UUID
        req.startHandle = serviceStartHandle;
        req.endHandle = serviceEndHandle;
        uint8_t dataInUuid[ATT_UUID_SIZE] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E };
        req.type.len = ATT_UUID_SIZE;
        memcpy(req.type.uuid, dataInUuid, ATT_UUID_SIZE);

        // Initiate discovery for DataIn characteristic
        status = GATT_DiscCharsByUUID(connHandle, &req, BLEAppUtil_getSelfEntity());
        ClockP_sleep(1); // Small delay before discovery

        if (status != SUCCESS) {
            logError("Failed to initiate DataIn characteristic discovery.");
        } else {
//            UART2_write(uart, "DataIn characteristic discovery initiated\n", strlen("DataIn characteristic discovery initiated\n"), NULL);
        }

//         Discover the DataOut characteristic by UUID
        uint8_t dataOutUuid[ATT_UUID_SIZE] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E };
        memcpy(req.type.uuid, dataOutUuid, ATT_UUID_SIZE);

//         Initiate discovery for DataOut characteristic
        status = GATT_DiscCharsByUUID(connHandle, &req, BLEAppUtil_getSelfEntity());
        ClockP_sleep(1); // Small delay before discovery

        if (status != SUCCESS) {
            logError("Failed to initiate DataOut characteristic discovery.");
        } else {
//            UART2_write(uart, "DataOut characteristic discovery initiated\n", strlen("DataOut characteristic discovery initiated\n"), NULL);
        }

    } else {
        UART2_write(uart, "Invalid service handles, cannot discover characteristics\n", strlen("Invalid service handles, cannot discover characteristics\n"), NULL);
    }
}


// Callback function for connection events
void connectionEventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData) {
    switch (event) {
        case BLEAPPUTIL_LINK_ESTABLISHED_EVENT:
        {
            gapEstLinkReqEvent_t *gapEstMsg = (gapEstLinkReqEvent_t *)pMsgData;
            connHandle = gapEstMsg->connectionHandle;
            UART2_write(uart, "Device connected\n", strlen("Device connected\n"), NULL);
            vTaskDelay(pdMS_TO_TICKS(10)); // Small delay (10 milliseconds)

            // Proceed to service discovery after stopping the scan
            discoverServices();
            break;
        }

        case BLEAPPUTIL_LINK_TERMINATED_EVENT:
        {
            UART2_write(uart, "Device disconnected\n", strlen("Device disconnected\n"), NULL);

            // Reset connection handle when disconnected
            connHandle = LL_CONNHANDLE_INVALID;
            break;
        }
        default:
            break;
    }
}

// Enable notification
void enableNotifications(uint16_t connHandle)
{
    // Define the request structure
    attWriteReq_t req;
    uint8_t configData[2] = {0x01, 0x00};  // Notification enable value
    req.pValue = GATT_bm_alloc(connHandle, ATT_WRITE_REQ, sizeof(configData), NULL);

    // Ensure the handle is valid and memory is allocated for the request
    if (req.pValue != NULL && dataOutCharHandle != 0)
    {
        // Set the handle to the CCCD (Client Characteristic Configuration Descriptor) handle
        req.handle = 0x29; //dataOutCharHandle + 1;  // CCCD is usually located right after characteristic handle
        // Set the data to enable notifications
        req.len = sizeof(configData);
        memcpy(req.pValue, configData, sizeof(configData));
        req.cmd = TRUE;    // Command flag indicates it's a write without response
        req.sig = FALSE;   // No signature required

        // Write the configuration without a response (Write Command)
        bStatus_t status = GATT_WriteNoRsp(connHandle, &req);
        ClockP_sleep(1);

        if (status != SUCCESS)
        {
            logError("Failed to write CCCD for enabling notifications.");
            GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);  // Free buffer in case of failure
        }
        else
        {
//            UART2_write(uart, "Notifications enabled successfully\n", strlen("Notifications enabled successfully\n"), NULL);
            vTaskDelay(pdMS_TO_TICKS(10));

        }
    }
    else
    {
        logError("Invalid handle or failed to allocate memory for CCCD write.");
    }
}

// Log error to UART
static void logError(const char *message)
{
    UART2_write(uart, (void *)message, strlen(message), NULL);
    vTaskDelay(pdMS_TO_TICKS(20));
}

//Function needs to called from the main for  central function initialization
void BLECentralClient_init(void)
{
    bStatus_t status;

    status = BLEAppUtil_registerEventHandler(&scanHandler);
    if(status != SUCCESS)
    {
        logError("Error registering scanhandler");
    }

    status = BLEAppUtil_scanInit(&centralScanInitParams);
    if(status != SUCCESS)
    {
        logError("Error registering initParam");
    }

    status = BLEAppUtil_scanStart(&centralScanStartParams);
    if(status != SUCCESS)
    {
        logError("Error registering startParam");
    }

    status = BLEAppUtil_setConnParams(&centralConnInitParams);
    if(status != SUCCESS)
    {
        logError("Error registering connInitParam");
    }

    status = BLEAppUtil_registerEventHandler(&connEventHandler);

    if (status != SUCCESS) {
          logError("Failed to register connection event handler.");
    }
    UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);  // Set up UART for the next data reception


}

