//*****************************************************************************
//
// Application Name     - int_sw
// Application Overview - The objective of this application is to demonstrate
//                          GPIO interrupts using SW2 and SW3.
//                          NOTE: the switches are not debounced!
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup int_sw
//! @{
//
//****************************************************************************

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
#include "uart_if.h"

#include "pin_mux_config.h"
#include "timer_if.h"


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern void (* const g_pfnVectors[])(void);

volatile unsigned long SW2_intcount;
volatile unsigned long SW3_intcount;
volatile unsigned char SW2_intflag;
volatile unsigned char SW3_intflag;

volatile unsigned long pin_intcount;
volatile unsigned char pin_intflag;

unsigned long int b0;
unsigned long int b1;
unsigned long int b2;
unsigned long int b3;
unsigned long int b4;
unsigned long int b5;
unsigned long int b6;
unsigned long int b7;
unsigned long int b8;
unsigned long int b9;
unsigned long int mute;
unsigned long int pattern;
unsigned long int enter;
unsigned long int time;


//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

// an example of how you can use structs to organize your pin settings for easier maintenance
typedef struct PinSetting {
    unsigned long port;
    unsigned int pin;
} PinSetting;


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************
static void GPIOA0IntHandler(void) { // pin 61 handler
    unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);        // clear interrupts on GPIOA1
    //pin_intcount++;
    //pin_intflag=1;

    time = TimerValueGet(TIMERA0_BASE,TIMER_A);
    Report(time);
    //TimerValueSet ()

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
BoardInit(void) {
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
    
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}
//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//!
//! \return None.
//
//****************************************************************************

void printpattern(unsigned long int pattern) {

    b0 = 0010000000000;
    b1 = 010000000001;
    b2 = 010000000011;
    b3 = 010000000010;
    b4 = 001000000110;
    b5 = 001000000111;
    b6 = 001000000101;
    b7 = 010000000100;
    b8 = 010000001100;
    b9 = 010000001101;
    mute = 0010000001011;
    enter = 1110000011100;

    if (pattern == b0) {
        Report("Button 0 was pressed.");
    }
    if (pattern == b1) {
        Report("Button 1 was pressed.");
    }
    if (pattern == b2) {
           Report("Button 2 was pressed.");
       }
    if (pattern == b3) {
           Report("Button 3 was pressed.");
       }
    if (pattern == b4) {
           Report("Button 4 was pressed.");
       }
    if (pattern == b5) {
           Report("Button 5 was pressed.");
       }
    if (pattern == b6) {
           Report("Button 6 was pressed.");
       }
    if (pattern == b7) {
           Report("Button 7 was pressed.");
       }
    if (pattern == b8) {
           Report("Button 8 was pressed.");
       }
    if (pattern == b9) {
           Report("Button 9 was pressed.");
       }
    if (pattern == mute) {
           Report("Button mute (delete) was pressed.");
       }
    if (pattern == enter) {
           Report("Button enter was pressed.");
       }
}

//*****************************************************************************
//
//! The interrupt handler for the first timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
void
TimerBaseIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(TIMERA0_BASE);
}

int main() {
    unsigned long ulStatus;

    BoardInit();
    
    PinMuxConfig();
    
    InitTerm();

    ClearTerm();

    // Register the interrupt handlers
    MAP_GPIOIntRegister(GPIOA0_BASE, GPIOA0IntHandler);

    // Configure rising edge interrupts
    MAP_GPIOIntTypeSet(GPIOA0_BASE, 0x40, GPIO_RISING_EDGE);

    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, false);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);            // clear interrupts on GPIOA0

    pin_intflag=0;
    pin_intcount=0;
    //high_flag = 0;

    // Enable pin 61 interrupt
    MAP_GPIOIntEnable(GPIOA0_BASE, 0x40);

    // Configuring the timers
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);

    // Setup the interrupts for the timer timeouts.
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_A, TimerBaseIntHandler);

    // Turn on the timers feeding values in mSec
    Timer_IF_Start(TIMERA0_BASE, TIMER_A, 500);


}
