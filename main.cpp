+/*
 Tema: Parking senzori
 Autor: Matko Manzin
*/

#include <avr/io.h>
#include "AVR VUB/avrvub.h"
#include <util/delay.h>
#include "LCD/lcd.h"
#include "Timer/timer.h"
#include "Interrupt/interrupt.h"
// #include "ADC/adc.h" // Korekcija po temperaturi

// Mjerenje za 2 senzora
volatile uint32_t broj_impulsa1 = 0; // ECHO1 (PB4)
volatile uint32_t broj_impulsa2 = 0; // ECHO2 (PB5)
bool hcsr04_measured1 = false;
bool hcsr04_measured2 = false;

// Stanje ECHO pinova
volatile uint8_t echo1_high = 0;
volatile uint8_t echo2_high = 0;

// PCINT0 ISR (PB4/PB5)
ISR(PCINT0_vect)
{
    uint8_t pins = PINB;

    // Senzor 1: PB4
    if (pins & (1<<PB4)) {
        if (!echo1_high) {
            echo1_high = 1; // Rastuci brid
            TCNT1 = 0; // Start mjerenja
        }
    } else {
		if (echo1_high) {
            echo1_high = 0; // Padajuci brid
            broj_impulsa1 = TCNT1;
            hcsr04_measured1 = true;
        }
    }

    // Senzor 2: PB5
    if (pins & (1<<PB5)) {
        if (!echo2_high) {
            echo2_high = 1; // Rastuci brid
            TCNT1 = 0; // Start mjerenja
        }
    } else {
        if (echo2_high) {
            echo2_high = 0; // Padajuci brid
            broj_impulsa2 = TCNT1;
            hcsr04_measured2 = true;
        }
    }
}

void inicijalizacija()
{
    lcd_init();                 // Inicijalizacija LCD displeja
    // adc_init();              // Korekcija po temperaturi

    // Timer1 normalni nacin rada
	TCCR1A &= ~((1 << WGM00) | (1 << WGM01));
	TCCR1B &= ~(1 << WGM02);

    // F_CPU/8
	TCCR1B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
	TCCR1B |= (1 << CS01);

    // Konfiguracija pinova za HCSR04
	// TRIG1 (PD2) - izlazni pin
	DDRD |= (1 << PD2);
	PORTD |= (1 << PD2);

	// TRIG2 (PD3) - izlazni pin
	DDRD |= (1 << PD3);
	PORTD |= (1 << PD3);

    // ECHO1 (PB4) - ulazni pin
	DDRB &= ~(1 << PB4);
	PORTB |= (1 << PB4);

	// ECHO2 (PB5) - ulazni pin
	DDRB &= ~(1 << PB5);
	PORTB |= (1 << PB5);

    // Omoguci pin-change za PB4 i PB5 (PCINT4, PCINT5) na portu B
    PCMSK0 |= (1<<PCINT4) | (1<<PCINT5);
    PCICR  |= (1<<PCIE0);

    sei(); // Omoguci prekide (sei)
}

// TRIG funkcije
void hcsr04_trigg1()
{
    PORTD |= (1 << PD2);
    _delay_us(10);
    PORTD &= ~(1 << PD2);
}

void hcsr04_trigg2()
{
    PORTD |= (1 << PD3);
    _delay_us(10);
    PORTD &= ~(1 << PD3);
}

int main(void)
{
    inicijalizacija();

    float d1 = 0.0f, d2 = 0.0f;		// Izracun udaljenosti [cm]
    float t_echo;					// Trajanje impulsa na ECHO pinu [s]
    uint16_t time_delay = 0;
    // uint16_t ADC_4;				// Korekcija po temperaturi
    // float T, v_z;

    uint8_t koji = 0;				// 0 -> trigg senzor 1, 1 -> trigg senzor 2

    while(1) 
	{
        // Naizmjenicno salje TRIG svakih ~100ms
        if (time_delay == 0) {
            if (koji == 0) {
				hcsr04_trigg1();
			}
            else {
				hcsr04_trigg2();
			}
            
			koji ^= 1; // Mijenjaj senzor 0/1
        }


        if (hcsr04_measured1) { // Ako je mjerenje zavrseno za senzor 1
            t_echo = (float)broj_impulsa1 * 8.0f / (float)F_CPU; // FORMULA: t_echo = broj_impulsa * PRESCALER / F_CPU
            // v_z = 343.0f;  // [m/s] (korekcija po temperaturi)
            // d1 = t_echo / 2 * v_z * 100; // FORMULA za korekciju po temperaturi
            d1 = (t_echo / 2.0f) * 343.0f * 100.0f; //FORMULA bez korekcije po temperaturi
            hcsr04_measured1 = false;
        }

        if (hcsr04_measured2) { // Ako je mjerenje zavrseno za senzor 2
            t_echo = (float)broj_impulsa2 * 8.0f / (float)F_CPU; // FORMULA: t_echo = broj_impulsa * PRESCALER / F_CPU
            d2 = (t_echo / 2.0f) * 343.0f * 100.0f; //FORMULA bez korekcije po temperaturi
            hcsr04_measured2 = false;
        }

        // LCD ispis
        if (time_delay == 0) {
            lcd_clrscr();
            lcd_home();
            lcd_print("d1 = %0.2f cm\n", d1);
            lcd_print("d2 = %0.2f cm",  d2);
        }

        _delay_ms(10);
        if (time_delay > 10) {
            time_delay = 0;
        } else {
			time_delay++;
		}
    }
}
