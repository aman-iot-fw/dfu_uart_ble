/******************************************************************************

@file  app_main.c

@brief This file contains the application main functionality
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
#include "ti_ble_config.h"
#include <ti/bleapp/ble_app_util/inc/bleapputil_api.h>
#include <ti/bleapp/menu_module/menu_module.h>
#include <health_toolkit/inc/debugInfo.h>
#include <app_main.h>
#include <ti/bleapp/profiles/data_stream/data_stream_profile.h>
#include <ti/bleapp/profiles/simple_gatt/simple_gatt_profile.h>
#include "sys/types.h"


extern configurableData_t swapedConfigData;
uint8_t connectionFlag =0;

//Git add changes
//*****************************************************************************
//! Defines
//*****************************************************************************

//*****************************************************************************
//! Globals
//*****************************************************************************
UART2_Handle uart;
UART2_Params uartParams;

// Parameters that should be given as input to the BLEAppUtil_init function
BLEAppUtil_GeneralParams_t appMainParams =
{
    .taskPriority = 1,
    .taskStackSize = 1024,
    .profileRole = (BLEAppUtil_Profile_Roles_e)(HOST_CONFIG),
    .addressMode = DEFAULT_ADDRESS_MODE,
    .deviceNameAtt = attDeviceName,
//    .pDeviceRandomAddress = pRandomAddress,
};

BLEAppUtil_PeriCentParams_t appMainPeriCentParams =
{
#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( PERIPHERAL_CFG ) )
 .connParamUpdateDecision = DEFAULT_PARAM_UPDATE_REQ_DECISION,
#endif

#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( PERIPHERAL_CFG | CENTRAL_CFG ) )
 .gapBondParams = &gapBondParams
#endif
};

//*****************************************************************************
//! Functions
//*****************************************************************************

/*********************************************************************
 * @fn      criticalErrorHandler
 *
 * @brief   Application task entry point
 *
 * @return  none
 */
void criticalErrorHandler(int32 errorCode , void* pInfo)
{
//    trace();
//
//#ifdef DEBUG_ERR_HANDLE
//
//    while (1);
//#else
//    SystemReset();
//#endif
}

/*********************************************************************
 * @fn      App_StackInitDone
 *
 * @brief   This function will be called when the BLE stack init is
 *          done.
 *          It should call the applications modules start functions.
 *
 * @return  none
 */

void App_StackInitDoneHandler(gapDeviceInitDoneEvent_t *deviceInitDoneData)
{
    bStatus_t status = SUCCESS;
    uint8_t pinState = GPIO_read(GPIO_BLE_ROLE);

    // Print the device ID address
//    MenuModule_printf(APP_MENU_DEVICE_ADDRESS, 0, "BLE ID Address: "
//                      MENU_MODULE_COLOR_BOLD MENU_MODULE_COLOR_GREEN "%s" MENU_MODULE_COLOR_RESET,
//                      BLEAppUtil_convertBdAddr2Str(deviceInitDoneData->devAddr));
//
//    if ( appMainParams.addressMode > ADDRMODE_RANDOM)
//    {
//      // Print the RP address
//        MenuModule_printf(APP_MENU_DEVICE_RP_ADDRESS, 0,
//                     "BLE RP Address: "
//                     MENU_MODULE_COLOR_BOLD MENU_MODULE_COLOR_GREEN "%s" MENU_MODULE_COLOR_RESET,
//                     BLEAppUtil_convertBdAddr2Str(GAP_GetDevAddress(FALSE)));
//    }

//#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( PERIPHERAL_CFG | CENTRAL_CFG ) )
//    status = DataStream_start();
//    if ( status != SUCCESS )
//    {
//    // TODO: Call Error Handler
//    }
//#endif

#ifdef BLE_HEALTH
//    status = DbgInf_init( DBGINF_DOMAIN_ALL );
    if ( status != SUCCESS )
    {
    // TODO: Call Error Handler
    }
#endif

#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( PERIPHERAL_CFG ) )
    // Any device that accepts the establishment of a link using
    // any of the connection establishment procedures referred to
    // as being in the Peripheral role.
    // A device operating in the Peripheral role will be in the
    // Peripheral role in the Link Layer Connection state.

    if (swapedConfigData.bleRole == 0xAA) {
        status = Peripheral_start();
        if(status != SUCCESS)
        {
        // TODO: Call Error Handler
        }
    }
    else if(swapedConfigData.bleRole == 0xBB){
        BLECentralClient_init();
    }
    else{
        status = Peripheral_start();
        if(status != SUCCESS)
        {
        // TODO: Call Error Handler
        }
    }

#endif

#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( BROADCASTER_CFG ) )
    // A device operating in the Broadcaster role is a device that
    // sends advertising events or periodic advertising events
    status = Broadcaster_start();
    if(status != SUCCESS)
    {
    // TODO: Call Error Handler
    }
#endif

#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( CENTRAL_CFG ) )
    // A device that supports the Central role initiates the establishment
    // of an active physical link. A device operating in the Central role will
    // be in the Central role in the Link Layer Connection state.
    // A device operating in the Central role is referred to as a Central.
//    status = Central_start();
//    if(status != SUCCESS)
//    {
//    // TODO: Call Error Handler
//    }
#endif

#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( OBSERVER_CFG ) )
    // A device operating in the Observer role is a device that
    // receives advertising events or periodic advertising events
    status = Observer_start();
    if(status != SUCCESS)
    {
    // TODO: Call Error Handler
    }
#endif
//#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( PERIPHERAL_CFG | CENTRAL_CFG ) )

        status = Pairing_start();
            if(status != SUCCESS)
            {
                // TODO: Call Error Handler
            }
        status = Connection_start();
           if(status != SUCCESS)
           {
               // TODO: Call Error Handler
           }
       status = Data_start();
           if ( status != SUCCESS )
           {
           // TODO: Call Error Handler
           }
       status = DevInfo_start();
           if ( status != SUCCESS )
           {
           // TODO: Call Error Handler
           }
       status = DataStream_start();

           if ( status != SUCCESS )
           {
           // TODO: Call Error Handler
           }
#if defined( HOST_CONFIG ) && ( HOST_CONFIG & ( PERIPHERAL_CFG | CENTRAL_CFG ))  &&  defined(OAD_CFG)
       status =  OAD_start();
            if(status != SUCCESS)
            {
                // TODO: Call Error Handler
            }
#endif
}

/*********************************************************************
 * @fn      SleepMode_callback
 *
 * @brief   This function handles the device mode - sleep/wakup as per gpio state
 *
 * @return  none
 */
void switchRole_callback(uint_least8_t index){
    Power_reset();
}

/*********************************************************************
 * @fn      appMain
 *
 * @brief   Application main function
 *
 * @return  none
 */
void appMain(void)
{
    Config_init();
    GPIO_init();

    GPIO_setCallback(GPIO_BLE_ROLE, switchRole_callback);
    GPIO_enableInt(GPIO_BLE_ROLE);

    /* Create a UART in CALLBACK read mode */
    UART2_Params_init(&uartParams);
    uartParams.readMode     = UART2_Mode_CALLBACK;
    uartParams.readCallback = callbackFxn;
    uartParams.baudRate     = 38400;

    uart = UART2_open(0, &uartParams);
    if (uart == NULL)
        {
            /* UART2_open() failed */
            while (1) {}
        }

    if(swapedConfigData.bleRole == 0xBB){
        appMainPeriCentParams.gapBondParams->pairMode = GAPBOND_PAIRING_MODE_NO_PAIRING;
        appMainPeriCentParams.gapBondParams->mitm = false;
        appMainPeriCentParams.gapBondParams->ioCap = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
    }
    else if(swapedConfigData.bleRole == 0xAA){
        appMainPeriCentParams.gapBondParams->pairMode = GAPBOND_PAIRING_MODE_INITIATE;
        appMainPeriCentParams.gapBondParams->mitm = false;
        appMainPeriCentParams.gapBondParams->ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
    }
    // Call the BLEAppUtil module init function
    BLEAppUtil_init(&criticalErrorHandler, &App_StackInitDoneHandler,
                    &appMainParams, &appMainPeriCentParams);
}
