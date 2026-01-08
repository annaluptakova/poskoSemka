#ifndef SHARED_H
#define SHARED_H
#include "simulation.h"

typedef enum{
    //client to server
    START,
    NACITAJ,
    ZMEN_MOD,
    STOP,
    //server to client
    PROCES,
    KROK,
    SUM,
    KONIEC
}MsgTyp;

typedef enum{
    INTERACTIVE,
    SUMMARY
}Mod;

typedef enum{
    PRIEMER_KROKY,
    PRAVDEPODOBNOST
}Stat;

typedef struct{
    MsgTyp msgTyp;
    Parametre pam;
    int replikacie;
    char out[256];
    char in[256];
    Mod mode;
    Stat stat;
    int aktualnaRepl;
    int sumRepl;
    int xC; //->toto suradnice chodca preto C
    int yC;
    int krok;
    int xP; //->toto suradnice policka 
    int yP;
    double priemer;
    double pravdepodobnost;
} Msg;

#endif