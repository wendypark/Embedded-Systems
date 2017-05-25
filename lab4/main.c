// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "timer.h"
#include "rom.h"
#include "rom_map.h"
#include "pin.h"
#include "spi.h"
#include <math.h>
#include <stdbool.h>

// Common interface includes
#include "uart_if.h"
#include "pin_mux_config.h"
#include "systick_if.h"

#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME                "Lab 4"
#define TIMER_FREQ      80000000
#define SPI_IF_BIT_RATE  400000

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
static unsigned long g_ulSamples[2];
static unsigned long g_ulFreq;

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

volatile int int_flag = 0;
unsigned long time = 0;
int sample_i = 0;

int N = 410;                        // block size
volatile int samples[410];       // buffer to store N samples
volatile int count;             // samples count
volatile unsigned long signal1;
volatile unsigned long signal2;
volatile unsigned int signalfinal;
volatile tBoolean flag;             // flag set when the samples buffer is full with N samples
volatile tBoolean new_dig;          // flag set when inter-digit interval (pause) is detected

// texting variables
char cChar;       // current character
char prev;
int num_press;
volatile int special;
static volatile unsigned long g_ulRefBase;
volatile int int_flag2 = 0;
int t_count;
int button = -1;
int sp_i = -1;

int power_all[8];               // array to store calculated power of 8 frequencies

int coeff[8] = {31549, 31282, 30952, 30558, 29147, 28365, 27414, 26264};                   // array to store the calculated coefficients
int f_tone[8]={697, 770, 852, 941, 1209, 1336, 1477, 1633}; // frequencies of rows & columns

// char array to keep the characters for 3 letter keys
char letters3[8][3] = {
                      {' ', ' ', ' '} ,
                      {' ', ' ', ' '} ,
                      {'A', 'B', 'C'} ,
                      {'D', 'E', 'F'} ,
                      {'G', 'H', 'I'} ,
                      {'J', 'K', 'L'} ,
                      {'M', 'N', 'O'} ,
                      {'T', 'U', 'V'}
};

// char array to keep the characters for 4 letter keys
char letters4[2][4] = {
                      {'P', 'Q', 'R', 'S'} ,
                      {'W', 'X', 'Y', 'Z'}
};

// int array to find the frequency of buttons
// gets rid of issue with multiple readings from short presses
int freq[16] = {-1};
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

//*****************************************************************************
//
//! Timer interrupt handler
//
//*****************************************************************************

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t\t  CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
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
void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
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

void timerhandler(void)
{
    // Reset SPI
    //MAP_SPIReset(GSPI_BASE);

    unsigned long status;

    //disable timer interrupt
    status = TimerIntStatus(TIMERA0_BASE, true);
    TimerIntClear(TIMERA0_BASE, TIMER_A);
    // (END)
    TimerIntDisable(TIMERA0_BASE, status);
   // TimerDisable(TIMERA0_BASE, TIMER_A);

    // (MAIN)
    // set ADC CS to high inactive
    //MAP_GPIOPinWrite(GPIOA2_BASE, 0x40, 0x40); //AD CS inactive
    //MAP_GPIOPinWrite(GPIOA0_BASE, 0x10, 0x10); //OLED CS inactive

    // Enable SPI CS for communication
    SPICSEnable(GSPI_BASE);


    GPIOPinWrite(GPIOA0_BASE, 0x80, 0xff); //OLED CS active
    GPIOPinWrite(GPIOA2_BASE, 0x40, 0x00);  //AD CS active

    // Send data into samples
    SPIDataPut(GSPI_BASE, 'x');
    SPIDataGet(GSPI_BASE, &signal1);
    SPIDataPut(GSPI_BASE, 'x');
    SPIDataGet(GSPI_BASE, &signal2);

    signal1 = (signal1 & 0x1f) << 5;
    signal2 = (signal2 & 0xf8) >> 3;
//    signalfinal = signal1 + signal2; //adding two bit values
    samples[count] = ((int)  (signal1 + signal2)) - 388;
    SPICSDisable(GSPI_BASE);



    // Disable SPI CS for communication
    // MAP_SPICSDisable(GSPI_BASE);

    // set ADC CS to high (inactive)
    //MAP_GPIOPinWrite(GPIOA2_BASE, 0x40, 0x40);

    if (count < N) {
        count++;
        int_flag = 1;
    }
    else if (count == N) {
        int_flag = 0;
        //TimerIntDisable(TIMERA0_BASE, status);
        //TimerDisable(TIMERA0_BASE, TIMER_A);
//        int i;
//        for (i=0; i < N; i++) {
//            Report(" %d \n\r", samples[i]);
//        }
//        i=0;
    }

    GPIOPinWrite(GPIOA2_BASE, 0x40, 0xff); //ADC High
    GPIOPinWrite(GPIOA0_BASE, 0x80, 0x00); //oled high

    TimerIntEnable(TIMERA0_BASE, TIMER_A);
    TimerLoadSet(TIMERA0_BASE, TIMER_A, 5000);
    TimerEnable(TIMERA0_BASE, TIMER_A);
}

//-------Goertzel function---------------------------------------//
long int goertzel(int sample[], long int coeff, int N)
//---------------------------------------------------------------//
{
//initialize variables to be used in the function
int Q, Q_prev, Q_prev2,i;
long prod1,prod2,prod3,power;

    Q_prev = 0;         //set delay element1 Q_prev as zero
    Q_prev2 = 0;        //set delay element2 Q_prev2 as zero
    power=0;            //set power as zero

    for (i=0; i<N; i++) // loop N times and calculate Q, Q_prev, Q_prev2 at each iteration
        {
            Q = (sample[i]) + ((coeff* Q_prev)>>14) - (Q_prev2); // >>14 used as the coeff was used in Q15 format
            Q_prev2 = Q_prev;                                    // shuffle delay elements
            Q_prev = Q;
        }

        //calculate the three products used to calculate power
        prod1=( (long) Q_prev*Q_prev);
        prod2=( (long) Q_prev2*Q_prev2);
        prod3=( (long) Q_prev *coeff)>>14;
        prod3=( prod3 * Q_prev2);

        power = ((prod1+prod2-prod3))>>8; //calculate power using the three products and scale the result down

        return power;
}

// increments the index of the button that was pressed
void find_freq(void) {

        switch(cChar) {
            case '0':
                freq[0] += 1;
                break;
            case '1':
                freq[1] += 1;
                break;
            case '2':
                freq[2] += 1;
                break;
            case '3':
                freq[3] += 1;
                break;
            case '4':
                freq[4] += 1;
                break;
            case '5':
                freq[5] += 1;
                break;
            case '6':
                freq[6] += 1;
                break;
            case '7':
                freq[7] += 1;
                break;
            case '8':
                freq[8] += 1;
                break;
            case '9':
                freq[9] += 1;
                break;
            case '*':
            case '#':
        }
}

int find_high(void) {
    int i;
    int high = 1;

    for(i = 0; i < 16; i++) {
        if(freq[i] > freq[high]) {
            high = i;
        }
    }

    return high;
}

void clear_freq(void) {
    int i;

    for(i = 0; i < 16; i++) {
        freq[i] = 0;
    }
}

void timer_int_handler(void) {
    unsigned long status;

    status = TimerIntStatus(TIMERA1_BASE, true);
    TimerIntClear(TIMERA1_BASE, TIMER_A);
    TimerIntDisable(TIMERA1_BASE, status);
    TimerDisable(TIMERA1_BASE, TIMER_A);

    t_count++;

    TimerIntEnable(TIMERA1_BASE, TIMER_A);
    TimerLoadSet(TIMERA1_BASE, TIMER_A, 80000000);
    TimerEnable(TIMERA1_BASE, TIMER_A);
}

void post_test(void)
//---------------------------------------------------------------//
{
//initialize variables to be used in the function
int i=0,row=0,col=0,max_power;

     char row_col[4][4] =       // array with the order of the digits in the DTMF system
        {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
        };

     // find the maximum power in the row frequencies and the row number

     max_power=0;            //initialize max_power=0

     for(i=0;i<4;i++)        //loop 4 times from 0>3 (the indecies of the rows)
        {
        if (power_all[i] > max_power)   //if power of the current row frequency > max_power
            {
            max_power=power_all[i];     //set max_power as the current row frequency
            row=i;                      //update row number
            }
        }


     // find the maximum power in the column frequencies and the column number

     max_power=0;            //initialize max_power=0

     for(i=4;i<8;i++)        //loop 4 times from 4>7 (the indecies of the columns)
        {
        if (power_all[i] > max_power)   //if power of the current column frequency > max_power
            {
            max_power=power_all[i];     //set max_power as the current column frequency
            col=i;                      //update column number
            }
        }


     if(power_all[col]==0 && power_all[row]==0) //if the maximum powers equal zero > this means no signal or inter-digit pause
         new_dig=1;                             //set new_dig to 1 to display the next decoded digit


        if((power_all[col]>100000 && power_all[row]>100000)) //&& (new_dig==1)) // check if maximum powers of row & column exceed certain threshold AND new_dig flag is set to 1
            {
                   //write_lcd(1,row_col[row][col-4]);
                  // display the digit on the LCD
            //dis_7seg(8,row_col[row][col-4]);                        // display the digit on 7-seg
        //Report("%c \n\r", row_col[row][col-4]);

        // save the digit in a global variable to access in another function
            cChar = row_col[row][col-4];
            find_freq();
            //cycle_print();
//            new_dig=0;
            // set new_dig to 0 to avoid displaying the same digit again.
            }
    //Report("%c \n\r", row_col[row][col-4]);
}

//*****************************************************************************
//
//! Main  Function
//
//*****************************************************************************
int main()
{
    // Initialize Board configurations
    BoardInit();

    // Pinmux for UART
    PinMuxConfig();

    // Enable the SPI module clock
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    MAP_PRCMPeripheralReset(PRCM_GSPI);

    // Configuring UART
    InitTerm();

    ClearTerm();

    // Display Application Banner
    DisplayBanner(APP_NAME);

    // Configure Timer
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_A, timerhandler);
    TimerLoadSet(TIMERA0_BASE, TIMER_A, 5000);

    // different timer handler 
    Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, timer_int_handler);
    TimerLoadSet(TIMERA1_BASE, TIMER_A, 5000);

    //MAP SPI RESET
    MAP_SPIReset(GSPI_BASE);

    // Configure SPI interface
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                    SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                    (SPI_SW_CTRL_CS |
                    SPI_4PIN_MODE |
                    SPI_TURBO_OFF |
                    SPI_CS_ACTIVEHIGH |
                    SPI_WL_8));

    MAP_SPIEnable(GSPI_BASE);
    SPICSEnable(GSPI_BASE);

    Adafruit_Init();

    SPICSDisable(GSPI_BASE);

    MAP_GPIOPinWrite(GPIOA2_BASE, 0x40, 0x40); 
    MAP_GPIOPinWrite(GPIOA0_BASE, 0x80, 0x80); 
    count = 0;
    int_flag = 1;
    num_press = 0;
    special = 0;
    t_count = 0;
//    g_ulRefBase = TIMERA1_BASE;

    TimerEnable(TIMERA0_BASE, TIMER_A);
    TimerEnable(TIMERA1_BASE, TIMER_A);
    while(1)
    {
        unsigned long status;
        while (int_flag == 0){
            count = 0;
            int_flag = 1;
            status = TimerIntStatus(TIMERA0_BASE, true);
            TimerIntClear(TIMERA0_BASE, status);
            TimerIntDisable(TIMERA0_BASE, status);
            TimerDisable(TIMERA0_BASE, TIMER_A);
            int i;
            for (i=0; i < 8; i++) {
                power_all[i] = goertzel(samples, coeff[i], N);
        }
        post_test();
        if(t_count == 2){
            button = find_high();

            if(button == 7) {
                sp_i = 0;
            }
            if(button == 9) {
                sp_i = 1;
            }

            // 4 letter key
            if(button == 7 || button == 9) {
                Report("%c\r\n", letters4[sp_i][num_press]);
                //Report("special. ci = %d, n = %d\n\r", char_index, num_press);
                if(button == prev) {
                    num_press++;
                }

                if(num_press == 4) {
                    num_press = 0;
                }
            }
            // 3 letter key
            else {
                if(button == 8) {
                    sp_i = 7;
                    Report("%c\r\n", letters3[sp_i][num_press]);
                }
                else if (button != 1) {
                    Report("%c\r\n", letters3[button][num_press]);
                    //Report("%d", button);
                }
                // same button pressed
                // want to iterate to next character of that key
                if(button == prev) {
                    num_press++;
                }

                // cycle back to the first character after last character
                if(num_press == 3) {
                    num_press = 0;
                }
            }

            prev = button;

            clear_freq();
            t_count = 0;
        }
        //cycle_print();
        count = 0;
        TimerIntEnable(TIMERA0_BASE, status);
        TimerLoadSet(TIMERA0_BASE, TIMER_A, 80000000);
        TimerEnable(TIMERA0_BASE, TIMER_A);
        }
    }
}
