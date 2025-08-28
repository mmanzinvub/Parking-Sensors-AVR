/*
<<<<<<< Updated upstream
 Naslov: MIKROUPRAVLJACI - Programiranje mikrokontrolera AVR familije
 Autor: prilagodba za 2x HC-SR04 (PD2/PB4 i PD3/PB5)

 Vježba 12.3.1 (format zadržan, ECHO preko PCINT na PB4/PB5)
=======
	Tema: Parking senzori za automobil
	Autor: Matko Manzin
>>>>>>> Stashed changes
*/

#include <avr/io.h>
#include "AVR VUB/avrvub.h"
#include <util/delay.h>
#include "LCD/lcd.h"
#include "Timer/timer.h"
#include "Interrupt/interrupt.h"
<<<<<<< Updated upstream
// #include "ADC/adc.h" // uklju?i ako želiš korekciju po temperaturi

// --- Mjerenje za 2 senzora (Timer1 tick-ovi) ---
volatile uint32_t broj_impulsa1 = 0;    // ECHO1 (PB4)
volatile uint32_t broj_impulsa2 = 0;    // ECHO2 (PB5)
bool hcsr04_measured1 = false;
bool hcsr04_measured2 = false;

// unutarnje stanje ECHO pinova
volatile uint8_t echo1_high = 0;
volatile uint8_t echo2_high = 0;

// ---------------- PCINT0 ISR (PB4/PB5) ----------------
// Umjesto INT6 iz skripte, koristimo pin-change za port B.
ISR(PCINT0_vect)
{
    uint8_t pins = PINB;

    // Senzor 1: PB4
    if (pins & (1<<PB4)) {
        if (!echo1_high) {
            echo1_high = 1;      // rastu?i brid
            TCNT1 = 0;           // start mjerenja
        }
    } else {
        if (echo1_high) {
            echo1_high = 0;      // padaju?i brid
            broj_impulsa1 = TCNT1;
            hcsr04_measured1 = true;
        }
    }

    // Senzor 2: PB5
    if (pins & (1<<PB5)) {
        if (!echo2_high) {
            echo2_high = 1;      // rastu?i brid
            TCNT1 = 0;           // start mjerenja
        }
    } else {
        if (echo2_high) {
            echo2_high = 0;      // padaju?i brid
            broj_impulsa2 = TCNT1;
            hcsr04_measured2 = true;
        }
    }
}

// ---------------- Inicijalizacija (format iz vježbe) ----------------
void inicijalizacija()
{
    lcd_init();                 // inicijalizacija LCD displeja
    // adc_init();              // ako želiš T-temperaturu kasnije

    // tajmer 1 u normalnom na?inu rada
    timer1_set_normal_mode();
    timer1_set_prescaler(TIMER1_PRESCALER_8); // F_CPU/8 (tick ? 0.5us @16MHz)

    // konfiguracija pinova za hcsr04 (TRIG na PD2 i PD3)
    output_port(DDRD, PD2);     // TRIG1 (PD2) - izlazni pin
    output_port(DDRD, PD3);     // TRIG2 (PD3) - izlazni pin

    // ECHO pinovi kao ulazi (PB4, PB5)
    input_port(DDRB, PB4);      // ECHO1 (PB4) - ulaz
    input_port(DDRB, PB5);      // ECHO2 (PB5) - ulaz

    // Omogu?i pin-change za PB4 i PB5 (PCINT4, PCINT5) na portu B
    PCMSK0 |= (1<<PCINT4) | (1<<PCINT5);
    PCICR  |= (1<<PCIE0);

    interrupt_enable();         // omogu?i prekide (sei)
}

// ---------------- TRIG funkcije (isti stil) ----------------
void hcsr04_trigg1()
{
    set_port(PORTD, PD2, 1);
    _delay_us(10);              // ~10–20 us
    set_port(PORTD, PD2, 0);
}
void hcsr04_trigg2()
{
    set_port(PORTD, PD3, 1);
    _delay_us(10);
    set_port(PORTD, PD3, 0);
}

// ---------------- Glavni program ----------------
=======
// #include "ADC/adc.h" // korekcija po temperaturi

volatile uint32_t broj_impulsa1 = 0;	// ECHO1 (PB4)
volatile uint32_t broj_impulsa2 = 0;	// ECHO2 (PB5)
bool hcsr04_measured1 = false;
bool hcsr04_measured2 = false;

// stanje ECHO pinova
volatile uint8_t echo1_high = 0;
volatile uint8_t echo2_high = 0;

// PCINT0 ISR (PB4/PB5)
ISR(PCINT0_vect)
{
    uint8_t pins = PINB;

    // Senzor 1: PB4
    if (pins & (1<<PB4)) {
        if (!echo1_high) {
            echo1_high = 1;
            TCNT1 = 0;
        }
    } else {
        if (echo1_high) {
            echo1_high = 0;
            broj_impulsa1 = TCNT1;
            hcsr04_measured1 = true;
        }
    }

    // Senzor 2: PB5
    if (pins & (1<<PB5)) {
        if (!echo2_high) {
            echo2_high = 1;
            TCNT1 = 0;
        }
    } else {
        if (echo2_high) {
            echo2_high = 0;
            broj_impulsa2 = TCNT1;
            hcsr04_measured2 = true;
        }
    }
}

void inicijalizacija()
{
    lcd_init();					// inicijalizacija LCD displeja
    // adc_init();				// korekcija po temperaturi

    // tajmer 1 u normalnom nacinu rada
	TCCR1A &= ~((1 << WGM00) | (1 << WGM01));
	TCCR1B &= ~(1 << WGM02);
	
    // F_CPU/8
	TCCR1B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
	TCCR1B |= (1 << CS01);

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
	PORTB |= (1 << PB5)

    // omoguci pin-change za PB4 i PB5 (PCINT4, PCINT5) na portu B
    PCMSK0 |= (1<<PCINT4) | (1<<PCINT5);
    PCICR  |= (1<<PCIE0);

    sei(); // omoguci prekide
}

// TRIG funkcije
void hcsr04_trigg1()
{
    PORTD |= (1 << PD2);
    _delay_us(10);				// ~10–20 us
    PORTD &= ~(1 << PD2);
}
void hcsr04_trigg2()
{
    PORTD |= (1 << PD3);
    _delay_us(10);
    PORTD &= ~(1 << PD3);
}

>>>>>>> Stashed changes
int main(void)
{
    inicijalizacija();

<<<<<<< Updated upstream
    float d1 = 0.0f, d2 = 0.0f;    // udaljenosti u cm
    float t_echo;                  // trajanje impulsa na Echo pinu (s)
    uint16_t time_delay = 0;       // brojanje ~100ms
    // uint16_t ADC_4;             // za temp. korekciju ako koristiš ADC
    // float T, v_z;

    uint8_t koji = 0;              // 0 -> triggaj senzor 1, 1 -> senzor 2

    while(1)
    {
        // Naizmjeni?no šalji TRIG svakih ~100ms (jednostavno, stabilno)
=======
    float d1 = 0.0f, d2 = 0.0f;		// udaljenost u cm
    float t_echo;					// trajanje impulsa na echo pinu (s)
    uint16_t time_delay = 0;		// brojanje 100ms
    // uint16_t ADC_4;				// za korekciju temperature
    // float T, v_z;

	// koji senzor aktivirati; 0 -> senzor 1, 1 -> senzor 2
    uint8_t koji = 0;              

    while(1)
    {
        // stalno šalji TRIG svakih 100ms
>>>>>>> Stashed changes
        if (time_delay == 0) {
            if (koji == 0) { hcsr04_trigg1(); }
            else            { hcsr04_trigg2(); }
            koji ^= 1; // prebacuj 0/1
        }

<<<<<<< Updated upstream
        // Ako je mjerenje završeno za senzor 1
        if (hcsr04_measured1) {
            // t_echo = broj_impulsa * PRESCALER / F_CPU (kao u skripti)
            t_echo = (float)broj_impulsa1 * 8.0f / (float)F_CPU;  // s
            // v_z = 343.0f;  // m/s (bez korekcije)
            // d1 = t_echo / 2 * v_z * 100;
=======
        // ako je senzor 1 napravio mjerenje
        if (hcsr04_measured1) {
            // FORMULA: t_echo = broj_impulsa * PRESCALER / F_CPU
            t_echo = (float)broj_impulsa1 * 8.0f / (float)F_CPU; // trajanje u sekundama
			
            // v_z = 343.0f;  // za korekciju temperature
            // d1 = t_echo / 2 * v_z * 100; // formula
>>>>>>> Stashed changes
            d1 = (t_echo / 2.0f) * 343.0f * 100.0f;
            hcsr04_measured1 = false;
        }

<<<<<<< Updated upstream
        // Ako je mjerenje završeno za senzor 2
=======
        // ako je senzor 2 napravio mjerenje
>>>>>>> Stashed changes
        if (hcsr04_measured2) {
            t_echo = (float)broj_impulsa2 * 8.0f / (float)F_CPU;  // s
            d2 = (t_echo / 2.0f) * 343.0f * 100.0f;
            hcsr04_measured2 = false;
        }

<<<<<<< Updated upstream
        // LCD ispis u istom stilu
=======
        // ispis informacija na LCD displeju
>>>>>>> Stashed changes
        if (time_delay == 0) {
            lcd_clrscr();
            lcd_home();
            lcd_print("d1 = %0.2f cm\n", d1);
            lcd_print("d2 = %0.2f cm",  d2);
        }

        _delay_ms(10);
<<<<<<< Updated upstream
        if (time_delay > 10) {   // ~100ms
=======
        if (time_delay > 10) {   // 100ms
>>>>>>> Stashed changes
            time_delay = 0;
        } else {
			time_delay++;
		}
    }
}
