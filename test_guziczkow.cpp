#include <stdio.h>
#include "guziczki.hpp"
#include "ekran.hpp"
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <wiringPi.h>

#define COUNT_KEY 0

void gdy_duzy_niski(void)
{
    printf("\nDUZY PRZELACZNIK W STANIE NISKIM\n\n");
}
void gdy_duzy_wysoki(void)
{
    printf("\nDUZY PRZELACZNIK W STANIE WYSOKIM\n\n");
}

void gdy_maly_niski(void)
{
    printf("\nMALY PRZELACZNIK W STANIE NISKIM\n\n");
}
void gdy_maly_wysoki(void)
{
    printf("\nMALY PRZELACZNIK W STANIE WYSOKIM\n\n");
}

int tmax, t0;
void odswiez_ekran()
{
    char buf[2048];
    piLock(COUNT_KEY);
    int min = (tmax-t0)/60;
    int sek = (tmax-t0)%60;
    piUnlock(COUNT_KEY);
    sprintf(buf," %2d:%2d ", min,sek);
    ekran_nagrywanie((char *)symbolOstatniegoDuzego(), buf);
}

PI_THREAD (obsluga_ekranu)//ekran jest powolny...
{
    while(1)
    {
        odswiez_ekran();
        usleep(1000 * 500);//0,5s
    }
}

int main (void)
{
  ustawWiringPi();
  ustawPinDuzegoPrzelacznika(gdy_duzy_niski, gdy_duzy_wysoki);
  ustawPinMalegoPrzelacznika(gdy_maly_niski, gdy_maly_wysoki);
  ustawPinyDuzych();

  int st = piThreadCreate(obsluga_ekranu) ;
  if (st != 0)printf("Nie udalo sie uruchomic watku");
  /*if (wiringPiISR (BUTTON_PIN, INT_EDGE_FALLING, &myInterrupt) < 0)
  {
    fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno)) ;
    return 1 ;
  }*/



  //SETISR(0);
  //SETISRS;
  //printf("%d", pinA);
/*int myCounter = 0 ;
  for (;;)
  {
    printf("Waiting ... ") ; fflush(stdout) ;
    printf("*%d %d", ostatni_duzy(), myCounter);
    while (myCounter == ostatni_duzy())
      {delay(100) ;}

    
    myCounter = ostatni_duzy() ;
    printf(" Done. counter: %5d %d\n", ostatni_duzy(), myCounter) ;
  }
*/
  printf("TYLKO NIE GUZICZKI!\n");
  int i=0;
  t0=0;
  while(1)
  {
    //delay(100);
    usleep(1000 * 100);
    printf("\r%dostD:%d %s", i++,ostatni_duzy(), symbolOstatniegoDuzego());
    fflush(stdout);
    tmax=i;
    //odswiez_ekran();
  }
    return 0 ;
}
