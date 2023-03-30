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

int zapis = 0;
long int wielkosc_zapisu=-1;

long int t0=-1;
long int tmax=-1;
int czestotliwosc = 20;//Hz //docelowa czestotliowsc
const float histereza = 0.5;
int czest_do_spania = 30;//spi przez 1s/czest_do_spania
float rzeczyw_czest, wazona_czest; 
bool wypisuj = false;
bool zapisuj = true;
bool ekran = true;
bool regulacja_czestotliowsci=false;
bool wylaczaj_fizycznym_przelacznikiem=true;
//konczy proces
void zakoncz()
{
    int s;
    ekran_godzina();
    fflush(stdout);
    fprintf(stderr, "Otrzymano SIGINT/SIGTERM po %lds = %fmin...\n", tmax-t0, 1.0*(tmax-t0)/60);
    s=close(zapis);
    fprintf(stderr, "zamknieto plik: %d\n",s);
    fflush(stderr);
    exit(EXIT_SUCCESS);
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM: zakoncz();
    default:
        fprintf(stderr, "Nie tego sie spodziewalem!\n");
        abort();
    }
}
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
PI_THREAD (obsluga_ekranu)//ekran jest powolny...
{
    while(1)
    {
        odswiez_ekran();
        usleep(1000 * 100);//0,1s
    }
}


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

int main(int argc, char *argv[]) {
    //FILE * wy = stdout;
    char * sciezka_zapisu = (char *)"dane/zapis";
    int opt;

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
    signal(SIGINT,sig_handler);
    signal(SIGTERM,sig_handler);

    zapis = open(sciezka_zapisu,  O_CREAT | O_APPEND | O_RDWR, 0666);
    if(zapis < 0)
    {
        fprintf(stderr, "ERROR OPENING FILE:%d %s", zapis, sciezka_zapisu);
        exit(EXIT_FAILURE);
    }
    printf("zapis:%d",zapis);

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
    //imu = imu_tab[1]; int i=0;
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

//PO PIERWSZYM ODCZYCIE
        //** INICJALIZACJA GUZICZKOW **//
    //ustawWiringPi();
    ustawPinDuzegoPrzelacznika(gdy_duzy_niski, gdy_duzy_wysoki);
    ustawPinMalegoPrzelacznika(gdy_maly_niski, gdy_maly_wysoki);
    ustawPinyDuzych();
    //**  KONIEC INICJALIZACJI GUZICZKOW **//

    //** OSOBNY WATEK ODSWIEAZJACY EKRAN **//
    int st = piThreadCreate(obsluga_ekranu) ;
    if (st != 0)printf("Nie udalo sie uruchomic watku");

    t0 = odczyt.czas.tv_sec;

    int status,zapisane;
    int wiersze=0;
    long int ms1,ms2,dms;
    for (;;wiersze++) {

        for(int i=0; i<imu_num && (imu = imu_tab[i]) ; i++)//dla obu imu
        {
            while (!imu->gyroAvailable()) ;
            imu->readGyro();
            while(!imu->accelAvailable()) ;
            imu->readAccel();
            while(!imu->magAvailable()) ;
            imu->readMag();
        }
        ms1 = odczyt.czas.tv_usec;
        wypelnij_odczyt(&odczyt, &imu1, &imu2, ostatni_duzy(), 0);
        ms2 = odczyt.czas.tv_usec;
        //dms = (ms2>ms1)? (ms2-ms1) : (ms2+1000000-ms1);
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

        piLock(COUNT_KEY);
        tmax = odczyt.czas.tv_sec;
        piUnlock(COUNT_KEY);
        if(zapisuj)
        {
            if(wielkosc_zapisu > MAKS_ROZMIAR_ZAPISU )
            {
                fprintf(stderr, " %ld/%d Przekroczono maksymalny rozmiar zapisu. Konczenie.\n", wielkosc_zapisu, MAKS_ROZMIAR_ZAPISU);
                close(zapis);
                exit(2);
            }
            zapisane=0;
            do{
                status = write(zapis,&odczyt, sizeof(odczyt));
                //status = write(zapis,"The epoll API performs a similar task to poll(2): monitorin gmultiple file descriptors to see if I/O is possible on any ofthem.  The epoll API can be used either as an edge-triggered or alevel-triggered interface and scales well to large numbers of watched file descriptors.", sizeof(odczyt));
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
        
        
        /*printf("\nAccel: %f, %f, %f [Gs] ", imu.calcAccel(imu.ax), imu.calcAccel(imu.ay), imu.calcAccel(imu.az));
        printf("Gyro: %f, %f, %f [deg/s] ", imu.calcGyro(imu.gx), imu.calcGyro(imu.gy), imu.calcGyro(imu.gz));
        printf("Mag: %f, %f, %f [gauss]", imu.calcMag(imu.mx), imu.calcMag(imu.my), imu.calcMag(imu.mz));
        printf("\n\n");*/
        usleep(1000000/czest_do_spania);
    }
    close(zapis);
    exit(EXIT_SUCCESS);
}

