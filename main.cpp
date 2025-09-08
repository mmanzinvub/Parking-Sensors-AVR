#include <avr/io.h>
#include "AVR VUB/avrvub.h"
#include <util/delay.h>
#include "LCD/lcd.h"
#include "Timer/timer.h"
#include "Interrupt/interrupt.h"
#include "ADC/adc.h"

// Measurement variables
volatile uint32_t broj_impulsa1 = 0; // ECHO1 (PB4)
volatile uint32_t broj_impulsa2 = 0; // ECHO2 (PB5)
volatile uint32_t broj_impulsa3 = 0; // ECHO3 (PB6)
volatile bool hcsr04_measured1 = false;
volatile bool hcsr04_measured2 = false;
volatile bool hcsr04_measured3 = false;
volatile uint8_t echo1_high = 0, echo2_high = 0, echo3_high = 0;

// Updated LED pin mapping (GREEN moved to PF5)
#define LED_GREEN  PF5
#define LED_YELLOW PF6
#define LED_RED    PF7
#define BUZZER     PB7  // active buzzer via NPN low-side, PB7 LOW = ON

// BUZZER state (non-blocking)
static uint8_t  buzz_on = 0;         // 0=OFF phase, 1=ON phase
static uint16_t buzz_phase_ms = 0;   // elapsed in current phase
static uint16_t buzz_on_ms = 0;      // ON duration
static uint16_t buzz_off_ms = 0;     // OFF duration
static uint8_t  last_zone = 0xFF;    // force initial set

// ISR for PCINT0_vect (handles PB4, PB5, PB6)
ISR(PCINT0_vect)
{
	uint8_t pins = PINB;
	// Sensor 1: PB4
	if (pins & (1 << PB4)) {
		if (!echo1_high) { 
			echo1_high = 1; TCNT1 = 0; 
		}
	} else {
		if (echo1_high == 1) {
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

	// Timer1 normal mode
	TCCR1A &= ~((1 << WGM00) | (1 << WGM01));
	TCCR1B &= ~(1 << WGM02);

	// F_CPU/8, PRESCALER
	TCCR1B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
	TCCR1B |= (1 << CS01);

	// TRIG pins: PD2, PD3, PD4
	DDRD |= (1 << PD2) | (1 << PD3) | (1 << PD4);

	// ECHO pins as input: PB4, PB5, PB6 (no pull-ups)
	DDRB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6));
	PORTB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6));

	// Enable PCINT for PB4, PB5, PB6
	PCMSK0 |= (1 << PCINT4) | (1 << PCINT5) | (1 << PCINT6);
	PCICR  |= (1 << PCIE0);

	// LED pins as outputs, start OFF (GREEN=PF5, YELLOW=PF6, RED=PF7)
	DDRF |= (1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED); 
	PORTF &= ~((1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED));

	// BUZZER pin output, default OFF (HIGH)
	DDRB  |= (1 << BUZZER);
	PORTB |= (1 << BUZZER);

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

// BUZZER: select beep pattern per zone
static void buzzer_set_pattern(uint8_t zone) {
	// zone: 0=off, 1=green slow, 2=yellow medium, 3=red fast
	if (zone == 1) {         // green
		buzz_on_ms  = 500;
		buzz_off_ms = 100;
		} else if (zone == 2) {  // yellow
		buzz_on_ms  = 300;
		buzz_off_ms = 90;
		} else if (zone == 3) {  // red
		buzz_on_ms  = 120;
		buzz_off_ms = 80;
		} else {                 // off
		buzz_on_ms  = 0;
		buzz_off_ms = 0;
	}
	buzz_on = (buzz_on_ms > 0); // 120 > 0 ---> buzz_on = 1, true ; 0 > 0 ---> 0, buzz_on = 0
	buzz_phase_ms = 0;
	if (buzz_on = 0) {
		PORTB |= (1 << BUZZER); // HIGH = OFF
	}
}

// BUZZER: active buzzer drive (LOW = ON, HIGH = OFF)
static void buzzer_update(uint16_t dt_ms) {
	if (buzz_on_ms == 0 && buzz_off_ms == 0) {
		PORTB |= (1 << BUZZER);  // OFF
		return;
	}
	if (buzz_on == 1) {
		PORTB &= ~(1 << BUZZER); // ON
		} else {
		PORTB |= (1 << BUZZER);  // OFF
	}

	buzz_phase_ms += dt_ms;

	if (buzz_on) {
		if (buzz_phase_ms >= buzz_on_ms) { buzz_phase_ms = 0; buzz_on = 0; }
		} else {
		if (buzz_phase_ms >= buzz_off_ms) { buzz_phase_ms = 0; buzz_on = 1; }
	}
}

int main(void)
{
	inicijalizacija();

	float d1 = 0, d2 = 0, d3 = 0;
	float t_echo;
	uint16_t time_delay = 0;
	uint8_t koji = 0;

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

		static uint16_t lcd_brojac = 0;  // redraw ~200 ms to avoid flicker
		
		lcd_brojac += 10;

		if (lcd_brojac >= 200) {
			lcd_brojac = 0;

			lcd_clrscr();
			lcd_home();

			lcd_gotoxy(0, 0);
			lcd_print("L:%0.1f  D:%0.1f", d1, d3);

			lcd_gotoxy(1, 0);
			lcd_print("C:%0.1f", d2);
		}

		// Determine closest distance and zone
		float min_dist = d1;
		if (d2 < min_dist) min_dist = d2;
		if (d3 < min_dist) min_dist = d3;

		uint8_t zone = 0;
		if (min_dist <= 200.0f && min_dist >= 60.0f)      zone = 1;
		else if (min_dist < 60.0f && min_dist >= 20.0f)   zone = 2;
		else if (min_dist < 20.0f && min_dist >= 0.0f)    zone = 3;

		// LEDs (one at a time) ? Port F only (GREEN on PF5)
		PORTF &= ~((1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED));

		if (zone == 1) {
			PORTF |= (1 << LED_GREEN);
		} else if (zone == 2) {
			PORTF |= (1 << LED_YELLOW);
		} else if (zone == 3) {
			PORTF |= (1 << LED_RED); 
		}

		// Buzzer pattern
		if (zone != last_zone) {
			buzzer_set_pattern(zone);
			last_zone = zone;
		}

		// Advance buzzer timing by loop delay (10 ms below)
		buzzer_update(10);

		_delay_ms(10);

		time_delay++;
		if (time_delay > 4)
		time_delay = 0;
	}
}