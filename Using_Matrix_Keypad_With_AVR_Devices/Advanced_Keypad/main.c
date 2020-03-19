/*
    \file   main.c
    
    \brief  Advanced Keypad Application
    
    (c) 2020 Microchip Technology Inc. and its subsidiaries.
    
    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include <atmel_start.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <string.h>

#define PRESS_VALID 0x01
#define COLUMN_gm   (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm)
#define COLUMN_PORT PORTC

volatile uint8_t key_pressed;
char input_pass[20];
uint8_t cursor = 0;
char passcode[20] = "123ABC";

/* Macro for buttons in the grid */
char btn_value[16] = "123A456B789C*0#D";

/* Function prototypes */
void row_output_column_input(void);
void row_input_column_output(void);
void scan_keys(void);
void btn_debounce(void);
void lp_delay_ms(uint16_t delay);
void check_passcode(void);


int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

	while(1)
	{
		/* Sleep until key press wakes up the CPU */
		sleep_mode();
		/* PRESS_VALID flag in GPIOR0 is high if a valid press is detected */
		btn_debounce();

		/* If the key press was valid */
		if((GPIOR0 & PRESS_VALID) == PRESS_VALID)
		{
			scan_keys();
			check_passcode();
			/* Wait for all buttons to be released */
			while((COLUMN_PORT.IN & COLUMN_gm) != COLUMN_gm)
			{
				/* Sleep until key release wakes up the CPU */
				sleep_mode();
			}
		}
	}
}

void row_output_column_input()
{
	ROW0_set_dir(PORT_DIR_OUT);
	ROW1_set_dir(PORT_DIR_OUT);
	ROW2_set_dir(PORT_DIR_OUT);
	ROW3_set_dir(PORT_DIR_OUT);

	COLUMN0_set_dir(PORT_DIR_IN);
	COLUMN1_set_dir(PORT_DIR_IN);
	COLUMN2_set_dir(PORT_DIR_IN);
	COLUMN3_set_dir(PORT_DIR_IN);
}

void row_input_column_output()
{
	ROW0_set_dir(PORT_DIR_IN);
	ROW1_set_dir(PORT_DIR_IN);
	ROW2_set_dir(PORT_DIR_IN);
	ROW3_set_dir(PORT_DIR_IN);

	COLUMN0_set_dir(PORT_DIR_OUT);
	COLUMN1_set_dir(PORT_DIR_OUT);
	COLUMN2_set_dir(PORT_DIR_OUT);
	COLUMN3_set_dir(PORT_DIR_OUT);
}

void scan_keys()
{
	key_pressed = 0;

	if(!COLUMN0_get_level())
	{
		key_pressed = 0;
	}
	else if(!COLUMN1_get_level())
	{
		key_pressed = 1;
	}
	else if(!COLUMN2_get_level())
	{
		key_pressed = 2;
	}
	else if(!COLUMN3_get_level())
	{
		key_pressed = 3;
	}

	row_input_column_output();

	if(!ROW0_get_level())
	{
		key_pressed += 0;
	}
	else if(!ROW1_get_level())
	{
		key_pressed += 4;
	}
	else if(!ROW2_get_level())
	{
		key_pressed += 8;
	}
	else if(!ROW3_get_level())
	{
		key_pressed += 12;
	}

	row_output_column_input();
}

void btn_debounce()
{
	/* GPIOR0 bit 0 (PRESS_VALID) is press validation flag */
	GPIOR0 |= PRESS_VALID;
	for(uint8_t i = 0; i < 10; i++)
	{
		/* If no button is pressed */
		if((COLUMN_PORT.IN & COLUMN_gm) == COLUMN_gm)
		{
			GPIOR0 &= ~PRESS_VALID;
			break;
		}
		lp_delay_ms(2);
	}
}

void lp_delay_ms(uint16_t delay)
{
	volatile uint16_t pit_cnt = 0;
	/* Enable PIT interrupt */
	RTC.PITINTCTRL = RTC_PI_bm;
	/* Going to sleep until number of ms has been reached */
	while(pit_cnt != delay)
	{
		sleep_mode();
		pit_cnt += 1;
	}
	/* Disable PIT interrupt */
	RTC.PITINTCTRL &= ~RTC_PI_bm;
}

void check_passcode()
{
	/* If star is pressed */
	if(btn_value[key_pressed] == '*')
	{
		/* Reset input_pass */
		memset(input_pass, 0, sizeof(input_pass));
		cursor = 0;
	}
	/* If pound is pressed */
	else if(btn_value[key_pressed] == '#')
	{
		/* If input passcode is the same as the set passcode */
		if(!(strcmp(passcode, input_pass)))
		{
			LED_GREEN_set_level(0);
			lp_delay_ms(500);
			LED_GREEN_set_level(1);
		}
		else
		{
			LED_RED_set_level(0);
			lp_delay_ms(500);
			LED_RED_set_level(1);
		}
		/* Reset input_pass */
		memset(input_pass, 0, sizeof(input_pass));
		cursor = 0;
	}
	/* If no special character (star or pound) is pressed */
	else
	{
		/* If length is not surpassed (20 characters) */
		if(cursor < 20)
		{
			/* Record press */
			input_pass[cursor] = btn_value[key_pressed];
			cursor++;
		}
		else
		{
			cursor = 0;
			LED_RED_set_level(0);
			lp_delay_ms(250);
			LED_RED_set_level(1);
			lp_delay_ms(250);
			LED_RED_set_level(0);
			lp_delay_ms(250);
			LED_RED_set_level(1);
			/* Reset input_pass */
			memset(input_pass, 0, sizeof(input_pass));
		}
	}
}

ISR(PORTC_PORT_vect)
{
	/* Clear interrupt flag */
	COLUMN_PORT.INTFLAGS = COLUMN_gm;
}

ISR(RTC_PIT_vect)
{
	/* Clear interrupt flag */
	RTC.PITINTFLAGS = RTC_PI_bm;
}