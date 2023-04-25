#include "LSM9DS1_Types.h"
#include "LSM9DS1.h"
struct OdczytIMU {
    
    float ax;//akcelerometr
    float ay;
    float az;

    float zx;//zyroskop
    float zy;
    float zz;

    float mx;//magnetometr
    float my;
    float mz;
};
struct Odczyt {
    struct OdczytIMU prim;
    struct OdczytIMU bis;
    struct timeval czas;
    //suseconds_t t_msec;
    int etykieta;
    int impulsy;
};
//void wypelnij(OdczytIMU * d, LSM9DS1 * imu);
void wypelnij_odczyt(Odczyt * odczyt, LSM9DS1 * imu1, LSM9DS1 * imu2, int etykieta, int impulsy);
//void wypisz(FILE * fp, OdczytIMU *d, int csv);
void wypisz_odczyt(FILE *fp, Odczyt * odczyt, int csv);
void wypisz_naglowek(FILE * wy);