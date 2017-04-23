/*
 *  main.c
 *  Main file for EE4735 shuttle bot project
 *
 *  Team Members:   Nathan Duprey
 *                  Noah Symanzik
 *                  Chad Pollock
 */

#include "msp430x22x4.h"
#include "uart/uart.h"
#include "mma8450q/mma8450q.h"
#include "i2c/i2c.h"
#include "stdint.h"

const uint8_t forward[] = {96, 234};      // preset motor commands
const uint8_t stop[] = {0, 0};
const uint8_t reverse[] = {32, 160};

#pragma vector=TIMERA1_VECTOR
#pragma type_attribute=__interrupt
void TimerA1Interrupt(void)
{
    switch(__even_in_range(TAIV, 10))
    {
        case TAIV_TAIFG:
            __bic_SR_register_on_exit(CPUOFF);
            break;
        case TAIV_TACCR1:
            break;
        case TAIV_TACCR2:
            break;
        default:
            break;
    }
}

int16_t NewVel(int16_t accel, int16_t vInit, uint8_t tmsec)
//-------------------------------------------------------------------------
// Func:  Calculate total distance travelled given initial velocity
// Args:  accel  - acceleration from accelerometer
//        vInit  - initial velocity in units/msec (previously returned newVel)
//        tmsec  - time period in miliseconds
// Retn:  newVel - new velocity in units/dsec
//-------------------------------------------------------------------------
{
    int16_t dVel     = (accel * tmsec) / 100;  // change in velocity in units/dsec
    int16_t newVel = dVel + vInit;           // new velocity after acceleration
    return(newVel);
}

int16_t NewDist(int16_t vel, int16_t currDist, uint8_t tmsec)
//-------------------------------------------------------------------------
// Func:  Calculate total distance travelled given velocity and time
// Args:  vel      - velocity in units/msec (previously returned newVel)
//        currDist - current distance travelled in units
//        tmsec    - time period in miliseconds
// Retn:  newDist  - new distance in units
//-------------------------------------------------------------------------
{
    int16_t dDist = (vel * tmsec) / 100;     // change in distance
    int16_t newDist = dDist + currDist;      // new distance
    return(newDist);
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // disable watchdog
    DCOCTL = CALDCO_1MHZ;       // 1MHz DCO
    BCSCTL1 = CALBC1_1MHZ;
    P1DIR |= 0x03;              // set led outputs
    P1OUT &= ~0x03;             // clear led outputs

    UARTInit();         // initialize uart
    P1OUT |= 0x01;      // turn on red led while setting up accelerometer
    MMA8450Init();      // initialize accelerometer
    MMA8450SetZero();   // zero out accelerometer, dont move robot while happening
    P1OUT &= ~0x01;     // turn off led after finished

    TACCR0 = 5000;                          // 1 MHz / 5000 = 200Hz
    TACTL = TASSEL_2 | ID_0 | MC_1 | TAIE;  // SMCLK, div 1, Up mode


    int16_t accelData[3];
    while(1)
    {
        int16_t data[3];            // array for storing acceleration data
        int16_t xAccel;             // x component of acceleration
        static int16_t vel = 0;     // current velocity
        static int16_t dist = 0;    // distance travelled
        int8_t t = 20;             // time step between updates
        static int8_t step = 0;     // flag for what action is happening
        static uint8_t i = 0;       // sample counter

        P1OUT |= 0x02;
        MMA8450ReadXYZ(data);   // read accelerometer
        // convert to 16 bit signed and sum samples
        xAccel += (data[0] > 0x07FF) ? (data[0] - 4096) : data[0];
        P1OUT &= ~0x02;
        i += 1;                 // increment sample counter

        if(i == 4)
        {
            i = 0;                              // reset sample counter
            xAccel >>= 2;                       // divide by 4 to get average
            //xaccel = data[0] * 10;              // Convert x to mm/s^2 and store
            vel = NewVel(xAccel, vel, t);       // Find velocity and distance
            dist = NewDist(vel, dist, t);

            if(dist > 1000 && step == 0)        // Stop at 12 meters
            {
              UARTSend(stop, 2);
              step = 1;
              for (i = 0; i < 1000000; i++){};    // Wait 1 sec before reversing
              UARTSend(reverse, 2);
            }
            else if(dist < 0 && step == 1)      // Stop at starting line
            {
              UARTSend(stop, 2);
            }
        }

        __bis_SR_register(CPUOFF | GIE);
    }
}
