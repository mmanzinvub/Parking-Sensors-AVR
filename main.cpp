/*
 Tema: Naslov projektnog zadatka
 Autor: Student 1, student 2, ....
 
 */ 

#include <avr/io.h>
#include "AVR VUB/avrvub.h"
#include <util/delay.h>

void inicijalizacija() {
	
	DDRB |= (1 << PB7); // pin PB7 izlazni pin
	PORTB |= (1 << PB7); // pin PB7 po?etno u visokom stanju

}

int main(void){

	inicijalizacija();

	while(1) {
		// program koji se izvršava u beskona?noj petlji
		
	}
}
