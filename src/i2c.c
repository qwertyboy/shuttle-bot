/*
 *  i2c.c
 *  Functions for using I2C on the MSP430. USCI_A is reserved for UART so we
 *  are using USCI_B, which supports I2C and SPI.
 *
 *  MSP430F22x2/4 Datasheet: http://www.ti.com/lit/ds/symlink/msp430f2274.pdf
 *  MSP430x2xx USer Guide: http://www.ti.com/lit/ug/slau144j/slau144j.pdf
 *
 *  Version 1: 04/01/17 - Nathan Duprey
 *                      - added initialization and send cmd
 */

#include "i2c.h"
#include "msp430x22x4.h"
#include "stdint.h"

void I2CInitMaster(void)
//-------------------------------------------------------------------------
// Func:  Configure I2C for master mode, SMCLK source
// Args:  None
// Retn:  None
//-------------------------------------------------------------------------
{
    UCB0CTL1 = UCSWRST;                     // stop/reset USCB0
    UCB0CTL0 = UCMODE_3 | UCMST | UCSYNC;   // I2C mode, master, synchronous
    UCB0CTL1 = UCSSEL_2;                    // SMCLK source
    UCB0BR1 = 0;                            // msb of divisor
    UCB0BR0 = 12;                           // lsb of divisor, SMCLK/12 = ~100kHz
    UCB0CTL1 &= ~UCSWRST;                   // start USCB0
}

void I2CSetSlaveAddr(uint16_t addr)
//-------------------------------------------------------------------------
// Func:  Set I2C slave address
// Args:  addr - slave address
// Retn:  None
//-------------------------------------------------------------------------
{
    UCB0I2CSA = addr;   // set slave address
}

void I2CSendByte(uint8_t data)
//-------------------------------------------------------------------------
// Func:  Send a single byte over I2C. Note: this is blocking until the bus
//        is free and the transaction is complete
// Args:  data - 8 bit data to send
// Retn:  None
//-------------------------------------------------------------------------
{
    while(UCB0STAT & UCBBUSY);      // wait for bus to be free
    UCB0CTL1 |= UCTR | UCTXSTT;     // send start bit, slave addr, write bit
    while(!(IFG2 & UCB0TXIFG));     // wait until tx buffer is empty
    UCB0TXBUF = data;               // send data
    while(!(IFG2 & UCB0TXIFG));     // wait until tx buffer is empty
    UCB0CTL1 |= UCTXSTP;            // send stop bit
    while(UCB0CTL1 & UCTXSTP);      // wait until stop bit is sent
}

void I2CSend(uint8_t * data, uint8_t length)
//-------------------------------------------------------------------------
// Func:  Send multiple bytes over I2C. Note: this is blocking until the bus
//        is free and the transaction is complete. Also sends MSB first
// Args:  data - pointer to data byte array
//        length - length of data in bytes
// Retn:  None
//-------------------------------------------------------------------------
{
    while(UCB0STAT & UCBBUSY);      // wait for bus to be free
    UCB0CTL1 |= UCTR | UCTXSTT;     // send start bit, slave addr, write bit

    int8_t i;
    for(i = length - 1; i >= 0; i--)
    {
        while(!(IFG2 & UCB0TXIFG)); // wait until tx buffer is empty
        UCB0TXBUF = data[i];        // send data
    }
    while(!(IFG2 & UCB0TXIFG));     // wait until tx buffer is empty
    UCB0CTL1 |= UCTXSTP;            // send stop bit
    while(UCB0CTL1 & UCTXSTP);      // wait until stop bit is sent
}