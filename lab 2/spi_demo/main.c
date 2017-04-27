// Standard includes
#include <string.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"
#include "test.h"

// Common interface includes
#include "uart_if.h"
#include "pin_mux_config.h"


//#include "Adafruit_OLED.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "test.h"
#include "glcdfont.h"

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define GREEN           0x07E0
#define CYAN            0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF


#define APPLICATION_VERSION     "1.1.1"
//*****************************************************************************
//
// Application Master/Slave mode selector macro
//
// MASTER_MODE = 1 : Application in master mode
// MASTER_MODE = 0 : Application in slave mode
//
//*****************************************************************************
#define MASTER_MODE      0

#define SPI_IF_BIT_RATE  1000000
#define TR_BUFF_SIZE     100

#define MASTER_MSG       "This is CC3200 SPI Master Application\n\r"
#define SLAVE_MSG        "This is CC3200 SPI Slave Application\n\r"

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
//! Main function for spi demo application
//!
//! \param none
//!
//! \return None.
//
//*****************************************************************************
void main()
{
    char *string = "Hello world!";
    int i;
    //int len;
    //
    // Initialize Board configurations
    //
    BoardInit();

    //
    // Muxing UART and SPI lines.
    //
    PinMuxConfig();

    //
    // Enable the SPI module clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);


    // Initialising the Terminal.
    InitTerm();

    // Clearing the Terminal.
    ClearTerm();

    // Display the Banner
    Message("\n\n\n\r");
    Message("\t\t   ********************************************\n\r");
    Message("\t\t        CC3200 OLED Display TEST \n\r");
    Message("\t\t   ********************************************\n\r");
    Message("\n\n\n\r");

    //
    // Reset the peripheral
    //
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    //
    // Configure SPI Interface
    //

    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                           SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_3,
                           (SPI_SW_CTRL_CS |
                            SPI_4PIN_MODE |
                            SPI_TURBO_OFF |
                            SPI_CS_ACTIVEHIGH |
                            SPI_WL_8));

    //Enable SPI for communication
    MAP_SPIEnable(GSPI_BASE);

    Adafruit_Init();

    //Print the full character-set found in the font table in glcdfont.c.
    fillScreen(BLACK);
    //len = (sizeof(font)\sizeof(font[0]));
    for(i = 0; i < 50; i++)
    {
        drawChar(1,1,font[i], WHITE, 1, 1);
        delay(20);
    }
    delay(200);

    //Print the string “Hello world!” to the display
    fillScreen(BLACK);
    Outstr(string);
    delay(300);

    //Display a pattern of 8 bands of different colors horizontally across the full OLED display.
    lcdTestPattern();
    delay(200);

    //Display a pattern of 8 bands of different colors vertically across the full OLED display.
    lcdTestPattern2();
    delay(200);

    //Call the testlines() function to display diagonal lines.
    testlines(MAGENTA);
    delay(200);

    //Call the testfastlines() function to display a rectangular grid pattern.
    testfastlines(MAGENTA, YELLOW);
    delay(200);

    //Call the testdrawrects() function to draw a pattern of rectangles.
    testdrawrects(YELLOW);
    delay(200);

    //Call the testfillrects() function.
    testfillrects(MAGENTA, YELLOW);
    delay(200);

    //Call the testfillcircles() and testdrawcircles() functions.
    testfillcircles(4, YELLOW);
    delay(200);
    testdrawcircles(4, YELLOW);
    delay(200);

    //Call the testroundrects() function.
    testroundrects();
    delay(200);

    //Call the testtriangles() function
    testtriangles();
    delay(200);

}

