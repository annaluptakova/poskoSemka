#ifndef SIMULATION_H
#define SIMULATION_H
#include <pthread.h>

typedef struct{
    int sirka;
    int vyska;
    double hore;
    double dole;
    double vlavo;
    double vpravo;
    int maxKroky;
    int** mapa;
    int maPrekazky;
} Parametre;

typedef void (*KrokCallback)(int x, int y, int krok, void* data);

typedef struct {
    int id;
    int startY;
    int koniecY;
    Parametre* pam;
    double** priemery;
    double** pravdepodobnosti;
    int replikacie;
    KrokCallback callback;
    pthread_mutex_t* mutex;
    void* callbackData;
    volatile int* bezi;
} ThreadData;

int cesta(int xStart, int yStart, Parametre* pam, KrokCallback callBack, void* data);
double priemernyPocet(int x, int y, int replikacie, Parametre* pam, KrokCallback callBack, void* data);
double pravdepodobnost(int x, int y, int k, int replikacie, Parametre* pam, KrokCallback callBack, void* data);
double** alloc_pole(int sirka, int vyska);
void free_pole(double** pole, int vyska);
int** alloc_mapa(int sirka, int vyska);
void free_mapa(int** mapa, int vyska);
int** prekazky(int sirka, int vyska, double pocetnost);
int** nacitajMapu(const char* subor, int* sirka, int* vyska);
int vsetkoOk(int** mapa, int sirka, int vyska);
int ulozVysledky(const char* subor, Parametre* pam, double** priemery, double** pravdepodobnosti, int replikacie);
int nacitajVysledky(const char* subor, Parametre* pam, double*** priemery, double*** pravdepodobnosti, int* replikacie);

#endif