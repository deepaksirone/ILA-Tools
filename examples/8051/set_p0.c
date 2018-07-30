/*
 * Copyright (c) 1999-2001 Tony Givargis.  Permission to copy is granted
 * provided that this header remains intact.  This software is provided
 * with no warranties.
 *
 * Version : 2.9
 */

/*---------------------------------------------------------------------------*/

#include <8052.h>

/*---------------------------------------------------------------------------*/
unsigned char data[10]; // allocated at 8h
unsigned char secret[10]; //allocated at 12h
/*
void fib(unsigned char n) {
    
    unsigned char i;
    
    buf[0] = 0;
    buf[1] = 1;
    for(i=2; i<n; i++) {
        buf[i] = buf[i-1] + buf[i-2];
    }
}
*/
/*---------------------------------------------------------------------------*/
/*
void print(unsigned char n) {
    
    unsigned char i;
    
    for(i=0; i<n; i++) {
        P0 = buf[i];
    }
}
*/
/*---------------------------------------------------------------------------*/

void quit() {
    P0 = P1 = P2 = P3 = 0xDE;
    P0 = P1 = P2 = P3 = 0xAD;
    P0 = P1 = P2 = P3 = 0x00;
    while(1);
}
/*---------------------------------------------------------------------------*/

void main() {

	volatile unsigned char secret;
	volatile unsigned char data;
	if (secret == 0)
		P0 = data;
	quit();
}
