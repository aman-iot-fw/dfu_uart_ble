/******************************************************************************

@file  app_data.c

@brief This file contains the Data Stream application functionality.

Group: WCS, BTS
$Target Device: DEVICES $

******************************************************************************
$License: BSD3 2022 $
******************************************************************************
$Release Name: PACKAGE NAME $
$Release Date: PACKAGE RELEASE DATE $
*****************************************************************************/

//*****************************************************************************
//! Includes
//*****************************************************************************
#include <FreeRTOS.h>
#include <task.h>

#include <time.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART2.h>
#include <ti/bleapp/profiles/data_stream/data_stream_profile.h>
#include <ti/bleapp/ble_app_util/inc/bleapputil_api.h>
#include <app_main.h>
#include <ti/devices/DeviceFamily.h>
//#include "ti/bleapp/util/sw_update/sw_update.h"
//#include "ti/bleapp/profiles/oad/oad_profile.h"
//*****************************************************************************
//! Defines
//*****************************************************************************
#define DS_CCC_UPDATE_NOTIFICATION_ENABLED  1
//#define UART_MAX_READ_SIZE    (256) //modf

//*****************************************************************************
//! Globals
//*****************************************************************************

extern UART2_Handle uart;
extern uint16_t connHandle;
//UART2_Params uartParams;
// UART read buffer
uint8_t uartReadBuffer[UART_MAX_READ_SIZE] = {0};
static uint16_t BufferSize = 0;

extern struct ADV_CONT_HANDLER Adv_controlHandler;
extern uint8 peripheralAdvHandle_1;
extern configurableData_t nvsConfigData;
extern configurableData_t swapedConfigData;

extern uint8_t connectionFlag;
extern uint16_t conHandle;
extern int8_t rssiT;

//Timer Specific
int ConnTick = 0;
uint32_t thresHold=0;
uint8_t pinUpFlag=0;


//*****************************************************************************
//!LOCAL FUNCTIONS
//*****************************************************************************

static void DS_onCccUpdateCB( uint16 connHandle, uint16 pValue );
static void DS_incomingDataCB( uint16 connHandle, char *pValue, uint16 len );
static void DS_TerminateCB();
static void triggerPersistentBoot(void);

//
void delayMs(uint32_t ms);
 //#

//*****************************************************************************
//!APPLICATION CALLBACK
//*****************************************************************************
// Data Stream application callback function for incoming data
static DSP_cb_t ds_profileCB =
{
  DS_onCccUpdateCB,
  DS_incomingDataCB,
};

//*****************************************************************************
//! Functions
//*****************************************************************************
/*********************************************************************
 * @fn      DS_onCccUpdateCB
 *
 * @brief   Callback from Data_Stream_Profile indicating ccc update
 *
 * @param   cccUpdate - pointer to data structure used to store ccc update
 *
 * @return  SUCCESS or stack call status
 */
static void DS_onCccUpdateCB( uint16 connHandle, uint16 pValue )
{

}

//// Function for clock delay
//void delayMs(uint32_t ms)
//{
//    volatile uint32_t i;
//    for (i = 0; i < (ms * 1000); i++);
//}

//void DS_TimeOut()
//{
//    char timeout_msg[] = "TIME_OUT";
//    if(!pinUpFlag){
//        DSP_sendData( (uint8 *)timeout_msg, 8);
//    }
//    delayMs(1000); //2 second
//    conHandle=Connection_getConnhandle(0);
//    BLEAppUtil_disconnect(conHandle);
//}

/*********************************************************************
 * @fn      DS_incomingDataCB
 *
 * @brief   Callback from Data_Stream_Profile indicating incoming data
 *
 * @param   dataIn - pointer to data structure used to store incoming data
 *
 * @return  SUCCESS or stack call status
 */
static void DS_incomingDataCB( uint16 connHandle, char *pValue, uint16 len )
{
  bStatus_t status = SUCCESS;
  char dataOut[] = "Data size is too long";
//  char ackTimeOut[] = "Client busy, wait";
  char printData[len+1];
  uint16 i = 0;

  // The incoming data length was too large
  if ( len == 0 )
  {
    // Send error message over GATT notification
    status = DSP_sendData( (uint8 *)dataOut, sizeof( dataOut ) );
  }

  // New data received from peer device
  else
  {
    // Copy the incoming data to buffer before printing it
    memcpy (printData, pValue, len );
    printData[len] ='\0';

   //Write incoming data processing here
//  if(!GPIO_read(CONFIG_GPIO_INPUT_PIN)){
     UART2_write(uart,printData, len, NULL);
//  }
//  else{
//      DSP_sendData( (uint8 *)ackTimeOut, sizeof( ackTimeOut ) );
//  }
//    UART2_write(uart, "\n\r", 2, NULL);
  }
}

// Function to handle data validation and response to commands
uint8_t DataValidateHandler(uint8_t *data) {
    uint8_t initDfuCmd[]={0x02,0x0E,0x01,0x02,0xFA,0x01,0x01,0x2C,0xDD,0x03};
    if (memcmp(data, initDfuCmd, sizeof(initDfuCmd)) == 0)
    {
        return 1;
    }
    // Check if the header and manufacturer-specific byte match
    if (*(data + 0) == 0x02 && *(data + 2) == 0xBB && *(data + 3) == 0xCC ) {
        uint8_t dataLength = *(data + 4); // Get the data length
        uint8_t commandType = *(data + 1); // Get the command type
        // If the command type is a set command, simply return 1
        if ((commandType & 0x0F)) {
//            UART2_write(uart, "ret", 3, NULL);
            return 1;
        }

        // Handle get commands
        switch (commandType) {
            case 0x10: // Get firmware version
            {
                const char firmwareVersion[] = "0.17.15";
                UART2_write(uart, firmwareVersion, sizeof(firmwareVersion) - 1, NULL);
                break;
            }
            case 0x20: // Get RSSI
            {
                volatile uint8_t RSS=-rssiT;
                uint8_t third = RSS%10+48;
                uint8_t second = RSS/10%10+48;
                volatile uint8_t rssi[3] = {0x2D,second,third};
                UART2_write(uart, &rssi, sizeof(rssi), NULL);
                break;
            }
            case 0x30: // Get device name
            {
//              dataLength = Data[1];
                size_t nameLength = strcspn(nvsConfigData.deviceName, "\0");
                UART2_write(uart, &nvsConfigData.deviceName, nameLength, NULL);
                break;
            }
            case 0x40: // Get passkey
            {
//                dataLength = Data[1];
                UART2_write(uart, &nvsConfigData.passKey, sizeof(nvsConfigData.passKey), NULL);
                break;
            }
//            case 0x50: // Get timeout
//            {
//                UART2_write(uart, &nvsConfigData.timeOut, dataLength, NULL);
//                break;
//            }
            case 0x60: // Reset command
            {
                UART2_write(uart, "Initiating Reset...", sizeof("Initiating Reset..."), NULL);
                Power_reset();
                break;
            }
            case 0x70: // BLE role status command
            {
                uint8_t infoMsg[] = {0x02, 0xAA, 0xBB, 0xCC, 0x03}; // Original message
                uint8_t validByte = (uint8_t)(swapedConfigData.bleRole & 0xFF); // Take the least significant byte

                if (validByte == 0xFF) {
                    validByte = 0xAA;
                }

                infoMsg[1] = validByte;

                UART2_write(uart, infoMsg, sizeof(infoMsg), NULL);
                break;
            }
            default:
                // Unknown command type
                UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);  // Set up UART for the next data reception
                return 0;
        }
        UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);  // Set up UART for the next data receptio
        return 0; // Return 0 after successfully handling a get command
    }
    UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);  // Set up UART for the next data reception
    return 0;
}

//static void triggerPersistentBoot(void) {
//    // Invalidate the current image
//    SwUpdate_RevokeImage(INT_PRIMARY_SLOT);
//
//    // Reset the device
//    Power_reset();
//
//}


void sendUartToDataIn() {
    bStatus_t status = SUCCESS;
    uint16_t offset = 0;
    uint16_t mtuSize = 200;       // Default MTU size; can be updated dynamically if known
    uint16_t headerSize = 3;     // Approximate GATT header size
    uint16_t maxChunkSize = mtuSize - headerSize;
    uint16_t dataLength = BufferSize;  // Total data length
    attWriteReq_t req;
    char buffer[40];
//    UART2_write(uart, "InsideSendUart\n", strlen("InsideSendUart\n"), NULL);

//    if(Adv_controlHandler.connFlag == 0)
//    {
        //Validate & Parse Received data buffer for Adv
    if(DataValidateHandler((uint8 *)uartReadBuffer)) //If validate send
    {
        parseConfigData((uint8 *)uartReadBuffer);
        //Stop Advertisement to invoke callback & update the Adv buffer
        //Implement set and get commands
//            BLEAppUtil_advStop(peripheralAdvHandle_1);
    }
    if(connHandle == LL_CONNHANDLE_INVALID){
//        UART2_write(uart, "No device connected\n", strlen("No device connected\n"), NULL);
    }
//    if(connHandle != LL_CONNHANDLE_INVALID){
//        UART2_rxEnable(uart);
//    }
    else{

    // Loop until all data is sent
    while (offset < dataLength) {
        // Determine the chunk size for this iteration
        uint16_t chunkSize = (dataLength - offset > maxChunkSize) ? maxChunkSize : (dataLength - offset);
        bool chunkSent = false;

        // Retry sending the current chunk until successful
        while (!chunkSent) {
            // Allocate memory for the write request
            req.pValue = GATT_bm_alloc(0x0, ATT_WRITE_REQ, chunkSize, NULL);
            if (req.pValue != NULL) {
                // Set up the request details
                req.handle = 0x25;  // Set handle for DataIn characteristic
                req.len = chunkSize;
                req.sig = FALSE;
                req.cmd = TRUE;  // Using Write Command (Write without Response)

                // Copy data to the request buffer
                memcpy(req.pValue, uartReadBuffer + offset, chunkSize);

                // Attempt to send the data chunk
//                vTaskDelay(pdMS_TO_TICKS(5));
                status = GATT_WriteNoRsp(0x0, &req);
//                vTaskDelay(pdMS_TO_TICKS(10));

                if (status == SUCCESS) {
                    chunkSent = true;      // Exit retry loop on success
                    offset += chunkSize;   // Move to the next chunk
                    // Only clear sent chunk data in `uartReadBuffer` if desired, or keep it until the entire buffer is read
                } else {
                    // Free the allocated buffer on failure
                    GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            } else {
                // Memory allocation failed; log error and retry after delay
                UART2_write(uart, "Memory allocation failed\n", strlen("Memory allocation failed\n"), NULL);
                vTaskDelay(pdMS_TO_TICKS(50));  // Delay before retrying allocation
            }
        }
    }
    }

    // Reset UART read buffer for new data
    memset(uartReadBuffer, 0, UART_MAX_READ_SIZE);
    UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);  // Set up UART for the next data reception
}


void SendUARTOverBLE(){
    //In Adv. mode
    if(Adv_controlHandler.connFlag == 0)
    {
        //Validate & Parse Received data buffer for Adv
        if(DataValidateHandler((uint8 *)uartReadBuffer)) //If validate send
        {
            parseConfigData((uint8 *)uartReadBuffer);
            //Stop Advertisement to invoke callback & update the Adv buffer
            //Implement set and get commands
            BLEAppUtil_advStop(peripheralAdvHandle_1);
        }
    }
    else
    {
        if(DataValidateHandler((uint8 *)uartReadBuffer)){
            parseConfigData((uint8 *)uartReadBuffer);
            //Implement get data command
        }
        //Transparent mode
//        else if(!GPIO_read(CONFIG_GPIO_INPUT_PIN)){
        DSP_sendData( (uint8 *)uartReadBuffer, BufferSize );
//        }
    }

    UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);
}

/*
 *  ======== callbackFxn ========
 */
void callbackFxn(UART2_Handle handle, void *buffer, size_t count, void *userArg, int_fast16_t status)
{
    uint8_t pinState = GPIO_read(GPIO_BLE_ROLE);
    BufferSize = count;

    if(swapedConfigData.bleRole == 0xBB){
        BLEAppUtil_invokeFunctionNoData(sendUartToDataIn);
    }
    else{
    BLEAppUtil_invokeFunctionNoData(SendUARTOverBLE);
    }
}

//
//void timerCallback(LGPTimerLPF3_Handle lgptHandle, LGPTimerLPF3_IntMask interruptMask) {
//    ConnTick++;
//    thresHold=swapedConfigData.timeOut;
//    if (thresHold < 0 || thresHold > 300000) {
//        thresHold = B_APP_DEFAULT_CONN_TIMEOUT;
//    }
//
//    if(connectionFlag && GPIO_read(CONFIG_GPIO_INPUT_PIN)){
//        if(ConnTick>=thresHold)
//        {
////            LGPTimerLPF3_stop(hTimer);
//            BLEAppUtil_invokeFunctionNoData(DS_TimeOut);
//            ConnTick=0;
//        }
//    }
//    else{
//        pinUpFlag=1;
//        ConnTick=thresHold;
//    }
//}

/*********************************************************************
 * @fn      DataStream_start
 *
 * @brief   This function is called after stack initialization,
 *          the purpose of this function is to initialize and
 *          register the Data Stream profile.
 *
 * @return  SUCCESS or stack call status
 */
bStatus_t DataStream_start(void)
{
  bStatus_t status = SUCCESS;

  status = DSP_start( &ds_profileCB );
  if( status != SUCCESS )
  {
    // Return status value
    return status;
  }
//  /* Create a UART in CALLBACK read mode */
//  UART2_Params_init(&uartParams);
//  uartParams.readMode     = UART2_Mode_CALLBACK;
//  uartParams.readCallback = callbackFxn;
//  uartParams.baudRate     = 115200;
//
//  uart = UART2_open(0, &uartParams);
//  if (uart == NULL)
//      {
//          /* UART2_open() failed */
//          while (1) {}
//      }
  UART2_read(uart, uartReadBuffer, UART_MAX_READ_SIZE, NULL);

//  uint32_t counterTarget;
//   LGPTimerLPF3_Params params;
//   LGPTimerLPF3_Params_init(&params);
//   params.hwiCallbackFxn = timerCallback;
//   hTimer = LGPTimerLPF3_open(CONFIG_LGPTIMER_3, &params);
//   if(hTimer == NULL) {
//     //Log_error0("Failed to open LGPTimer");
//     //Task_exit();
//   }
//   counterTarget = 48000-1;  // 1 ms with a system clock of 48 MHz
//   LGPTimerLPF3_setInitialCounterTarget(hTimer, counterTarget, true);
//   LGPTimerLPF3_enableInterrupt(hTimer, LGPTimerLPF3_INT_TGT);

//   GPIO_write(CONFIG_GPIO_OUTPUT_PIN,0);
//   GPIO_write(LED_PIN,0);

  return ( SUCCESS );
}
