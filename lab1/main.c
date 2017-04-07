
// Standard includes
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"

// Common interface includes
#include "gpio_if.h"

#include "pin_mux_config.h"

#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME                "GPIO"

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
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
void LEDBlinkyRoutine();
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//*****************************************************************************

//*****************************************************************************
//
//! Configures the pins as GPIOs and peroidically toggles the lines
//!
//! \param None
//!
//! This function
//!    1. Configures 3 lines connected to LEDs as GPIO
//!    2. Sets up the GPIO pins as output
//!    3. Periodically toggles each LED one by one by toggling the GPIO line
//!
//! \return None
//
//*****************************************************************************
void LEDBlinkyRoutine()
{
    //
    // Toggle the lines initially to turn off the LEDs.
    // The values driven are as required by the LEDs on the LP.
    //
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    while(1)
    {
        //
        // Alternately toggle hi-low each of the GPIOs
        // to switch the corresponding LED on/off.
        //
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
    }

}
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

static void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t        CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t        Push SW3 to start LED binary counting       \n\r");
    Report("\t\t        Push SW2 to blink LEDs on and off       \n\r");
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

void blinkall()
{

    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    MAP_UtilsDelay(8000000);
    GPIO_IF_LedOn(MCU_ALL_LED_IND);
    MAP_UtilsDelay(8000000);


}

void countbinary()
{
    // first bit = red light
    // second bit = orange light
    // third bit = green light


    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    // 001
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    MAP_UtilsDelay(8000000);

    // 010
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    MAP_UtilsDelay(8000000);

    // 011
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    MAP_UtilsDelay(8000000);

    // 100
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    MAP_UtilsDelay(8000000);

    // 101
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    MAP_UtilsDelay(8000000);

    // 110
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    MAP_UtilsDelay(8000000);

    // 111
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    MAP_UtilsDelay(8000000);

    // 000
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
    MAP_UtilsDelay(8000000);

}

//****************************************************************************
//
//! Main function
//!
//! \param none
//!
//! This function
//!    1. Invokes the LEDBlinkyTask
//!
//! \return None.
//
//****************************************************************************
int
main()
{
    //
    // Initialize Board configurations
    //
    BoardInit();
    
    //
    // Power on the corresponding GPIO port B for 9,10,11.
    // Set up the GPIO lines to mode 0 (GPIO)
    //
    PinMuxConfig();
    DisplayBanner(APP_NAME);

    InitTerm();
    ClearTerm();

    long int sw3=0;
    long int sw2=0;
    int flag = 0;

    GPIO_IF_LedConfigure(LED1|LED2|LED3);

    while(1)
    {
        GPIO_IF_LedOff(MCU_ALL_LED_IND);
        sw3=GPIOPinRead(GPIOA1_BASE, 0x20);
        sw2=GPIOPinRead(GPIOA2_BASE, 0x40);

        if (sw3 && flag !=1)
        {
            Message("SW3 pressed \n");
            // Configure PIN_18 to be low
            GPIOPinWrite(GPIOA3_BASE, 0x10, 0x00);
            flag = 1;

        }
        if (sw2 && flag !=2 )
        {
            Message("SW2 pressed \n");
            // Configure PIN_18 to be high
            GPIOPinWrite(GPIOA3_BASE, 0x10, 0x10);
            flag = 2;
        }
        if (flag==1)
        {
            countbinary();
        }
        if (flag==2)
        {
            blinkall();
        }
        sw3=0;
        sw2=0;

    }

    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    //
    // Start the LEDBlinkyRoutine
    //
    //LEDBlinkyRoutine();
    return 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************


