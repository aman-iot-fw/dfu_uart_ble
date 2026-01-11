/******************************************************************************

@file  app_peripheral.c

@brief This example file demonstrates how to activate the peripheral role with
the help of BLEAppUtil APIs.

Two structures are used for event handling, one for connection events and one
for advertising events.
In each, eventMask is used to specify the events that will be received
and handled.
In addition, fill the BLEAppUtil_AdvInit_t structure with variables generated
by the Sysconfig.

In the events handler functions, write what actions are done after each event.
In this example, after a connection is made, activation is performed for
re-advertising up to the maximum connections.

In the Peripheral_start() function at the bottom of the file, registration,
initialization and activation are done using the BLEAppUtil API functions,
using the structures defined in the file.

More details on the functions and structures can be seen next to the usage.

Group: WCS, BTS
$Target Device: DEVICES $

******************************************************************************
$License: BSD3 2022 $
******************************************************************************
$Release Name: PACKAGE NAME $
$Release Date: PACKAGE RELEASE DATE $
*****************************************************************************/

#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( PERIPHERAL_CFG ) )

//*****************************************************************************
//! Includes
//*****************************************************************************
#include "ti_ble_config.h"
#include <ti/bleapp/ble_app_util/inc/bleapputil_api.h>
#include <ti/bleapp/menu_module/menu_module.h>
#include <app_main.h>

//*****************************************************************************
//! Prototypes
//*****************************************************************************
void Peripheral_AdvEventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData);
void Peripheral_GAPConnEventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData);
void Peripheral_GAPConnReportHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData);

//*****************************************************************************
//! Globals
//*****************************************************************************
int8_t rssiT;
extern configurableData_t swapedConfigData;
extern configurableData_t nvsConfigData;

BLEAppUtil_EventHandler_t peripheralConnHandler =
{
    .handlerType    = BLEAPPUTIL_GAP_CONN_TYPE,
    .pEventHandler  = Peripheral_GAPConnEventHandler,
    .eventMask      = BLEAPPUTIL_LINK_ESTABLISHED_EVENT |
                      BLEAPPUTIL_LINK_PARAM_UPDATE_REQ_EVENT |
                      BLEAPPUTIL_LINK_TERMINATED_EVENT
};

BLEAppUtil_EventHandler_t peripheralAdvHandler =
{
    .handlerType    = BLEAPPUTIL_GAP_ADV_TYPE,
    .pEventHandler  = Peripheral_AdvEventHandler,
    .eventMask      = BLEAPPUTIL_ADV_START_AFTER_ENABLE |
                      BLEAPPUTIL_ADV_END_AFTER_DISABLE
};

BLEAppUtil_EventHandler_t peripheralConnNotiHandler =
{
    .handlerType    = BLEAPPUTIL_CONN_NOTI_TYPE,
    .pEventHandler  = Peripheral_GAPConnReportHandler,
    .eventMask      = BLEAPPUTIL_CONN_NOTI_CONN_EVENT_ALL,
};

struct ADV_CONT_HANDLER Adv_controlHandler;
uint8 peripheralAdvHandle_1;

//! Store handle needed for each advertise set
uint8 peripheralAdvHandle_1;
uint8_t advData2[] =
{
 0x15,
 GAP_ADTYPE_LOCAL_NAME_COMPLETE,
 'O', 'm', 'k', 'a', 'r', 'E', 'n', 'e', 'r', 'g', 'y', '_', '0', '0', '0', '0', '0', '0', '0','1',
 0x02,
 0x01,
 0x06,
 0x04,  // Length of manufacturer data
 GAP_ADTYPE_MANUFACTURER_SPECIFIC,  // Manufacturer data type
 0x0D, 0x00,  // TI Manufacturer ID
 0xC0 // COFFEE data
};

//! Advertise param, needed for each advertise set, Generate by Sysconfig
BLEAppUtil_AdvInit_t advSetInitParamsSet_1 =
{
    /* Advertise data and length */
    .advDataLen        = sizeof(advData2),
    .advData           = advData2,

    /* Scan respond data and length */
//    .scanRespDataLen   = sizeof(scanResData1),
//    .scanRespData      = scanResData1,

    .advParam          = &advParams1
};

const BLEAppUtil_AdvStart_t advSetStartParamsSet_1 =
{
    /* Use the maximum possible value. This is the spec-defined maximum for */
    /* directed advertising and infinite advertising for all other types */
    .enableOptions         = GAP_ADV_ENABLE_OPTIONS_USE_MAX,
    .durationOrMaxEvents   = 0
};

//*****************************************************************************
//! Functions
//*****************************************************************************

//To read RSSI
void Peripheral_GAPConnReportHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData)
{
    switch(event)
    {

        case BLEAPPUTIL_CONN_NOTI_CONN_EVENT_ALL:
        {
            Gap_ConnEventRpt_t *report = (Gap_ConnEventRpt_t *)pMsgData;
            rssiT = report->lastRssi;
            //Your code
            break;
        }

        default:
        {
            break;
        }
    }
}

/*********************************************************************
 * @fn      Peripheral_AdvEventHandler
 *
 * @brief   The purpose of this function is to handle advertise events
 *          that rise from the GAP and were registered in
 *          @ref BLEAppUtil_registerEventHandler
 *
 * @param   event - message event.
 * @param   pMsgData - pointer to message data.
 *
 * @return  none
 */
void Peripheral_AdvEventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData)
{
    switch(event)
    {
        case BLEAPPUTIL_ADV_START_AFTER_ENABLE:
        {
//            MenuModule_printf(APP_MENU_ADV_EVENT, 0, "Adv status: Started - handle: "
//                              MENU_MODULE_COLOR_YELLOW "%d" MENU_MODULE_COLOR_RESET,
//                              ((BLEAppUtil_AdvEventData_t *)pMsgData)->pBuf->advHandle);
            break;
        }

        case BLEAPPUTIL_ADV_END_AFTER_DISABLE:
        {
//            MenuModule_printf(APP_MENU_ADV_EVENT, 0, "Adv status: Ended - handle: "
//                              MENU_MODULE_COLOR_YELLOW "%d" MENU_MODULE_COLOR_RESET,
//                              ((BLEAppUtil_AdvEventData_t *)pMsgData)->pBuf->advHandle);
            if (GapAdv_prepareLoadByBuffer(peripheralAdvHandle_1, GAP_ADV_FREE_OPTION_DONT_FREE) == SUCCESS)
            {
//                // Load Buffer
//                for(uint8 i = 0; i< 8; i++)
//                {
//                    advData2[10+i] = Adv_controlHandler.payload[i];
//                }

                // Reload buffer to handle
                 GapAdv_loadByHandle(peripheralAdvHandle_1, GAP_ADV_DATA_TYPE_ADV, advSetInitParamsSet_1.advDataLen , advData2);
                 // Works with connec & disconnect - end -

                 //If in disconnected state then only start adv. again otherwise bypass
                 if(Adv_controlHandler.connFlag == 0)
                 {
                     BLEAppUtil_advStart(peripheralAdvHandle_1, &advSetStartParamsSet_1);
                 }
            }
            break;
        }

        default:
        {
            break;
        }
    }
}


/*********************************************************************
 * @fn      Peripheral_GAPConnEventHandler
 *
 * @brief   The purpose of this function is to handle connection related
 *          events that rise from the GAP and were registered in
 *          @ref BLEAppUtil_registerEventHandler
 *
 * @param   event - message event.
 * @param   pMsgData - pointer to message data.
 *
 * @return  none
 */
void Peripheral_GAPConnEventHandler(uint32 event, BLEAppUtil_msgHdr_t *pMsgData)
{
    switch(event)
    {
        case BLEAPPUTIL_LINK_ESTABLISHED_EVENT:
        {
            gapEstLinkReqEvent_t *gapEstMsg = (gapEstLinkReqEvent_t *)pMsgData;
            BLEAppUtil_registerConnNotifHandler(gapEstMsg->connectionHandle);

            Adv_controlHandler.connFlag = 1;
            /* Check if we reach the maximum allowed number of connections */
            if(linkDB_NumActive() < linkDB_NumConns())
            {
                /* Start advertising since there is room for more connections */
                BLEAppUtil_advStart(peripheralAdvHandle_1, &advSetStartParamsSet_1);
            }
            else
            {
                /* Stop advertising since there is no room for more connections */
                BLEAppUtil_advStop(peripheralAdvHandle_1);
            }
            break;
        }

        case BLEAPPUTIL_LINK_TERMINATED_EVENT:
        {
            Adv_controlHandler.connFlag = 0;
            BLEAppUtil_advStart(peripheralAdvHandle_1, &advSetStartParamsSet_1);
            break;
        }

        default:
        {
            break;
        }
    }
}

/*********************************************************************
 * @fn      Peripheral_start
 *
 * @brief   This function is called after stack initialization,
 *          the purpose of this function is to initialize and
 *          register the specific events handlers of the peripheral
 *          application module
 *
 * @return  SUCCESS, errorInfo
 */
bStatus_t Peripheral_start()
{
    bStatus_t status = SUCCESS;

    if (nvsConfigData.deviceName[0] != 0xFF)
    {
        UpdateAdvDataWithName(nvsConfigData.deviceName);
    }

    status = BLEAppUtil_registerEventHandler(&peripheralConnNotiHandler);
    if(status != SUCCESS)
    {
        // Return status value
        return(status);
    }

//    //advertise 3 digit serial number if it other than default value of NVS
//    if (swapedConfigData.serNo != 0xFFFFFFFF)
//    {
//        insertSerialNumber(swapedConfigData.serNo);
//    }

    status = BLEAppUtil_registerEventHandler(&peripheralConnHandler);
    if(status != SUCCESS)
    {
        // Return status value
        return(status);
    }

    // CHANGE TX POWER::
    const int8_t txPower = 8;
    const uint8_t fraction = 0;
    HCI_EXT_SetTxPowerDbmCmd(txPower, fraction);

    status = BLEAppUtil_registerEventHandler(&peripheralAdvHandler);
    if(status != SUCCESS)
    {
        return(status);
    }

//    strcpy(scanResData1, "A1234");
//    advSetInitParamsSet_1.scanRespDataLen = sizeof(scanResData1);
//    advSetInitParamsSet_1.scanRespData = scanResData1;

    status = BLEAppUtil_initAdvSet(&peripheralAdvHandle_1, &advSetInitParamsSet_1);
    if(status != SUCCESS)
    {
        // Return status value
        return(status);
    }

    status = BLEAppUtil_advStart(peripheralAdvHandle_1, &advSetStartParamsSet_1);
    if(status != SUCCESS)
    {
        // Return status value
        return(status);
    }

    // Return status value
    return(status);
}

// Function to update the advertisement data with a new device name
void UpdateAdvDataWithName(uint8_t *nameData) {
    uint8_t nameLength = 0x14; // The first byte is the length of the name

    // Calculate the advertisement data length, including the name length and type byte
    advData2[0] = nameLength + 1;
//    advData2[1] =0x09;

    // Copy the new device name into advData
    memcpy(&advData2[2], nameData, nameLength);

    // Place the flag bytes immediately after the name data
    advData2[nameLength + 2] = 0x02; // Length of flags field
    advData2[nameLength + 3] = 0x01; // Flags field type
    advData2[nameLength + 4] = 0x06; // Flags field data
}



#endif // ( HOST_CONFIG & ( PERIPHERAL_CFG ) )
