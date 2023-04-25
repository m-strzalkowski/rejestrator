/*
 *** REJESTRATOR.cpp ***
 Podstawowa funkcjonalność odczytu z IMU poprzez kod z LSM9DS1
 Kod inicjalizujacy i kalibrujacy znajduje sie bezposrednio w nagrywaj.cpp, jako ze nie powinien byc uzywany gdziekolwiek indziej.

 Odczyt reprezentuje binarnie pomiar, do ktorego wkalda sie dane f. wypelnij_odczyt, a wyciaga ich tekstowa reprezentacje za pomoca wypisz
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "LSM9DS1_Types.h"
#include "LSM9DS1.h"
#include "rejestrator.h"
void wypelnij(OdczytIMU * d, LSM9DS1 * imu)
{
    d->zx = imu->calcGyro(imu->gx);
    d->zy = imu->calcGyro(imu->gy);
    d->zz = imu->calcGyro(imu->gz);

    d->ax = imu->calcAccel(imu->ax);
    d->ay = imu->calcAccel(imu->ay);
    d->az = imu->calcAccel(imu->az);

    d->mx = imu->calcMag(imu->mx);
    d->my = imu->calcMag(imu->my);
    d->mz = imu->calcMag(imu->mz);
}

void wypelnij_odczyt(Odczyt * odczyt, LSM9DS1 * imu1, LSM9DS1 * imu2, int etykieta, int impulsy)
{
    static struct timeval curr;
    gettimeofday(&curr, NULL);
    
    //odczyt->t_msec = curr.tv_usec;
    memcpy(&(odczyt->czas), &curr, sizeof(curr));
    //odczyt->time
    odczyt->etykieta = etykieta;
    odczyt->impulsy = impulsy;
    if(imu1)
        wypelnij(&(odczyt->prim), imu1);
    else
        memset(&(odczyt->prim), 0, sizeof(OdczytIMU));
    
    if(imu2)
        wypelnij(&(odczyt->bis), imu2);
    else
        memset(&(odczyt->bis), 0, sizeof(OdczytIMU));
    
    return;
}
//zapewnia oficjalny naglowek csv
void wypisz_naglowek(FILE * wy)
{
    //fprintf(wy,"t_sec,label, pax,pay,paz,pzx,pzy,pzz,pmx,pmy,pmz, bax,bay,baz,bzx,bzy,bzz,bmx,bmy,bmz\n");
    fprintf(wy,"datetime,microsec,label,impulses, pax,pay,paz,pzx,pzy,pzz,pmx,pmy,pmz, bax,bay,baz,bzx,bzy,bzz,bmx,bmy,bmz\n");
}
void wypisz(FILE * fp, OdczytIMU *d, int csv)
{
    if(!d) return;
    if(csv)
        fprintf(fp,"%f,%f,%f, %f,%f,%f, %f,%f,%f",d->ax, d->ay,d->az, d->zx, d->zy, d->zz, d->mx,d->my,d->mz);
    else
        fprintf(fp,"A< %+.3f, %+.3f, %+.3f [Gs]> G< %+.3f, %+.3f, %+.3f [deg/s]> M< %+.3f, %+.3f, %+.3f [gaus]>",d->ax, d->ay,d->az, d->zx, d->zy, d->zz, d->mx,d->my,d->mz);
}
//wypisuje caly pomiar, w formacie zgodnym z wypisz_naglowek
void wypisz_odczyt(FILE *fp, Odczyt * odczyt, int csv)
{
    if(!csv)
    {
        struct tm * timeinfo = localtime(&(odczyt->czas.tv_sec));
        char * repr = asctime(timeinfo);
        repr[strlen(repr)-1]='\0';//nie chcemy \n
        fprintf(fp, "%s + %06ldus, e:%d,i:%d,", repr, odczyt->czas.tv_usec, odczyt->etykieta, odczyt->impulsy);
    }
    else{
        struct tm * timeinfo = localtime(&(odczyt->czas.tv_sec));
        char * repr = asctime(timeinfo);
        repr[strlen(repr)-1]='\0';//nie chcemy \n
        fprintf(fp,"%s,%06ld,%d,%d,",repr,odczyt->czas.tv_usec, odczyt->etykieta, odczyt->impulsy);
    }
    wypisz(fp,&(odczyt->prim),csv);
    fprintf(fp,", ");
    wypisz(fp,&(odczyt->bis),csv);
    fprintf(fp,"\n");
}

/*
void inicjalizuj_i_kalibruj_konfiguracje_imu(LSM9DS1 *** imu_tab_wsk, int * imu_num_wsk)
{
    static LSM9DS1 imu1(IMU_MODE_I2C, 0x6b, 0x1e);
    static LSM9DS1 imu2(IMU_MODE_I2C, 0x6a, 0x1c);
    static LSM9DS1 * imu_tab[] = {&imu1, &imu2};
    *imu_num_wsk=2;
    *imu_tab_wsk = imu_tab;
}
void kalibruj_imu(LSM9DS1 * imu_tab[], const int imu_num)
{
    LSM9DS1 *imu;
    for(int i=0; i<imu_num && (imu = imu_tab[i]) ; i++)//dla obu imu
    {
        fprintf(stderr,"\nbegin calibration");
        //imu->begin();
        if (!imu->begin()) {
            fprintf(stderr, "Failed to communicate with LSM9DS1 %d.\n", i);
            exit(EXIT_FAILURE);
        }
        fprintf(stderr,"\nend calibration");

        imu->calibrate();
    }
}
void wykonaj_pomiary(LSM9DS1 * imu_tab[], const int imu_num)
{
    LSM9DS1 *imu;
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
*/

/*
int czestotliwosc = 20;//Hz
int main(int argc, char *argv[]) {
    printf("sizeof(OdczytIMU):%d sizeof(Odczyt):%d\n", sizeof(OdczytIMU), sizeof(Odczyt));
    
    Odczyt odczyt;
    LSM9DS1 imu(IMU_MODE_I2C, 0x6b, 0x1e);
    imu.begin();
    if (!imu.begin()) {
        fprintf(stderr, "Failed to communicate with LSM9DS1.\n");
        exit(EXIT_FAILURE);
    }
    imu.calibrate();

    for (;;) {
        while (!imu.gyroAvailable()) ;
        imu.readGyro();
        while(!imu.accelAvailable()) ;
        imu.readAccel();
        while(!imu.magAvailable()) ;
        imu.readMag();

        wypelnij_odczyt(&odczyt, &imu, NULL);

        wypisz_odczyt(stdout,&odczyt,0);
        
        
        printf("\nAccel: %f, %f, %f [Gs] ", imu.calcAccel(imu.ax), imu.calcAccel(imu.ay), imu.calcAccel(imu.az));
        printf("Gyro: %f, %f, %f [deg/s] ", imu.calcGyro(imu.gx), imu.calcGyro(imu.gy), imu.calcGyro(imu.gz));
        printf("Mag: %f, %f, %f [gauss]", imu.calcMag(imu.mx), imu.calcMag(imu.my), imu.calcMag(imu.mz));
        printf("\n\n");
        sleep(1.0/czestotliwosc);
    }

    exit(EXIT_SUCCESS);
}
*/