#ifndef SERVER_H
#define SERVER_H
#include "shared.h"
#include "simulation.h" 
#include <pthread.h>

#define VLAKNA 4

typedef struct{
    double** priemery;
    double** pravdepodobnosti;
    Parametre pam;
    int replikacie;
    volatile int bezi;
    Mod mod;
    char out[256];
    pthread_mutex_t mutex;
    pthread_t vlakna[VLAKNA];
    int pocet;
} Stav;

void inicializacia(Stav* stav);
void posliKrok(int x, int y, int krok, void* data);
void kontrola(Stav* stav);
void posliSpravu(Msg* msg);
void prijmiSpravu(Stav* stav, Msg* msg);
void spustiSimulaciu(Stav* stav);
void* funkcia(void* arg);
void cleanUp(Stav* stav);

#endif