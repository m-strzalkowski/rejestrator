#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "LSM9DS1_Types.h"
#include "LSM9DS1.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
/*
 * isr.c:
 *	Wait for Interrupt test program - ISR method
 *
 *	How to test:
 *	  Use the SoC's pull-up and pull down resistors that are avalable
 *	on input pins. So compile & run this program (via sudo), then
 *	in another terminal:
 *		gpio mode 0 up
 *		gpio mode 0 down
 *	at which point it should trigger an interrupt. Toggle the pin
 *	up/down to generate more interrupts to test.
 *
 * Copyright (c) 2013 Gordon Henderson.
 ***********************************************************************
*************************************************************
 */




// What GPIO input are we using?
//	This is a wiringPi pin number

#define	BUTTON_PIN	0

// globalCounter:
//	Global variable to count interrupts
//	Should be declared volatile to make sure the compiler doesn't cache it.

static volatile int globalCounter = 0 ;


/*
 * myInterrupt:
 *********************************************************************************
 */
void myInterrupt (void)
{
  if(digitalRead(0) == HIGH)//rising
  {printf("Rising\n");}
  else{printf("Falling\n");}
  ++globalCounter ;
}
///OBSLUGA ZDARZEN NA GUZIKACH///

#define IGNORUJ_ZMIANY_SZYBSZE_NIZ 100000//mikrosekund - 10milisek
#define IGNORUJ_ZMIANY_SZYBSZE_NIZ_NA_DUZYM 500000 //- 50 milisek
//DUZE CZERWONE GUZIKI//
//GPIO (+PULL UP) --- 10kOm --- GUZIK --- UZIEMIENIE
// Czas ostatniego zdarzenia
struct timeval ostatnieDuze;
static volatile int ostatniDuzy = 0;
int ostatni_duzy(void){return ostatniDuzy;}

//zwraca, czy w danym momencie jest nacisnieta kombinacja guzikow, ktora ma wyzerowac ostatni_duzy
int kombinacjaResetuDuzych();

// Obsługa nacasniecia jednego z duzych czerwonych guziczkow (Liczy sie tylko to, ktory zostal nacisniety ostatnio)
void isrDuzych(int pin) {
  printf("\nisrD(%d)\n",pin);
	struct timeval teraz;
	unsigned long dt;

	gettimeofday(&teraz, NULL);

	// Roznica czasu w mikrosekundach
	dt = (teraz.tv_sec * 1000000 + teraz.tv_usec) - (ostatnieDuze.tv_sec * 1000000 + ostatnieDuze.tv_usec);
  //Odfiltruj kazde zmiany szybsze niz ustalona wartosc (np. ustalanie sie stanu na kontaktach przycisku)
	if (dt > IGNORUJ_ZMIANY_SZYBSZE_NIZ) 
  {
    if(kombinacjaResetuDuzych()){ostatniDuzy=0;}
    printf("\nOSTATNI:%d->%d\n", ostatniDuzy,pin);
    ostatniDuzy = pin;
	}
        // Zapisz czas ostatniej zmiany
	ostatnieDuze = teraz;
}

void ustawPinOdDuzego(int pin, void (*isr)())
{
  pinMode(pin, INPUT);
  pullUpDnControl(pin, PUD_UP);
  if (wiringPiISR(pin, INT_EDGE_FALLING, isr) < 0)
  {
    fprintf (stderr, "Nie udalo sie podczepic procedury obslugi przerwania dla pinu %d: %s\n", pin,strerror(errno)) ;
    return;
  }
}

//DEFINICJA KONFIGURACJI CZERWONYCH GUZIKOW
//!!! PINY W NUMERACJI OFICJALNEJ, NIE WIRINGPI !!!//
#define PIN_DUZEGO_A 26//25
#define PIN_DUZEGO_B 19//24
#define PIN_DUZEGO_C 13//23
#define PIN_DUZEGO_D 6//22
void isrDuzegoA(void){isrDuzych(PIN_DUZEGO_A);}
void isrDuzegoB(void){isrDuzych(PIN_DUZEGO_B);}
void isrDuzegoC(void){isrDuzych(PIN_DUZEGO_C);}
void isrDuzegoD(void){isrDuzych(PIN_DUZEGO_D);}

int kombinacjaResetuDuzych(){return 0;}

#define PIN_DUZEGO_PRZELACZNIKA 5//21
#define PIN_MALEGO_PRZELACZNIKA 12//26
struct timeval ost_duzy_p;
static void (*gdy_niski_na_duzym)(void) = NULL;
static void (*gdy_wysoki_na_duzym)(void) = NULL;
void rozdzielacz_duzego_przelacznika(void)
{
  struct timeval teraz;
	unsigned long dt;
  gettimeofday(&teraz, NULL);
  // Roznica czasu w mikrosekundach
	dt = (teraz.tv_sec * 1000000 + teraz.tv_usec) - (ost_duzy_p.tv_sec * 1000000 + ost_duzy_p.tv_usec);
  //Odfiltruj kazde zmiany szybsze niz ustalona wartosc (np. ustalanie sie stanu na kontaktach przycisku)
	if (dt > IGNORUJ_ZMIANY_SZYBSZE_NIZ_NA_DUZYM * 10) 
  {
    usleep(1000 * 5);
    if(digitalRead(PIN_DUZEGO_PRZELACZNIKA) == LOW)
    {printf("\n!@NIS\n"); gdy_niski_na_duzym();}else{printf("\n!@WYS\n");gdy_wysoki_na_duzym();}
	}
        // Zapisz czas ostatniej zmiany
	ost_duzy_p = teraz;
}
void ustawPinDuzegoPrzelacznika(void(*gdy_niski)(void), void(*gdy_wysoki)(void))
{
  printf("ustawPinDuzegoPrzelacznika\n");
  gdy_niski_na_duzym = gdy_niski;
  gdy_wysoki_na_duzym = gdy_wysoki;
  pinMode(PIN_DUZEGO_PRZELACZNIKA, INPUT);
  pullUpDnControl(PIN_DUZEGO_PRZELACZNIKA, PUD_UP);
  if (wiringPiISR(PIN_DUZEGO_PRZELACZNIKA, INT_EDGE_BOTH, rozdzielacz_duzego_przelacznika) < 0)
  {
    fprintf (stderr, "Nie udalo sie podczepic procedury obslugi przerwania dla pinu %d: %s\n", PIN_DUZEGO_PRZELACZNIKA,strerror(errno)) ;
    return;
  }
}

/*IDIOTYCZNA KOPIA DLA DRUGIEGO, vide:https://forums.raspberrypi.com/viewtopic.php?t=104814*/
/*doslownie duzy->maly duzego->malego*/
struct timeval ost_maly_p;
static void (*gdy_niski_na_malym)(void) = NULL;
static void (*gdy_wysoki_na_malym)(void) = NULL;
void rozdzielacz_malego_przelacznika(void)
{
  struct timeval teraz;
	unsigned long dt;
  gettimeofday(&teraz, NULL);
  // Roznica czasu w mikrosekundach
	dt = (teraz.tv_sec * 1000000 + teraz.tv_usec) - (ost_maly_p.tv_sec * 1000000 + ost_maly_p.tv_usec);
  //Odfiltruj kazde zmiany szybsze niz ustalona wartosc (np. ustalanie sie stanu na kontaktach przycisku)
	if (dt > IGNORUJ_ZMIANY_SZYBSZE_NIZ) 
  {
    usleep(1000 * 5);
    if(digitalRead(PIN_MALEGO_PRZELACZNIKA) == LOW)
    {gdy_niski_na_malym();}else{gdy_wysoki_na_malym();}
	}
        // Zapisz czas ostatniej zmiany
	ost_maly_p = teraz;
}
void ustawPinMalegoPrzelacznika(void(*gdy_niski)(void), void(*gdy_wysoki)(void))
{
  gdy_niski_na_malym = gdy_niski;
  gdy_wysoki_na_malym = gdy_wysoki;
  pinMode(PIN_MALEGO_PRZELACZNIKA, INPUT);
  pullUpDnControl(PIN_MALEGO_PRZELACZNIKA, PUD_UP);
  if (wiringPiISR(PIN_MALEGO_PRZELACZNIKA, INT_EDGE_BOTH, rozdzielacz_malego_przelacznika) < 0)
  {
    fprintf (stderr, "Nie udalo sie podczepic procedury obslugi przerwania dla pinu %d: %s\n", PIN_MALEGO_PRZELACZNIKA,strerror(errno)) ;
    return;
  }
}
/*KONIEC IDIOTYCZNEJ KOPII*/

//FUNKCJA USTAWIAJACA CALY ZESTAW CZERWONYCH GUZIKOW
void ustawPinyDuzych()
{
  ustawPinOdDuzego(PIN_DUZEGO_A, &isrDuzegoA);
  ustawPinOdDuzego(PIN_DUZEGO_B, &isrDuzegoB);
  ustawPinOdDuzego(PIN_DUZEGO_C, &isrDuzegoC);
  ustawPinOdDuzego(PIN_DUZEGO_D, &isrDuzegoD);
}
#define SYMBOL_NIEUSTALONEGO_DUZEGO "NIEZNANY"
const char * symbolOstatniegoDuzego()
{
  switch(ostatni_duzy())
  {
    case PIN_DUZEGO_A: return "A";
    case PIN_DUZEGO_B: return "B";
    case PIN_DUZEGO_C: return "C";
    case PIN_DUZEGO_D: return "D";
    default: return SYMBOL_NIEUSTALONEGO_DUZEGO;
  }
}

void ustawWiringPi(void)
{
  //if (wiringPiSetup () < 0)
  if (wiringPiSetupGpio() < 0)
  {
    fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno)) ;
    exit(EXIT_FAILURE);
  }
}