#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "server.h"

void inicializacia(Stav* stav){
    stav->priemery = NULL;
    stav->pravdepodobnosti = NULL;
    stav->replikacie = 0;
    stav->bezi = 0;
}

void posliSpravu(Msg* msg){
    write(STDOUT_FILENO, msg, sizeof(Msg));
}

void prijmiSpravu(Stav* stav, Msg* msg){
    switch(msg->msgTyp){
        case START:
            stav->pam = msg->pam;
            stav->replikacie = msg->replikacie;
            spustiSimulaciu(stav);
        break;
        case STOP:
            stav->bezi = 0;
        break;
        default:
            printf("Neznamy typ spravy\n");
        break;
    }
}

void spustiSimulaciu(Stav* stav){
   stav->bezi = 1;
   stav->priemery = alloc_pole(stav->pam.sirka, stav->pam.vyska);
   stav->pravdepodobnosti = alloc_pole(stav->pam.sirka, stav->pam.vyska);

   for(int i = 0; i < stav->replikacie; i++){
    if(stav->bezi == 0){
        break;
        }
    
    Msg priebeh = {
        .msgTyp = PROCES,
        .aktualnaRepl = i + 1,
        .sumRepl = stav->replikacie
    };
    posliSpravu(&priebeh);

    for(int j = 0; j < stav->pam.vyska; j++){
        for(int l = 0; l < stav->pam.sirka; l++){
            if(j == 0 && l == 0){
                continue;
            }
            double prm = priemernyPocet(l, j, 1, &stav->pam);
            if(prm > 0){
                stav->priemery[j][l] += prm;
            }
            double pravd = pravdepodobnost(l, j, stav->pam.maxKroky, 1 , &stav->pam);
            stav->pravdepodobnosti[j][l] += pravd;
        }
    }
    }
    for(int i = 0; i < stav->pam.vyska; i++){
        for(int j = 0; j < stav->pam.sirka; j++){
            if(j == 0 && i ==0){
                continue;
            }
            stav->priemery[i][j] /= stav->replikacie;
            stav->pravdepodobnosti[i][j] /= stav->replikacie;
        }
    }

    for(int i = 0; i < stav->pam.vyska; i++){
        for(int j = 0; j < stav->pam.sirka; j++){
           Msg data = {
            .msgTyp = SUM,
            .xP = j,
            .yP = i,
            .priemer = stav->priemery[i][j],
            .pravdepodobnost = stav->pravdepodobnosti[i][j]
           };
           posliSpravu(&data);
        }
    }
    Msg koniec = {.msgTyp = KONIEC};
    posliSpravu(&koniec);
    stav->bezi = 0;
}

void cleanUp(Stav* stav){
    if(stav->priemery != NULL){
        free_pole(stav->priemery, stav->pam.vyska);
        stav->priemery = NULL;
    }
    if(stav->pravdepodobnosti != NULL){
        free_pole(stav->pravdepodobnosti, stav->pam.vyska);
        stav->pravdepodobnosti = NULL;
    }

}

int main(){
    Stav stav;
    inicializacia(&stav);
    Msg msg;
    while(read(STDIN_FILENO, &msg, sizeof(Msg)) > 0){
        prijmiSpravu(&stav, &msg);
    }

    cleanUp(&stav);
    return 0;
}