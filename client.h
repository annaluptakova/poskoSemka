#ifndef CLIENT_H
#define CLIENT_H
#include "shared.h"

typedef struct{
    int pipe_to[2]; //Client->server
    int pipe_from[2]; //Server->client cize to and from server :P
    pid_t pid;
    double** priemery;
    double** pravdepodobnosti;
    Parametre pam;
    int napojeni;
} StavC;

void inicializaciaClient(StavC* stav);
void cleanUpClient(StavC* stav);
void clientSpusti(StavC* stav);
void clientStop(StavC* stav);
void clientPosliSpravu(StavC* stav, Msg* msg);
int clientPrijmiSpravu(StavC* stav, Msg* msg);
void getParametre(Parametre* pam, int* replikacie, char* outFile);
void vysledky(StavC* stav, int priemer);
void simulacia(StavC* stav);
void clientMenu(StavC* stav);

#endif