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
#include "timer_if.h"

// Common interface includes
#include "uart_if.h"
#include "uart.h"
#include "pin_mux_config.h"
#include "timer_if.h"
#include "timer.h"
#include "Adafruit_SSD1351.h"
#include "Adafruit_GFX.h"
#include "glcdfont.h"

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
// index 10 = mute; index 11 = last/enter
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

// char array of arrays to keep hold all the characters
char letters[10][4] = {
    {' ', ' ', ' ', ' '} ,
    {'J', 'K', 'L', '0'} ,
    {'A', 'B', 'C', '0'} ,
    {'D', 'E', 'F', '0'} ,
    {'G', 'H', 'I', '0'} ,
    {'M', 'N', 'O', '0'} ,
    {'P', 'Q', 'R', 'S'} ,
    {'T', 'U', 'V', '0'} ,
    {'W', 'X', 'Y', 'Z'},
    {'\n'}
};

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

// variables to keep track of character display per button press
int char_i;
int prev_button;

//array to compare received signals to bit patterns
int checker[12] = {};

// variables to get bits from input
volatile unsigned char cNext = 0;
volatile unsigned long start;
volatile unsigned char done;
volatile unsigned char t;
volatile int xcd = 1;
volatile int ycd = 2;
int s;
int button;
int i;


// character buffer for holding characters of message to be sent
volatile char message[100];
volatile int message_i;
volatile int xcd2 = 1;
volatile int ycd2 = 68;

/************************ MACROS *********************************************/
#define SPI_IF_BIT_RATE  100000

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

// an example of how you can use structs to organize your pin settings for easier maintenance


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static void BoardInit(void);
static void print_button(void);
static void print_char(void);   
static void uartHandler(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
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
BoardInit(void) {
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);

    // Enable Processor
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

void print_button(void) {
    int a, b, click;

    for(a = 0; a < 25; a++) {
        for(b = 0; b < 12; b++) {
            if(checker[b] != patterns[a][b]) {      // not the current button
                break;
            }
            else if(b == 11) {                      // pattern matched
                click = a / 2;
                if(click == 10)                     // mute
                    button = 10;
                else if(click == 11)
                    button = 11;                    // last
                // button 5 doesn't work; use 1 to replace 5's numbers
                else if(click == 0 || click == 1 || click ==  2 || click == 3 || click == 4 || click == 5 ||
                        click == 6 || click == 7 || click == 8 || click == 9) {
                    if(click >= 5)
                        button = click - 1;                 // re-map all buttons greater than 5 to button number one less
                    else
                        button = click;
                    char_i++;
                    // index 6 == button 7, index 8 = button 9
                    // case where button represents 4 characters
                    // reset index when prev button pressed is not the same as current button
                    if(button != prev_button) {
                        char_i = 0;
                    }
                    else if(char_i == 3 && button != 6 && button != 8) {
                        char_i = 0;                // reset back to 0 index because we need to print first char of that button again
                    }
                    else if(char_i == 4) {
                        char_i = 0;
                    }
                }
                else
                    Report("Please press button again. \n\r");
                click = 0;
                t = 1;
                break;
            }
        }
        //want to stop searching through the bit patterns if a match is found
        if(t == 1) {
            prev_button = button;
            break;
        }
    }
}

void print_char() {
    char cChar, t_char;
    int i;

    cChar = letters[button][char_i];
    MAP_UARTCharPut(UARTA1_BASE, cChar);

    // if last pressed
    if(button == 11) 
    {
        message[message_i]= cChar;
        message_i++;
        message_i -= 2;
        xcd += 24;
        drawChar(xcd, ycd,' ',MAJENTA,BLACK,4);
    }

    //print the characters in the buffer if mute is pressed
    else if(button == 10) 
    {
        Report("\n\r");
        message[message_i] = ' ';
        fillRect(0, 0, 128, 60, BLACK);
        for(i = 0; i < message_i; i++) {
            t_char = message[i];
            MAP_UARTCharPut(UARTA1_BASE, t_char);
        }
        xcd = 1;
        ycd = 2;
        message_i = 0;
    }
    else 
    {
        if(xcd < 128)
        {
            xcd += 24;
        }
        else
        {
            xcd = 1;
            ycd += 32;
        }
    }

}

// GPIO interrupt handler
static void GPIOA0IntHandler(void) { // pin 61 handler
    unsigned long ulStatus;

    pin_intflag = 1;

    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);        // clear interrupts on GPIOA1

    start = TimerValueGet(TIMERA0_BASE, TIMER_A);
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);

    // if pulse length is greater than threshold for bit 1 but less than threshold for special character
    // and if we've seen two interrupts
    if (start > 200000 && start < 7000000 && s == 2) {
        checker[i]=1;
        i++;
    }
    // if pulse length is smaller than threshold for 1
    // and if we've seen two interrupts
    else if (start < 200000 && s == 2) {
        checker[i]=0;
        i++;
    }
    else if (start > 7000000) {         // special character
        s++;
        // if we've seen 3 interrupts
        // we are done setting the current button's bit pattern
        // reset values
        if (s == 3) {
            done = 1;
            i = 0;
            s = 0;
        }
    }
}

static void uartHandler(void){

    unsigned long ulStatus;

    ulStatus = MAP_UARTIntStatus(UARTA1_BASE, true);
    MAP_UARTIntClear(UARTA1_BASE, ulStatus);

    while(1)
    {
        message[message_i] = MAP_UARTCharGet(UARTA1_BASE);
        message_i ++;
    }
    if(message[message_i - 1] == '\n'){
        pin_intflag = 1;
    }
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

int main() {
    unsigned long ulStatus;

    BoardInit();

    PinMuxConfig();

    MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                    UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                    UART_CONFIG_PAR_NONE));

    UARTEnable(UARTA1_BASE);

    UARTFIFODisable(UARTA1_BASE);

    UARTIntRegister(UARTA1_BASE, uartHandler);

    UARTIntEnable(UARTA1_BASE, UART_INT_RX);

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

    InitTerm();
    ClearTerm();
    start = 0;

    // Configuring the timers
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 255);

    // Setup the interrupts for the timer timeouts.
    //Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerBaseIntHandler);

    // Turn on the timers feeding values in mSec
    Timer_IF_Start(TIMERA0_BASE, TIMER_A, 4000000000);

    // Register the interrupt handlers
    MAP_GPIOIntRegister(GPIOA0_BASE, GPIOA0IntHandler);

    // Configure rising edge interrupts
    MAP_GPIOIntTypeSet(GPIOA0_BASE, 0x40, GPIO_FALLING_EDGE);

    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);            // clear interrupts on GPIOA1

    pin_intflag=0;
    pin_intcount=0;

    done = 0;
    i = 0;
    t = 0;
    s = 1;
    char_i = -1;
    prev_button = -1;
    message_i = 0;

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
            int temp;
            fillScreen(BLACK);
            for (temp = 0; message[temp]!=''; temp++)
            {
                drawChar(xcd2, ycd2 , message[temp], YELLOW, 1, 4);
                if(xcd2 < 128)
                    {
                        xcd2 += 8;
                    }
                else
                    { 
                        xcd2 = 1;
                        ycd2 += 8;
                    }
            }
            xcd2 = 1;
            ycd2 = 68;
            message[message_i-1] = 0;
            message_i[message_i] = 0;
            message_i = 0;
        }
        if(done) {
            print_button();
            print_char();
            cNext=print_char(print_button());
            drawChar(xcd, ycd, cNext, WHITE,1,1);
            done = 0;
            t = 0;
            i = 0;
            if(message[1] != NULL) {
                Report("\r\n");
                Report("%c", &message[1]);
            }
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

