
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
#include "timer_if.h"

// Common interface includes
#include "uart_if.h"

#include "pin_mux_config.h"
#include "timer_if.h"
#include "timer.h"


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

static volatile unsigned long g_ulSysTickValue;
static volatile unsigned long g_ulBase;
static volatile unsigned long g_ulRefBase;
static volatile unsigned long g_ulRefTimerInts = 0;
static volatile unsigned long g_ulIntClearVector;
unsigned long g_ulTimerInts;
int temp;

// button bit patterns: stored in an array of arrays
// index is the button number
// index 10 = enter; index 11 = mute
int patterns[25][12] = {
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0} ,
    {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0} ,
    {0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0} ,
    {0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0} ,
    {0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0}
};

//array to compare received signals to bit patterns
int checker[12] = {};

// variables to get bits from input
volatile unsigned char high_flag;
volatile unsigned long delta;
volatile unsigned long start;
volatile unsigned long end;
volatile unsigned long time;
volatile unsigned char done;
int count;
int i;
int interrupts;
volatile unsigned char special;
volatile unsigned char t;
int s;
//volatile unsigned char bit_flag;

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

// an example of how you can use structs to organize your pin settings for easier maintenance


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static void BoardInit(void);
static void print_button(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//*****************************************************************************

void print_button(void) {
    int a, b, click;

    for(a = 0; a < 25; a++) {
        for(b = 0; b < 12; b++) {
            if(checker[b] != patterns[a][b]) {      // not the current button
                break;
            }
            else if(b == 11) {                      // pattern matched
                //Report("checker: %d, pattern: %d \n\r", checker[b], patterns[a][b]);
                click = a / 2;
                if(click == 10)
                    Report("Mute pressed. \n\r");
                else if(click == 11)
                    Report("Last pressed. \n\r");
                else if(click == 0 || click == 1 || click ==  2 || click == 3 || click == 4 || click == 5 || click == 6 ||
                        click == 7 || click == 8 || click == 9)
                    Report("%d pressed. \n\r", click);
                else
                    Report("Please press button again. \n\r");
                click = 0;
                t = 1;
                break;
            }
        }
        if(t == 1)
            break;
    }
}

static void GPIOA0IntHandler(void) { // pin 61 handler
    unsigned long ulStatus;

    pin_intflag = 1;

    //interrupts++;

    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);        // clear interrupts on GPIOA1

    start = TimerValueGet(TIMERA0_BASE, TIMER_A);
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);
    //Report("start: %d \n\r", start);
    if (start > 200000 && start < 7000000 && s == 2) {
        checker[i]=1;
        i++;
    }
    else if (start < 200000 && s == 2) {
        checker[i]=0;
        i++;
    }
    else if (start > 7000000) {         // special character
        s++;
        if(s == 3) {
            done = 1;
            i = 0;
            s = 0;
        }
    }
//    if(count == 0) {
//        start = TimerValueGet(TIMERA0_BASE, TIMER_A);
//        count++;
//    }
//    else if(count == 1) {
//        end = TimerValueGet(TIMERA0_BASE, TIMER_A);
//        delta = end - start - 4294800000;
//        t = end;
//        start = t;
//        TimerLoadSet(TIMERA0_BASE, TIMER_A, 0x0);
////        start = TimerValueGet(TIMERA0_BASE, TIMER_A);
//
//        if(delta > 7000000 && s == 0) {       // special character
//            special = 1;
//            s++;
//        }
//
////        else if(delta > 7000000 && s == 2) {
////            done = 1;
////            s = 0;
////            Report("special2");
////        }
//
//        else if(special == 1 && s == 1 && i == 17) {       // start reading pattern
//            if(delta > 100000 && delta < 1000000) {         // 1
//                checker[i] = 1;
//                Report("1");
//                i++;
//            }
//            else if(delta < 100000 && delta > 1000) {       // 0
//                checker[i] = 0;
//                Report("0");
//                i++;
//            }
//        }
//    }
}

void reset(void) {
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0x0);
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

//void printpattern(unsigned long int pattern) {
//
//    if (pattern == b1) {
//        Report("Button 1 was pressed.");
//    }
//    if (pattern == b2) {
//           Report("Button 2 was pressed.");
//       }
//    if (pattern == b3) {
//           Report("Button 3 was pressed.");
//       }
//    if (pattern == b4) {
//           Report("Button 4 was pressed.");
//       }
//    if (pattern == b5) {
//           Report("Button 5 was pressed.");
//       }
//    if (pattern == b6) {
//           Report("Button 6 was pressed.");
//       }
//    if (pattern == b7) {
//           Report("Button 7 was pressed.");
//       }
//    if (pattern == b8) {
//           Report("Button 8 was pressed.");
//       }
//    if (pattern == b9) {
//           Report("Button 9 was pressed.");
//       }
//    if (pattern == mute) {
//           Report("Button mute (delete) was pressed.");
//       }
//    if (pattern == enter) {
//           Report("Button enter was pressed.");
//       }
//}


int main() {
    unsigned long ulStatus;
    unsigned long g_ulBase;

    int c;

    BoardInit();

    PinMuxConfig();

    InitTerm();

    ClearTerm();

    delta = 0;
    start = 0;
    end = 0;
    count = 0;

    // Configuring the timers
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 255);

    // Turn on the timers feeding values in mSec
    Timer_IF_Start(TIMERA0_BASE, TIMER_A, 4000000000);

    // Register the interrupt handlers
    MAP_GPIOIntRegister(GPIOA0_BASE, GPIOA0IntHandler);

    // Configure rising edge interrupts
    MAP_GPIOIntTypeSet(GPIOA0_BASE, 0x40, GPIO_FALLING_EDGE);
    //MAP_GPIOIntTypeSet(GPIOA0_BASE, 0x40, GPIO_RISING_EDGE);

    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);            // clear interrupts on GPIOA1

    pin_intflag=0;
    pin_intcount=0;

    //high_flag = 1;
    done = 0;
    i = 0;
    t = 0;
    s = 1;
    //interrupts = 0;

    // Enable pin 61 interrupt
    MAP_GPIOIntEnable(GPIOA0_BASE, 0x40);

    Message("\t\t****************************************************\n\r");
    Message("\t\t\tRemote Button Pressed\n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");

    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);

    while (1) {
            while (pin_intflag==0) {;}
            if (pin_intflag) {
                pin_intflag=0;
                //Report("\t delta = %lu \n", delta);
                //TimerValueSet(TIMERA0_BASE, TIMER_A, 0x0);
            }
            if(done) {
                print_button();
                //Report("\r\n");
                done = 0;
                t = 0;
                //high_flag = 1;
                i = 0;
//                //interrupts = 0;
            }
        }
}
