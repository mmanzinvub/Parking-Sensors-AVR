/*
 Naslov: MIKROUPRAVLJACI - Programiranje mikrokontrolera AVR familije
 Autor: prilagodba za 2x HC-SR04 (PD2/PB4 i PD3/PB5)

 Vježba 12.3.1 (format zadržan, ECHO preko PCINT na PB4/PB5)
*/

#include <avr/io.h>
#include "AVR VUB/avrvub.h"
#include <util/delay.h>
#include "LCD/lcd.h"
#include "Timer/timer.h"
#include "Interrupt/interrupt.h"
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
int main(void)
{
    inicijalizacija();

    float d1 = 0.0f, d2 = 0.0f;    // udaljenosti u cm
    float t_echo;                  // trajanje impulsa na Echo pinu (s)
    uint16_t time_delay = 0;       // brojanje ~100ms
    // uint16_t ADC_4;             // za temp. korekciju ako koristiš ADC
    // float T, v_z;

    uint8_t koji = 0;              // 0 -> triggaj senzor 1, 1 -> senzor 2

    while(1)
    {
        // Naizmjeni?no šalji TRIG svakih ~100ms (jednostavno, stabilno)
        if (time_delay == 0) {
            if (koji == 0) { hcsr04_trigg1(); }
            else            { hcsr04_trigg2(); }
            koji ^= 1; // prebacuj 0/1
        }

        // Ako je mjerenje završeno za senzor 1
        if (hcsr04_measured1) {
            // t_echo = broj_impulsa * PRESCALER / F_CPU (kao u skripti)
            t_echo = (float)broj_impulsa1 * 8.0f / (float)F_CPU;  // s
            // v_z = 343.0f;  // m/s (bez korekcije)
            // d1 = t_echo / 2 * v_z * 100;
            d1 = (t_echo / 2.0f) * 343.0f * 100.0f;
            hcsr04_measured1 = false;
        }

        // Ako je mjerenje završeno za senzor 2
        if (hcsr04_measured2) {
            t_echo = (float)broj_impulsa2 * 8.0f / (float)F_CPU;  // s
            d2 = (t_echo / 2.0f) * 343.0f * 100.0f;
            hcsr04_measured2 = false;
        }

        // LCD ispis u istom stilu
        if (time_delay == 0) {
            lcd_clrscr();
            lcd_home();
            lcd_print("d1 = %0.2f cm\n", d1);
            lcd_print("d2 = %0.2f cm",  d2);
        }

        _delay_ms(10);
        if (time_delay > 10) {   // ~100ms
            time_delay = 0;
        } else {
			time_delay++;
		}
    }
}
