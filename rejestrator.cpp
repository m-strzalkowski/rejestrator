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
void wypisz(FILE * fp, OdczytIMU *d, int csv)
{
    if(!d) return;
    if(csv)
        fprintf(fp,"%f,%f,%f, %f,%f,%f, %f,%f,%f",d->ax, d->ay,d->az, d->zx, d->zy, d->zz, d->mx,d->my,d->mz);
    else
        fprintf(fp,"A< %+.3f, %+.3f, %+.3f [Gs]> G< %+.3f, %+.3f, %+.3f [deg/s]> M< %+.3f, %+.3f, %+.3f [gaus]>",d->ax, d->ay,d->az, d->zx, d->zy, d->zz, d->mx,d->my,d->mz);
}
void wypisz_odczyt(FILE *fp, Odczyt * odczyt, int csv)
{
    if(!csv)
    {
        struct tm * timeinfo = localtime(&(odczyt->czas.tv_sec));
        char * repr = asctime(timeinfo);
        repr[strlen(repr)-1]='\0';//nie chcemy \n
        fprintf(fp, "%s + %ldus, e:%d,i:%d,", repr, odczyt->czas.tv_usec, odczyt->etykieta, odczyt->impulsy);
    }
    else{
        struct tm * timeinfo = localtime(&(odczyt->czas.tv_sec));
        char * repr = asctime(timeinfo);
        repr[strlen(repr)-1]='\0';//nie chcemy \n
        fprintf(fp,"%s,%ld,%d,%d,",repr,odczyt->czas.tv_usec, odczyt->etykieta, odczyt->impulsy);
    }
    fprintf(fp,"%d,",odczyt->etykieta);
    wypisz(fp,&(odczyt->prim),csv);
    fprintf(fp,", ");
    wypisz(fp,&(odczyt->bis),csv);
    fprintf(fp,"\n");
}
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