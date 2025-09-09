/*
Naslov: Parking senzori
Autor: Matko Manzin
*/

#include <avr/io.h>
#include "AVR VUB/avrvub.h"
#include <util/delay.h>
#include "LCD/lcd.h"
#include "Timer/timer.h"
#include "Interrupt/interrupt.h"
#include "ADC/adc.h"

volatile uint32_t broj_impulsa1 = 0; // broj impulsa tajmera 1 (trajanje ECHO impulsa) za senzor 1, za izracun udaljenosti
volatile uint32_t broj_impulsa2 = 0; // broj impulsa tajmera 1 (trajanje ECHO impulsa) za senzor 2, za izracun udaljenosti
volatile uint32_t broj_impulsa3 = 0; // broj impulsa tajmera 1 (trajanje ECHO impulsa) za senzor 3, za izracun udaljenosti
volatile bool hcsr04_measured1 = false; // pomocna varijabla, da znamo ako je mjerenje senzora 1 gotovo da bismo izacunali udaljenost
volatile bool hcsr04_measured2 = false; // pomocna varijabla, da znamo ako je mjerenje senzora 2 gotovo da bismo izacunali udaljenost
volatile bool hcsr04_measured3 = false; // pomocna varijabla, da znamo ako je mjerenje senzora 3 gotovo da bismo izacunali udaljenost
volatile uint8_t echo1_high = 0; // stanje ECHO signala za senzor 1, da znamo kada je signal u visokom stanju (1), a kada u niskom (0)
volatile uint8_t echo2_high = 0; // stanje ECHO signala za senzor 2, da znamo kada je signal u visokom stanju (1), a kada u niskom (0)
volatile uint8_t echo3_high = 0; // stanje ECHO signala za senzor 3, da znamo kada je signal u visokom stanju (1), a kada u niskom (0)

// predprocesorske direktive
#define LED_GREEN PF5 //PF5 je isto kao i LED_GREEN
#define LED_YELLOW PF6 //PF6 je isto kao i LED_YELLOW
#define LED_RED PF7 //PF7 je isto kao i LED_RED
#define BUZZER PB7 //PB7 je isto kao i BUZZER

// varijable za rad zujalice (BUZZER)
static uint8_t  buzz_on = 0; // stanje zujalice, 0 - iskljuceno, 1 - ukljuceno
static uint16_t buzz_phase_ms = 0; // proteklo vrijeme rada zujalice u trenutnoj fazi, ms
static uint16_t buzz_on_ms = 0; // vrijeme koliko zujalica treba raditi u ukljucenoj fazi, ms
static uint16_t buzz_off_ms = 0; // vrijeme koliko zujalica treba raditi u iskljucenoj fazi, ms
static uint8_t  last_zone = 0xFF; // koja je zona bila zadnja aktivna, inicijalizirano na 0xFF

ISR(PCINT0_vect) // prekidna rutina PCINT0, reagira na promjenu stanja pinova PB4, PB5, PB6
{
	uint8_t pins = PINB; // sprema trenutno stanje svih pinova PORTB

	// senzor 1 (lijevi)
	if (pins & (1 << PB4)) { // ako je PB4 u visokom stanju, samo ako je bio poslan TRIG na PD2
		if (echo1_high == 0) { // ako ECHO nije bio u visokom stanju (nije mjerio udaljenost)
			echo1_high = 1; // postaviti ECHO u visoko stanje
			TCNT1 = 0; // resetiranje tajmera 1, po?etak mjerenja
		}
		} else { // ako PB4 nije u visokom stanju
		if (echo1_high == 1) { // ako je ECHO bio u visokom stanju (mjerio udaljenost)
			echo1_high = 0; // postaviti ECHO u nisko stanje (zavrsiti mjerenje udaljenosti)
			broj_impulsa1 = TCNT1; // spremanje broja impulsa tajmera 1, sluzi za racunanje t_echo varijable
			hcsr04_measured1 = true; // pomocna varijabla, mjerenje senzora 1 je gotovo i moze se racunati udaljenost
		}
	}
	// senzor 2 (centralni), sve radi na isti nacin kao i senzor 1
	if (pins & (1 << PB5)) {
		if (echo2_high == 0) {
			echo2_high = 1;
			TCNT1 = 0;
		}
		} else {
		if (echo2_high == 1) {
			echo2_high = 0;
			broj_impulsa2 = TCNT1;
			hcsr04_measured2 = true;
		}
	}
	// senzor 3 (desni), sve radi na isti nacin kao i senzor 1
	if (pins & (1 << PB6)) {
		if (echo3_high == 0) {
			echo3_high = 1;
			TCNT1 = 0;
		}
		} else {
		if (echo3_high == 1) {
			echo3_high = 0;
			broj_impulsa3 = TCNT1;
			hcsr04_measured3 = true;
		}
	}
}

void inicijalizacija()
{
	lcd_init(); // inicijalizacija lcd ekrana, makronaredbe

	// tajmer 1 je postavljen u normalan nacin rada, WGM bitovi = 0 0 0
	TCCR1A &= ~((1 << WGM00) | (1 << WGM01)); // WGM00 = 0, WGM01 = 0, TCCR1A registar
	TCCR1B &= ~(1 << WGM02); // WGM02 = 0, TCCR1B registar

	// prescaler_8, CS01 mora biti 1, ocitano iz tablice u knjizi
	TCCR1B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00)); // postavljanje svih CS bitova u 0, TCCR1B registar
	TCCR1B |= (1 << CS01); // postavljanje CS01 bita u 1

	// TRIG pinovi postavljeni kao izlazi
	DDRD |= (1 << PD2) | (1 << PD3) | (1 << PD4);

	// ECHO pinovi postavljeni kao ulazi
	DDRB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6));
	PORTB &= ~((1 << PB4) | (1 << PB5) | (1 << PB6)); // iskljuceni su pull-up otpornici

	// omoguceni su pin-change prekidi za PB4, PB5, PB6
	PCMSK0 |= (1 << PCINT4) | (1 << PCINT5) | (1 << PCINT6);
	PCICR  |= (1 << PCIE0); // omogucen je PCINT0 prekid

	// LED pinovi postavljeni kao izlazi, PF5, PF6, PF7
	DDRF |= (1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED);
	PORTF &= ~((1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED)); // pocetno stanje svih LED dioda je iskljuceno

	// BUZZER pin, PB7 postavljen kao izlaz
	DDRB  |= (1 << BUZZER);
	PORTB |= (1 << BUZZER); // PB7, zujalicu je potrebno staviti u nisko stanje da se ukljuci, tako da je sada u visokom (iskljucenom) stanju

	sei(); // globalno omoguceni prekidi
}

// metoda za generiranje TRIG impulsa za odabrani senzor prema 'id' parametru
void hcsr04_trigg(uint8_t id) // potrebno proslijediti broj kao parametar kod pozivanja metode
{
	switch (id) // odabire switch-case ovisno o proslijedenom broju u parametru
	{
		case 0: // ako je proslijeden broj 0
		PORTD |= (1 << PD2); // TRIG pin, PD2, senzora 1 ide u visoko stanje (pocetak TRIG impulsa)
		_delay_us(10); // u visokom stanju je 10 mikrosekundi
		PORTD &= ~(1 << PD2); // TRIG pin, PD2, senzora 1 ide nazad u nisko stanje (kraj TRIG impulsa)
		break;

		case 1: // ako je proslijeden broj 1
		PORTD |= (1 << PD3); // TRIG pin, PD3, senzora 2 ide u visoko stanje (pocetak TRIG impulsa)
		_delay_us(10); // u visokom stanju je 10 mikrosekundi
		PORTD &= ~(1 << PD3); // TRIG pin, PD3, senzora 2 ide nazad u nisko stanje (kraj TRIG impulsa)
		break;

		case 2: // ako je proslijeden broj 2
		PORTD |= (1 << PD4); // TRIG pin, PD4, senzora 3 ide u visoko stanje (pocetak TRIG impulsa)
		_delay_us(10); // u visokom stanju je 10 mikrosekundi
		PORTD &= ~(1 << PD4); // TRIG pin, PD4, senzora 3 ide nazad u nisko stanje (kraj TRIG impulsa)
		break;
	}
}

// metoda za odabir ritma zujalice ovisno o proslijedenom parametru 'zone'
static void buzzer_set_pattern(uint8_t zone) { // potrebno proslijediti broj kao parametar kod pozivanja metode

	// zona: 0 = iskljuceno, 1 = zelena zona; sporo, 2 = zuta zona; ubrzano, 3 = crvena zona; jako brzo
	if (zone == 1) { // ako je proslijeden broj 1, zelena zona, sporo
		buzz_on_ms  = 500; // zujalica mora biti upaljena 500 milisekundi
		buzz_off_ms = 100; // zujalica mora biti iskljucena 100 milisekundi
		} else if (zone == 2) {  // ako je proslijeden broj 2, zuta zona, ubrzano
		buzz_on_ms  = 300; // zujalica mora biti upaljena 300 milisekundi
		buzz_off_ms = 90; // zujalica mora biti iskljucena 90 milisekundi
		} else if (zone == 3) {  // ako je proslijeden broj 3, crvena zona, jako brzo
		buzz_on_ms  = 120; // zujalica mora biti upaljena 120 milisekundi
		buzz_off_ms = 80; // zujalica mora biti iskljucena 80 milisekundi
		} else { // ako je proslijeden broj 0, iskljucena je zujalica
		buzz_on_ms  = 0; // zujalica mora biti upaljena 0 milisekundi
		buzz_off_ms = 0; // zujalica mora biti iskljucena 0 milisekundi
	}

	if (buzz_on_ms > 0) { // ako je vrijeme kada zujalica mora biti ukljucena veca od 0
		buzz_on = 1; // zujalica mora biti ukljucena
		} else {
		buzz_on = 0; // ako vrijeme nije vece od 0, onda mora biti iskljucena
	}

	buzz_phase_ms = 0; // brojac trajanja trenutne faze

	if (buzz_on = 0) { // ako je stanje zujalice iskljuceno
		PORTB |= (1 << BUZZER); // postavi PB7 u visoko stanje (iskljuciti buzzer)
	}
}

// metoda za upravljanje stanjem zujalice
static void buzzer_update(uint16_t dt_ms) { // potrebno je proslijediti broj kao parametar
	if (buzz_on_ms == 0 && buzz_off_ms == 0) { // ako je vrijeme kada zujalica treba biti ukljucena i iskljucena jednaka 0
		PORTB |= (1 << BUZZER);  // postaviti PB7 u visoko stanje
		return;
	}
	if (buzz_on == 1) { // ako je stanje zujalice 1
		PORTB &= ~(1 << BUZZER); // postaviti PB7 u nisko stanje (ukljuciti zujalicu)
		} else {
		PORTB |= (1 << BUZZER);  // ako je stanje zujalice 0, PB7 postaviti u visoko stanje
	}

	buzz_phase_ms += dt_ms; // vrijeme provedeno u fazi poprima vrijednost proslijedenog parametra dt_ms

	if (buzz_on == 1) { // ako je stanje zujalice 1, ukljucena
		if (buzz_phase_ms >= buzz_on_ms) { // i ako je vrijeme provedeno u trenutnoj fazi vece ili jednako vremenu kada zujalica mora biti ukljucena (ritam)
			buzz_phase_ms = 0; buzz_on = 0; // postavi 'buzz_phase_ms' nazad u 0 i napravi toggle 'buzz_on' varijable (iskljuci zujalicu)
		}
		} else { // ako je stanje zujalice 0, iskljucena
		if (buzz_phase_ms >= buzz_off_ms) { // i ako je vrijeme provedeno u trenutnoj fazi vece ili jednako vremenu kada zujalica mora biti iskljucena (ritam)
			buzz_phase_ms = 0; buzz_on = 1; // postavi 'buzz_phase_ms' nazad u 0 i napravi toggle 'buzz_on' varijable (ukljuci zujalicu)
		}
	}
}

int main(void)
{
	inicijalizacija();

	float d1 = 0, d2 = 0, d3 = 0; // varijable za udaljenost
	float t_echo; // vrijeme trajanja ECHO impulsa koje predstavlja ukupno vrijeme od senzora do prepreke i nazad
	uint16_t time_delay = 0; // pomocna varijabla koja sluzi za sekvencijalno aktiviranje senzora svakih 50 ms
	uint8_t koji = 0; // varijabla za odabir aktivnog senzora, sluzi za sekvencijalno pobudivanje

	while (1) {
		// sekvencijalno aktiviranje senzora
		if (time_delay == 0) { // ako je prva iteracija ili je proslo 50 ms, aktiviraj TRIG port odredenog ultrazvucnog senzora
			hcsr04_trigg(koji); // metoda za aktiviranje TRIG senzora 10 mikrosekundi
			koji = (koji + 1) % 3; // odabir 'id' sljedeceg senzora (0 -> 1 -> 2 -> 0 -> ..)
		}
		if (hcsr04_measured1 == true) { // ako je senzor 1 ECHO pin otisao iz visokog u nisko stanje, zavrsilo je mjerenje za taj senzor
			t_echo = broj_impulsa1 * 8.0 / F_CPU; // FORMULA trajanja ECHO signala: broj_impulsa * 8.0 / F_CPU
			d1 = t_echo / 2.0 * 343.0 * 100.0; // FORMULA udaljenosti do prepreke: t_echo / 2.0 * 343.0 * 100, 343.0 je brzina zvuka na 20 stupnjeva
			hcsr04_measured1 = false; // nakon izracunate udaljenosti, resetiranje varijable da bi se omogucio daljni rad ovog senzora
		}
		if (hcsr04_measured2 == true) { // sve radi na isti nacin kao i senzor 1
			t_echo = broj_impulsa2 * 8.0 / F_CPU;
			d2 = t_echo / 2.0 * 343.0 * 100.0;
			hcsr04_measured2 = false;
		}
		if (hcsr04_measured3 == true) { // sve radi na isti nacin kao i senzor 1
			t_echo = broj_impulsa3 * 8.0 / F_CPU;
			d3 = t_echo / 2.0 * 343.0 * 100.0;
			hcsr04_measured3 = false;
		}

		static uint16_t lcd_brojac = 0;  // brojac za osvjezavanje LCD ekrana

		lcd_brojac += 10; // povecava brojac za 10 jer petlja traje 10 ms

		if (lcd_brojac >= 200) { // kada brojac dode do 200, nakon 200 ms
			lcd_brojac = 0; // resetiranje brojaca

			lcd_clrscr(); // obrisi sve na ekranu
			lcd_home(); // postaviti pokazivac u prvi redak, prvi stupac (0, 0)

			lcd_gotoxy(0, 0); // postaviti pokazivac na koordinate x = 0, y = 0 (1. red, 1. stupac)
			lcd_print("L:%0.1f  D:%0.1f", d1, d3); // ispisi na ekranu informaciju o udaljenosti

			lcd_gotoxy(1, 0); // postaviti pokazivac na koordinate x = 1, y = 0 (2. red, 1. stupac)
			lcd_print("C:%0.1f", d2); // ispisi na ekranu informaciju o udaljenosti
		}

		// konfiguracija zona
		float min_dist = d1; // inicijalno najmanja udaljenost je lijevi senzor
		if (d2 < min_dist) {
			min_dist = d2; // ako centralni senzor ima manju udaljenost od prepreke od lijevog senzora onda se uzima njegova udaljenost
		}
		if (d3 < min_dist) {
			min_dist = d3; // i ako desni senzor ima manju udaljenost od centralnog senzora onda se uzima njegova udaljenost
		}

		uint8_t zone = 0; // inicijalno zona je 0

		if (min_dist <= 200.0f && min_dist >= 60.0f) { // ako je najmanja udaljenost manja od 200 cm i veca od 60 cm
			zone = 1; // zona je 1, zelena zona
		}
		else if (min_dist < 60.0f && min_dist >= 20.0f) { // ako je najmanja udaljenost manja od 60 cm i veca od 20 cm
			zone = 2; // zona je 2, zuta zona
		}
		else if (min_dist < 20.0f && min_dist >= 0.0f) { // ako je najmanja udaljenost manja od 20 cm i veca od 0 cm
			zone = 3; // zona je 3, crvena zona
		}

		PORTF &= ~((1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED)); // postavi sve LED diode u 0

		if (zone == 1) {
			PORTF |= (1 << LED_GREEN); // ako je zona 1 onda se upali zelena dioda
			} else if (zone == 2) {
			PORTF |= (1 << LED_YELLOW); // ako je zona 2 onda se upali zuta dioda
			} else if (zone == 3) {
			PORTF |= (1 << LED_RED); // ako je zona 3 onda se upali crvena dioda
		}

		// odredivanje ritma zujalice ovisno o zoni
		if (zone != last_zone) { // ako je trenutna zona razlicita od prosle zone
			buzzer_set_pattern(zone); // promjeniti ritam zujalice u tu novu zonu
			last_zone = zone; // trenutna zona je sada prosla zona
		}

		buzzer_update(10); // dodaje 10 ms u 'buzz_phase_ms' varijablu i svakom iteracijom petlje se 'buzz_phase_ms' povecava i kada dosegne zadano trajanje faze prelazi u sljede?u fazu

		_delay_ms(10); // postavljeno kasnjenje petlje na 10 ms

		time_delay++; // povecaj pomocnu varijablu za sekvencijsko aktiviranje senzora
		if (time_delay > 4) { // ako je ovo 5. iteracija petlje
			time_delay = 0; // resetiraj pomocnu varijablu 'time_delay' koja sluzi za sekvencijsko aktiviranje senzora
		}
	}
}