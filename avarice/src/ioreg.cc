/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2003 James Harris
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License Version 2
 *      as published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 * This file contains IO Register definitions for use with avr-gdb's 'info
 * io_regsiter' command.
 */

#include "ioreg.h"

gdb_io_reg_def_type atmega16_io_registers[] =
{
    { "TWBR",    0x20, 0x00 },
    { "TWSR",    0x21, 0x00 },
    { "TWAR",    0x22, 0x00 },
    { "TWDR",    0x23, 0x00 },
    { "ADCL",    0x24, IO_REG_RSE }, // Reading during a conversion corrupts
    { "ADCH",    0x25, IO_REG_RSE }, // ADC result
    { "ADCSRA",  0x26, 0x00 },
    { "ADMUX",   0x27, 0x00 },
    { "ACSR",    0x28, 0x00 },
    { "UBRRL",   0x29, 0x00 },
    { "UCSRB",   0x2A, 0x00 },
    { "UCSRA",   0x2B, 0x00 },
    { "UDR",     0x2C, IO_REG_RSE }, // Reading this clears the UART RXC flag
    { "SPCR",    0x2D, 0x00 },
    { "SPSR",    0x2E, 0x00 },
    { "SPDR",    0x2F, 0x00 },
    { "PIND",    0x30, 0x00 },
    { "DDRD",    0x31, 0x00 },
    { "PORTD",   0x32, 0x00 },
    { "PINC",    0x33, 0x00 },
    { "DDRC",    0x34, 0x00 },
    { "PORTC",   0x35, 0x00 },
    { "PINB",    0x36, 0x00 },
    { "DDRB",    0x37, 0x00 },
    { "PORTB",   0x38, 0x00 },
    { "PINA",    0x39, 0x00 },
    { "DDRA",    0x3A, 0x00 },
    { "PORTA",   0x3B, 0x00 },
    { "EECR",    0x3C, 0x00 },
    { "EEDR",    0x3D, 0x00 },
    { "EEARL",   0x3E, 0x00 },
    { "EEARH",   0x3F, 0x00 },
    { "UCSRC -- UBRRH",   0x40, 0x00 },
    { "WDTCR",   0x41, 0x00 },
    { "ASSR",    0x42, 0x00 },
    { "OCR2",    0x43, 0x00 },
    { "TCNT2",   0x44, 0x00 },
    { "TCCR2",   0x45, 0x00 },
    { "ICR1L",   0x46, 0x00 },
    { "ICR1H",   0x47, 0x00 },
    { "OCR1BL",  0x48, 0x00 },
    { "OCR1BH",  0x49, 0x00 },
    { "OCR1AL",  0x4A, 0x00 },
    { "OCR1AH",  0x4B, 0x00 },
    { "TCNT1L",  0x4C, 0x00 },
    { "TCNT1H",  0x4D, 0x00 },
    { "TCCR1B",  0x4E, 0x00 },
    { "TCCR1A",  0x4F, 0x00 },
    { "SFIOR",   0x50, 0x00 },
    { "OCDR -- OSCCAL",    0x51, 0x00 },
    { "TCNT0",   0x52, 0x00 },
    { "TCCR0",   0x53, 0x00 },
    { "MCUCSR",  0x54, 0x00 },
    { "MCUCR",   0x55, 0x00 },
    { "TWCR",    0x56, 0x00 },
    { "SPMCR",   0x57, 0x00 },
    { "TIFR",    0x58, 0x00 },
    { "TIMSK",   0x59, 0x00 },
    { "GIFR",    0x5A, 0x00 },
    { "GICR",    0x5B, 0x00 },
    { "OCR0",    0x5C, 0x00 },
    { "SPL",     0x5D, 0x00 },
    { "SPH",     0x5E, 0x00 },
    { "SREG",    0x5F, 0x00 },
    { 0, 0, 0 }
};

