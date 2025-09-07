#include <avr/io.h>
#include "AVR VUB/avrvub.h"
#include <util/delay.h>
#include "LCD/lcd.h"
#include "Timer/timer.h"
#include "Interrupt/interrupt.h"

// Measurement variables
volatile uint32_t broj_impulsa1 = 0; // ECHO1 (PB4)
volatile uint32_t broj_impulsa2 = 0; // ECHO2 (PB5)
volatile uint32_t broj_impulsa3 = 0; // ECHO3 (PB6)
volatile bool hcsr04_measured1 = false;
volatile bool hcsr04_measured2 = false;
volatile bool hcsr04_measured3 = false;
volatile uint8_t echo1_high = 0, echo2_high = 0, echo3_high = 0;

// ISR for PCINT0_vect (handles PB4, PB5, PB6)
ISR(PCINT0_vect)
{
	uint8_t pins = PINB;
	// Sensor 1: PB4
	if (pins & (1 << PB4)) {
		if (!echo1_high) { echo1_high = 1; TCNT1 = 0; }
		} else {
		if (echo1_high) {
			echo1_high = 0;
			broj_impulsa1 = TCNT1;
			hcsr04_measured1 = true;
		}
	}
	// Sensor 2: PB5
	if (pins & (1 << PB5)) {
		if (!echo2_high) { echo2_high = 1; TCNT1 = 0; }
		} else {
		if (echo2_high) {
			echo2_high = 0;
			broj_impulsa2 = TCNT1;
			hcsr04_measured2 = true;
		}
	}
	// Sensor 3: PB6
	if (pins & (1 << PB6)) {
		if (!echo3_high) { echo3_high = 1; TCNT1 = 0; }
		} else {
		if (echo3_high) {
			echo3_high = 0;
			broj_impulsa3 = TCNT1;
			hcsr04_measured3 = true;
		}
	}
}

void inicijalizacija()
{
	lcd_init();
	// Timer1 normal mode, prescaler 8
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1B |= (1 << CS11);
	// TRIG pins: PD2, PD3, PD4
	DDRD |= (1 << PD2) | (1 << PD3) | (1 << PD4);
	// ECHO pins as input: PB4, PB5, PB6
	DDRB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6));
	// Remove pull-ups if needed
	PORTB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6));
	// Enable PCINT for PB4, PB5, PB6
	PCMSK0 |= (1 << PCINT4) | (1 << PCINT5) | (1 << PCINT6);
	PCICR |= (1 << PCIE0);
	sei();
}

void hcsr04_trigg(uint8_t id)
{
	switch (id)
	{
		case 0:
		PORTD |= (1 << PD2);
		_delay_us(10);
		PORTD &= ~(1 << PD2);
		break;
		case 1:
		PORTD |= (1 << PD3);
		_delay_us(10);
		PORTD &= ~(1 << PD3);
		break;
		case 2:
		PORTD |= (1 << PD4);
		_delay_us(10);
		PORTD &= ~(1 << PD4);
		break;
	}
}

int main(void)
{
	inicijalizacija();
	float d1 = 0, d2 = 0, d3 = 0;
	float t_echo;
	uint16_t time_delay = 0;
	uint8_t koji = 0;
	char buf[17];

	while (1)
	{
		// Trigger sensors sequentially every ~50ms (full cycle 150ms)
		if (time_delay == 0) {
			hcsr04_trigg(koji);
			koji = (koji + 1) % 3;
		}
		if (hcsr04_measured1) {
			t_echo = (float)broj_impulsa1 * 8.0f / (float)F_CPU;
			d1 = (t_echo / 2.0f) * 343.0f * 100.0f;
			hcsr04_measured1 = false;
		}
		if (hcsr04_measured2) {
			t_echo = (float)broj_impulsa2 * 8.0f / (float)F_CPU;
			d2 = (t_echo / 2.0f) * 343.0f * 100.0f;
			hcsr04_measured2 = false;
		}
		if (hcsr04_measured3) {
			t_echo = (float)broj_impulsa3 * 8.0f / (float)F_CPU;
			d3 = (t_echo / 2.0f) * 343.0f * 100.0f;
			hcsr04_measured3 = false;
		}
		// LCD: show 3 sensor distances, max 16 chars per line
		if (time_delay == 0)
		{
			lcd_clrscr();
			lcd_home();
			// First line: D1:x.x D2:y.y
			lcd_print("%.1f %.1f\n", d1, d2);
			// Second line: D3:z.z
			lcd_print("%.1f", d3);
		}

		_delay_ms(10);
		if (++time_delay > 5)
		time_delay = 0;
	}
}
