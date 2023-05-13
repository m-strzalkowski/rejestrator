/*
 *** NAGRYWAJ.cpp ***
 Inicjalizacja, kalibracja oraz cykliczne odczyty z IMU, guzikow, wypisywanie statusu na ekraniku;
 oraz zapis tego, co odczytal w postaci binarnej do pliku ./dane/zapis.
 Domyslnie ma chodzic uruchamiany przez smaczdemona po przestawieniu przelacznika.
 (vide: man nagrywaj)
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <wiringPi.h>
#include "LSM9DS1_Types.h"
#include "LSM9DS1.h"
#include "rejestrator.h"
#include "guziczki.hpp"
#include "ekran.hpp"

#define MAKS_ROZMIAR_ZAPISU 536870912 //512 MB
#define	COUNT_KEY	0

int deskryptor_zapisu = 0;//pliku, do ktorego trafiaja pomiary
long int wielkosc_zapisu=-1;//wielkosc tego pliku

long int t0=-1;//czas poczatkowy
long int tmax=-1;

int czestotliwosc = 20;//Hz //docelowa czestotliowsc
const float histereza = 0.5;//do ewentualnego ustalania czestotliowsci
int czest_do_spania = 30;//spi przez 1s/czest_do_spania
float rzeczyw_czest, wazona_czest; 
//przelaczniki:
bool wypisuj = false;
bool zapisuj = true;
bool ekran = true;
bool regulacja_czestotliowsci=false;
bool wylaczaj_fizycznym_przelacznikiem=true;

//konczy proces zamykajac plik
void zakoncz()
{
    int s;
    ekran_godzina();
    fflush(stdout);
    fprintf(stderr, "Otrzymano SIGINT/SIGTERM po %lds = %fmin...\n", tmax-t0, 1.0*(tmax-t0)/60);
    s=close(deskryptor_zapisu);
    fprintf(stderr, "zamknieto plik: %d\n",s);
    fflush(stderr);
    exit(EXIT_SUCCESS);
}
//obsluga sygnalow
void sig_handler(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM: zakoncz();
    default:
        fprintf(stderr, "Nie tego sie spodziewalem!\n");
        abort();
    }
}
//wypisywanie rzeczy na ekraniku - (etykieta \n czas aktualnej sesji/przyblizona dlugosc calego zapisu w minutach)
void odswiez_ekran()
{
    char buf[2048];
    piLock(COUNT_KEY);
    int min = (tmax-t0)/60;
    int sek = (tmax-t0)%60;
    piUnlock(COUNT_KEY);
    if(regulacja_czestotliowsci)
    { sprintf(buf,"%02d:%02d@%2.1f", min,sek,rzeczyw_czest); }
    else{sprintf(buf,"%02d:%02d/%.0f", min,sek, ((float)(wielkosc_zapisu/sizeof(Odczyt)))/czestotliwosc/60 );}
    ekran_nagrywanie((char *)symbolOstatniegoDuzego(), buf);
}
PI_THREAD (obsluga_ekranu)//ekran jest powolny... (wiec osobny watek)
{
    while(1)
    {
        odswiez_ekran();
        usleep(1000 * 100);//0,1s
    }
}

//callbacki do przyciskow - ignorowanie oprocz przelaczenia z powrotem 'malego' - wtedy sie wylacza
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
    if(wylaczaj_fizycznym_przelacznikiem)zakoncz();
}

//main
int main(int argc, char *argv[]) {
    char * sciezka_zapisu = (char *)"dane/zapis";
    int opt;
    //opcje, opisane tez w podreczniku - man nagrywaj
    while ((opt = getopt(argc, argv, "pnESr")) != -1) {
        switch (opt) {
        case 'p': wypisuj = true; fprintf(stderr,"--wypisuj-odczyty ");break;
        case 'n': zapisuj = false;fprintf(stderr,"--nie-zapisuj "); break;
        case 'E': ekran = false; fprintf(stderr,"--nie-uzywaj-ekranu ");break;
        case 'S': wylaczaj_fizycznym_przelacznikiem = false; fprintf(stderr,"--nie-wylaczaj-fizycznym-przelacznikiem ");break;
        case 'r': regulacja_czestotliowsci = true; fprintf(stderr,"--regulacja-czestotliowsci ");break;
        default:
            fprintf(stderr, "Uzycie: %s [-p -n -E -S -r] [plik wyjsciowy]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if(optind < argc)
    {
        sciezka_zapisu = argv[optind];
    }
    fprintf(stderr, "Zapisuj:%d Wypisuj:%d\n", zapisuj,wypisuj);
    //zalozenie procedury obslugujacej sygnaly
    signal(SIGINT,sig_handler);
    signal(SIGTERM,sig_handler);

    //otwarcie pliku z zapisem pomiarow
    deskryptor_zapisu = open(sciezka_zapisu,  O_CREAT | O_APPEND | O_RDWR, 0666);
    if(deskryptor_zapisu < 0)
    {
        fprintf(stderr, "ERROR OPENING FILE:%d %s", deskryptor_zapisu, sciezka_zapisu);
        exit(EXIT_FAILURE);
    }
    printf("deskryptor_zapisu:%d",deskryptor_zapisu);

    //Wielkosc pliku
    struct stat zapisu_stat;
    stat(sciezka_zapisu, &zapisu_stat);
    wielkosc_zapisu = zapisu_stat.st_size;
    printf("do pliku o wielkosci:%ld",wielkosc_zapisu);

    //wy = fopen(sciezka_zapisu, "w+");

    //while(1){usleep(1000000);}
    printf("NAGRYWAJKA\n");
    
    Odczyt odczyt;
    LSM9DS1 imu1(IMU_MODE_I2C, 0x6b, 0x1e);
    LSM9DS1 imu2(IMU_MODE_I2C, 0x6a, 0x1c);
    const int imu_num=2;

    LSM9DS1 * imu = NULL;
    LSM9DS1 * imu_tab[] = {&imu1, &imu2};
    
    for(int i=0; i<imu_num && (imu = imu_tab[i]) ; i++)//dla obu imu
    {
        printf("\nbegin");
        //imu->begin();
        if (!imu->begin()) {
            fprintf(stderr, "Failed to communicate with LSM9DS1 %d.\n", i);
            exit(EXIT_FAILURE);
        }
        printf("\nabegin");

        imu->calibrate();
    }
    
    wypelnij_odczyt(&odczyt, &imu1, &imu2, ostatni_duzy(), 0);

    //PO PIERWSZYM ODCZYCIE (w innej kolejnosci nie dziala, bo bilioteka do imu chyba psuje callbacki)
    //** INICJALIZACJA GUZICZKOW **//
    //ustawWiringPi(); - niepotrzebne przez LSM9DS1
    ustawPinDuzegoPrzelacznika(gdy_duzy_niski, gdy_duzy_wysoki);
    ustawPinMalegoPrzelacznika(gdy_maly_niski, gdy_maly_wysoki);
    ustawPinyDuzych();
    //**  KONIEC INICJALIZACJI GUZICZKOW **//

    //** OSOBNY WATEK ODSWIEAZJACY EKRAN **//
    int st = piThreadCreate(obsluga_ekranu) ;
    if (st != 0)printf("Nie udalo sie uruchomic watku");
    
    //czas rozpoczecia
    t0 = odczyt.czas.tv_sec;

    //glowna petla
    int status,zapisane;
    int wiersze=0;
    long int ms1,ms2,dms;
    for (;;wiersze++) {
        try{
            //pomiar za pomoca imu
            for(int i=0; i<imu_num && (imu = imu_tab[i]) ; i++)//dla obu imu
            {
                while (!imu->gyroAvailable()) ;
                imu->readGyro();
                while(!imu->accelAvailable()) ;
                imu->readAccel();
                while(!imu->magAvailable()) ;
                imu->readMag();
            }
        }
        catch (int kodWyjatku)
        {
//LSM9DS1.cpp:1106 if ((read(_fd, temp_dest, 6)) < 0) {
//LSM9DS1.cpp:1107        //fprintf(stderr, "Error: read value\n");
//LSM9DS1.cpp:1108        throw 999;
            if(kodWyjatku == 999)
            {
                fprintf(stderr, "\nBLAD ODCZYTU Z KTOREGOS IMU!");
            }
            else{
                fprintf(stderr, "\nNie spodziewalem sie wyjatku typu 'int' o wartosci %d, podczas odczytu z IMU.", kodWyjatku);
            }
            fsync(deskryptor_zapisu);//jak fflush?
            zakoncz();
        }
        
        ms1 = odczyt.czas.tv_usec;
        wypelnij_odczyt(&odczyt, &imu1, &imu2, ostatni_duzy(), 0);
        ms2 = odczyt.czas.tv_usec;

        //prymitywna regulacja czestotliwosci odswiezania
        if(ms2<ms1)ms2+=1000000;
        dms = ms2-ms1;
        if(dms>0 && regulacja_czestotliowsci)
        {
            rzeczyw_czest = 1.0/(1e-6*(dms));
            if(wypisuj)printf("\n%ld %ld %ld zmierzona:%fHZ spania:%dHz docelowa:%dHz df%f", ms1,ms2,dms, rzeczyw_czest, czest_do_spania,czestotliwosc, abs(1.0*rzeczyw_czest-czestotliwosc));
            if(abs(1.0*rzeczyw_czest-czestotliwosc)>histereza)
            {
                if(rzeczyw_czest>czestotliwosc){if(wypisuj)printf("ZMNIEJSZANIE"); czest_do_spania--;}
                if(rzeczyw_czest<czestotliwosc){if(wypisuj)printf("ZWIEKSZANIE"); czest_do_spania++;}
            }
            printf("\n");
            
        }

        //aktualny czas (piLock, bo drugi watek piszacy po ekranie tez go uzywa)
        piLock(COUNT_KEY);
        tmax = odczyt.czas.tv_sec;
        piUnlock(COUNT_KEY);

        if(zapisuj)
        {
            //nie chcemy pozwolic, zeby cala karta pamieci zostala zjedzona przez nagrywajke.
            if(wielkosc_zapisu > MAKS_ROZMIAR_ZAPISU )
            {
                fprintf(stderr, " %ld/%d Przekroczono maksymalny rozmiar zapisu. Konczenie.\n", wielkosc_zapisu, MAKS_ROZMIAR_ZAPISU);
                close(deskryptor_zapisu);
                exit(2);
            }
            //sam zapis, z uzyciem write i upewnieniem sie, ze wszystkie bajty poszly.
            zapisane=0;
            do{
                status = write(deskryptor_zapisu,&odczyt, sizeof(odczyt));
                //status = write(deskryptor_zapisu,"The epoll API performs a similar task to poll(2): monitorin gmultiple file descriptors to see if I/O is possible on any ofthem.  The epoll API can be used either as an edge-triggered or alevel-triggered interface and scales well to large numbers of watched file descriptors.", sizeof(odczyt));
                if(status < (int)(sizeof(odczyt)))
                {
                    fprintf(stderr, "write error: %d:", status);
                    if(status<=0)
                    {
                        fprintf(stderr, " error/ 0 bytes written\n");
                    }
                    else{
                        zapisane +=status;
                        fprintf(stderr, " %d/%d bytes written\n", zapisane, sizeof(odczyt));
                    }
                }
                else{ zapisane +=status; }
                //printf("%d/%d\n", zapisane,sizeof(odczyt));
            }while(zapisane<(int)(sizeof(odczyt)));
            wielkosc_zapisu += sizeof(odczyt);
        }
        //printf("%d@\n",wiersze);
        
        if(wypisuj){wypisz_odczyt(stdout,&odczyt,0);}
        else{printf("\r %d wierszy, %d B %ld B", wiersze, wiersze *sizeof(odczyt), wielkosc_zapisu);}
        
        usleep(1000000/czest_do_spania);
    }
    close(deskryptor_zapisu);
    exit(EXIT_SUCCESS);
}

