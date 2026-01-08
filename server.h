#ifndef SERVER_H
#define SERVER_H
#include "shared.h"
#include "simulation.h" 

typedef struct{
    double** priemery;
    double** pravdepodobnosti;
    Parametre pam;
    int replikacie;
    int bezi;
} Stav;

void inicializacia(Stav* stav);
void posliSpravu(Msg* msg);
void prijmiSpravu(Stav* stav, Msg* msg);
void spustiSimulaciu(Stav* stav);
void cleanUp(Stav* stav); //->doriesit lepsie pomenovanie 

#endif
