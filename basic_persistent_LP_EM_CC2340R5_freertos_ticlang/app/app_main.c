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
#include <ti/bleapp/profiles/oad/oad_profile.h>
#include <app_main.h>
#include <ti/drivers/UART2.h>
#include "ti/bleapp/util/sw_update/sw_update.h"

#include "flash_interface.h"
#include <ti_drivers_config.h>
#include <ti/drivers/NVS.h>


//*****************************************************************************
//! Defines
//*****************************************************************************
#define UART_BUFFER_SIZE 1024
#define UART_FLASH_START_ADDRESS 0x0
#define POLYNOMIAL 40961

//*****************************************************************************
//! Globals
//*****************************************************************************
UART2_Handle uart;
extern NVS_Handle nvsHandle;
static uint8_t uartBuffer[UART_BUFFER_SIZE];
static uint16_t BufferSize = 0;
static uint32_t writeAddress = UART_FLASH_START_ADDRESS;
static uint16_t expectedPacketNumber = 1;
static uint8_t remActPckt = 0xFF;
uint8_t flag=0;

// Check if the received data matches the expected response
/*uint8_t expectedResponse[] = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x70, 0x00, 0x2B, 0x61, 0x29, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85,
    0x74, 0x05, 0x08, 0x01, 0x01, 0xA2, 0x03, 0x02, 0x01, 0x00, 0xA3, 0x05, 0xA1, 0x03, 0x02, 0x01,
    0x00, 0xBE, 0x10, 0x04, 0x0E, 0x08, 0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00, 0x00, 0x10, 0x10, 0x02,
    0x8A, 0x00, 0x07
};*/
uint8_t expectedResponse[] = {0x02, 0x00, 0x04, 0xAA, 0x00, 0x00, 0x03};
uint8_t initiateCmd[] = {0x02, 0x0E, 0x01, 0x02, 0xFA, 0x01, 0x01, 0x2C, 0xDD, 0x03};

// Parameters that should be given as input to the BLEAppUtil_init function
BLEAppUtil_GeneralParams_t appMainParams =
{
    .taskPriority = 1,
    .taskStackSize = 1024,
    .profileRole = (BLEAppUtil_Profile_Roles_e)HOST_CONFIG,
    .addressMode = DEFAULT_ADDRESS_MODE,
    .deviceNameAtt = attDeviceName,
    .pDeviceRandomAddress = pRandomAddress,
};

BLEAppUtil_PeriCentParams_t appMainPeriCentParams =
{
 .connParamUpdateDecision = DEFAULT_PARAM_UPDATE_REQ_DECISION,
 .gapBondParams = &gapBondParams
};

//*****************************************************************************
//! Functions
//*****************************************************************************

unsigned short CrcCalc(unsigned char *buffer, unsigned short NoOfBytes);

//Function to execute the firmware updation over UART.
void sendUartToDataIn() {
    if (BufferSize > 0)
       {
        //parse the coming pckt on uart
        uint8_t *data = (uint8_t *)uartBuffer;
        uint16_t packetNumber = (data[3] << 8) | data[4];
        uint16_t remainingPackets = (data[5] << 8) | data[6];
        uint16_t crcReceived = (data[263] << 8) | data[264];
        uint8_t *imageData = &data[7]; // Start of image data

        //Identify the initiate fw update command and response
        if (memcmp(uartBuffer, initiateCmd, sizeof(initiateCmd)) == 0)
        {
//            uint8_t imgDataCommand[] = {0x02,0x00,0x04,0xAA,0x00,0x00,0x03};
//            UART2_write(uart, imgDataCommand, sizeof(imgDataCommand), NULL);
//            flag = 0;
//            return;
            if(remActPckt == 0){
                uint8_t imgDataCommand[] = {0x02,0x00,0x04,0xCC,0x00,0x00,0x03};
                UART2_write(uart, imgDataCommand, sizeof(imgDataCommand), NULL);
            }
            Power_reset();
        }
        //Activate fw command response
        if (memcmp(uartBuffer, expectedResponse, sizeof(expectedResponse)) == 0)
        {
          // Send the next command to start receiving image data
           if(remActPckt == 0){
               uint8_t imgDataCommand[] = {0x02,0x00,0x04,0xAA,0x00,0x00,0x03};
               UART2_write(uart, imgDataCommand, sizeof(imgDataCommand), NULL);
               Power_reset();
          }
          else{
              uint8_t imgDataCommand[] = {0x02,0x00,0x04,0xCC,0x00,0x00,0x03};
              UART2_write(uart, imgDataCommand, sizeof(imgDataCommand), NULL);
              flag = 0;
              return;
          }
        }

/*          if (memcmp(uartBuffer, expectedResponse, sizeof(expectedResponse)) == 0)
          {
              // Send the next command to start receiving image data
              uint8_t imgDataCommand[] = {
                  0x00, 0x01, 0x00, 0x70, 0x00, 0x01, 0x00, 0x0D, 0xC0, 0x01, 0xC1, 0x00, 0x01, 0x01, 0x00, 0xBE,
                  0x01, 0x01, 0xFF, 0x02, 0x00
              };
              UART2_write(uart, imgDataCommand, sizeof(imgDataCommand), NULL);
              UART2_read(uart, uartBuffer, UART_BUFFER_SIZE, NULL);
              return;
          }

           uint8_t *data = (uint8_t *)uartBuffer;
           uint16_t packetNumber = (data[16] << 8) | data[17];
           uint16_t remainingPackets = (data[18] << 8) | data[19];
           uint16_t crcReceived = (data[20] << 8) | data[21];
           uint8_t *imageData = &data[22]; // Start of image data


           UART2_write(uart, data, 256, NULL);
           */

            //Check for packet number consistency
            if (packetNumber != expectedPacketNumber && data[0] == 0x02 && data[1] == 0x01 && data[2] == 0x07)
            {
                uint8_t errorMsg[] = {0x02,0x00,0x04,0xCC,0x00,0x00,0x03};
                UART2_write(uart, errorMsg, sizeof(errorMsg), NULL);
                UART2_read(uart, uartBuffer, UART_BUFFER_SIZE, NULL);
                flag = 0;
                return;
            }

          // CRC mismatched
           unsigned short crcHPL = CrcCalc((unsigned char*)imageData, 256);
           if(crcReceived != CrcCalc(imageData, 256)){
               uint8_t errorMsg[] = {0x02,0x00,0x04,0xCC,0x00,0x00,0x03};
               UART2_write(uart, errorMsg, sizeof(errorMsg), NULL);
               UART2_read(uart, uartBuffer, UART_BUFFER_SIZE, NULL);
               flag = 0;
               return;
           }

         //   Write image data to flash
           uint8_t flashStat = 0xFF;
          flag++;

          if(flag == 1){
          if(NVS_write(nvsHandle, writeAddress, imageData, 256, NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY)
                     == NVS_STATUS_SUCCESS)
          {
               uint8_t infoMsg[] = {0x02,0x00,0x04,0xAA,0x00,0x00,0x03};
               UART2_write(uart, infoMsg, sizeof(infoMsg), NULL);
               flashStat = SUCCESS;
          }
          if(flashStat != SUCCESS)
          {
                 uint8_t errorMsg[] = {0x02,0x00,0x04,0xCC,0x00,0x00,0x03};
                 UART2_write(uart, errorMsg, sizeof(errorMsg), NULL);
                 flag = 0;
          }

           }
           else if(flag <= 8){
               if(NVS_write(nvsHandle, writeAddress, imageData, 256, NVS_WRITE_POST_VERIFY)
                         == NVS_STATUS_SUCCESS)
               {
                   uint8_t infoMsg[] = {0x02,0x00,0x04,0xAA,0x00,0x00,0x03};
                   UART2_write(uart, infoMsg, sizeof(infoMsg), NULL);
                   flashStat = SUCCESS;
               }
              if(flashStat != SUCCESS)
              {
                     uint8_t errorMsg[] = {0x02,0x00,0x04,0xCC,0x00,0x00,0x03};
                     UART2_write(uart, errorMsg, sizeof(errorMsg), NULL);
              }
              if(flag == 8){
                  flag = 0;
              }
           }

          writeAddress+= 0x100;

          if(remainingPackets == 0 )
          {
//                uint8_t infoMsg[] = {0x02,0x00,0x04,0xAA,0x00,0x00,0x03};
//                UART2_write(uart, infoMsg, sizeof(infoMsg), NULL);
                SwUpdate_Close();
                remActPckt = 0;
//                Power_reset();
           }

           expectedPacketNumber++;
       }

       // Re-enable UART read callback
       UART2_read(uart, uartBuffer, UART_BUFFER_SIZE, NULL);
}
/*********************************************************************
 * @fn      uartCallback
 *
 * @brief   UART callback function to handle received data.
 *
 * @param   handle - UART handle
 *          buffer - Data buffer received
 *          size - Size of the received data
 *
 * @return  none
 */
void uartCallback(UART2_Handle handle, void *buffer, size_t size, void *userArg, int_fast16_t status)
{
    BufferSize = size;
    BLEAppUtil_invokeFunctionNoData(sendUartToDataIn);
}

/*********************************************************************
 * @fn      CrcCalc
 *
 * @brief   Calculate the CRC of coming img data on uart
 *
 * @return  none
 */
unsigned short CrcCalc(unsigned char *buffer, unsigned short NoOfBytes)
{
  unsigned short cc, i;
  unsigned short CRCD;
  CRCD = 0xFFFF; //init CRC value with 0xFFFF
  for (i = 0; i < NoOfBytes; i++)
  {
      CRCD ^= *buffer++;
    for (cc = 0; cc < 8; cc++)
    {
      if ((CRCD & 0x0001) == 1)
      {
          CRCD >>= 1;
          CRCD ^= POLYNOMIAL;
      }
      else
      {
          CRCD >>= 1;
      }
    }
  }
  return (CRCD);
}


/*********************************************************************
 * @fn      criticalErrorHandler
 *
 * @brief   Application task entry point
 *
 * @return  none
 */
void criticalErrorHandler(int32 errorCode , void* pInfo)
{
    // Reset the system on critical error
//    SystemReset();
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

    status = OADProfile_start(NULL);

    status = Connection_start();
    status = Peripheral_start();

    status = Pairing_start();

    if(status != SUCCESS)
    {
        // TODO: Call Error Handler for each call
    }
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

   /* status = */flash_open();
//    if(status != FLASH_OPEN)
//    {
//       return(NV_OPER_FAILED);
//    }

    // Initialize UART
    UART2_Params uartParams;
    UART2_Params_init(&uartParams);
    uartParams.readMode = UART2_Mode_CALLBACK;
    uartParams.writeMode = UART2_Mode_BLOCKING;
    uartParams.baudRate = 38400;
    uartParams.readCallback = uartCallback;

    uart = UART2_open(0, &uartParams);
    if (uart == NULL)
    {
        while (1); // UART initialization failed
    }

    // Old Command to indicate that ble is ready to receive fw

//    uint8_t command[] = {
//        0x00, 0x01, 0x00, 0x70, 0x00, 0x01, 0x00, 0x1F,
//        0x60, 0x1D, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85,
//        0x74, 0x05, 0x08, 0x01, 0x01, 0xBE, 0x10, 0x04,
//        0x0E, 0x01, 0x00, 0x00, 0x00, 0x06, 0x5F, 0x1F,
//        0x04, 0x00, 0x62, 0x1E, 0x5D, 0xFF, 0xFF
//    };
    uint8_t command[] = {0x02, 0x00, 0x04, 0xAA, 0x00, 0x00, 0x003};



    // Write the command to UART
    UART2_write(uart, command, sizeof(command), NULL);



    // Start UART read in callback mode
    UART2_read(uart, uartBuffer, UART_BUFFER_SIZE, NULL);

    // Call the BLEAppUtil module init function
    BLEAppUtil_init(&criticalErrorHandler, &App_StackInitDoneHandler,
                    &appMainParams, &appMainPeriCentParams);
}
