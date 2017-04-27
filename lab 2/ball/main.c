// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

// Common interface includes
#include "uart_if.h"
#include "i2c_if.h"

#include "pinmux.h"

#include "spi.h"
#include "Adafruit_SSD1351.h"
#include "Adafruit_GFX.h"
#include "glcdfont.h"



//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME                "I2C Demo"
#define UART_PRINT              Report
#define FOREVER                 1
#define CONSOLE                 UARTA0_BASE
#define FAILURE                 -1
#define SUCCESS                 0
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}


#define TR_BUFF_SIZE     100
#define SPI_IF_BIT_RATE  100000

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//! Main function handling the I2C example
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************

static void
DisplayBanner()
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t      CC3200 I2C Application       \n\r");
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}




void main()
{
    //Position variables
    int xPos, yPos;

    //velocity
    float v = 128/10;

    //Read data
    signed char xData, yData;

    //Register variables
    unsigned char ucDevAddr, xRegOffset, yRegOffset;
    unsigned char aucDataBuf[2];

    // Initialize board configurations
    BoardInit();

    // Configure the pinmux settings for the peripherals exercised
    PinMuxConfig();

    // I2C Init
    I2C_IF_Open(I2C_MASTER_MODE_FST);

    // Enable the SPI module clock
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    // Reset the peripheral
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    // Reset SPI
    MAP_SPIReset(GSPI_BASE);

    //initialize modules, configure SPI
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    // Enable SPI for communication
    MAP_SPIEnable(GSPI_BASE);
    SPICSEnable(GSPI_BASE);

    // Initialize Adafruit OLED configs
    Adafruit_Init();
    //DisplayBanner();

    SPICSDisable(GSPI_BASE);

    xPos = 128/2;
    yPos = 128/2;

    ucDevAddr = 0x18;
    xRegOffset = 0x3;
    yRegOffset = 0x5;

    //clear board
    fillScreen(BLACK);

    //create circle
    fillCircle(xPos, yPos, 2,YELLOW);

    while(1)
    {

      I2C_IF_Write(ucDevAddr,&xRegOffset,1,0);

      I2C_IF_Read(ucDevAddr, &aucDataBuf[0], 1);

      yData = aucDataBuf[0];

      I2C_IF_Write(ucDevAddr,&yRegOffset,1,0);

      I2C_IF_Read(ucDevAddr, &aucDataBuf[0], 1);

      xData = aucDataBuf[0];

      fillCircle(xPos, yPos, 2,BLACK);

      xPos += ((int)xData * v / 255);
      if(xPos > 124) {
          xPos = 124;
      }
      else if(xPos < 4) {
          xPos = 4;
      }

      yPos += ((int)yData * v / 255);
      if(yPos > 124) {
          yPos = 124;
      }
      else if(yPos < 4) {
          yPos = 4;
      }

      fillCircle(xPos, yPos, 2, YELLOW); 
    }
}

