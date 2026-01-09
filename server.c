#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include "server.h"

void inicializacia(Stav* stav){
    stav->priemery = NULL;
    stav->pravdepodobnosti = NULL;
    stav->replikacie = 0;
    stav->bezi = 0;
    stav->mod = SUMMARY;
    stav->pocet = VLAKNA;
    pthread_mutex_init(&stav->mutex, NULL);
}

void posliSpravu(Msg* msg){
    write(STDOUT_FILENO, msg, sizeof(Msg));
}

void posliKrok(int x, int y, int krok, void* data){
    Stav* stav = (Stav*)data;
    if(stav->mod == INTERACTIVE){
        int nX = x - (stav->pam.sirka / 2);
        int nY = y - (stav->pam.vyska / 2);
        Msg msg = {
            .msgTyp = KROK,
            .xC = nX,
            .yC = nY,
            .krok = krok
        };
        write(STDOUT_FILENO, &msg, sizeof(Msg));
        sleep(30);
    }
}

void kontrola(Stav* stav){
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    Msg msg;
    ssize_t a = read(STDIN_FILENO, &msg, sizeof(Msg));

    if(a == sizeof(Msg)){
        if(msg.msgTyp == ZMEN_MOD){
            stav->mod = msg.mode;
        } else if(msg.msgTyp == STOP){
            stav->bezi = 0;
        }
    }
    fcntl(STDIN_FILENO, F_SETFL, flags);
}

void prijmiSpravu(Stav* stav, Msg* msg){
    switch(msg->msgTyp){
        case START:
            stav->pam = msg->pam;
            stav->replikacie = msg->replikacie;
            strcpy(stav->out, msg->out);
            spustiSimulaciu(stav);
        break;
        case STOP:
            stav->bezi = 0;
        break;
        default:
        //?????????????????
        break;
    }
}

void spustiSimulaciu(Stav* stav){
   stav->bezi = 1;
   stav->priemery = alloc_pole(stav->pam.sirka, stav->pam.vyska);
   stav->pravdepodobnosti = alloc_pole(stav->pam.sirka, stav->pam.vyska);

    ThreadData* data = malloc(stav->pocet * sizeof(ThreadData));
    if(!data){
        cleanUp(stav);
        return;
    }

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

    if(stav->mod == INTERACTIVE){
        for(int l = 0; l < stav->pam.vyska && stav->bezi; l++){
            for(int j = 0; j < stav->pam.sirka && stav->bezi; j++){
                if(l == 0 && j == 0){
                    continue;
                }
                int kroky = cesta(j, l, &stav->pam, posliKrok, stav);

                if(kroky > 0){
                stav->priemery[l][j] += kroky;
                }
                if(kroky > 0 && kroky <= stav->pam.maxKroky){
                    stav->pravdepodobnosti[l][j] += 1.0;
                }
                kontrola(stav);
            }
        }
    } else {
        int nPocet = stav->pam.vyska / stav->pocet;
        int zvysok = stav->pam.vyska % stav->pocet;

        for(int t = 0; t < stav->pocet; t++){
            data[t].id = t;
            data[t].startY = t * nPocet + (t < zvysok ? t : zvysok);
            data[t].koniecY = data[t].startY + nPocet + (t < zvysok ? 1 : 0);
            data[t].pam = &stav->pam;
            data[t].priemery = stav->priemery;
            data[t].pravdepodobnosti = stav->pravdepodobnosti;
            data[t].replikacie = stav->replikacie;
            data[t].mutex = &stav->mutex;
            data[t].callback = NULL;
            data[t].callbackData = stav;
            data[t].bezi = &stav->bezi;

            if(pthread_create(&stav->vlakna[t], NULL, funkcia, &data[t]) !=0){
                printf("vlakna err");
                stav->bezi = 0;
                break;
            }
        }

        for(int t = 0; t < stav->pocet; t++){
            pthread_join(stav->vlakna[t], NULL);
        }

    }
    kontrola(stav);

    if(stav->mod == SUMMARY){
        for(int j = 0; j < stav->pam.vyska; j++){
            for(int l = 0; l < stav->pam.sirka; l++){
                Msg d = {
                    .msgTyp = SUM,
                    .xP = l,
                    .yP = j,
                    .priemer = (j == 0 && l == 0) ? 0 : stav->priemery[j][l] / (i + 1),
                    .pravdepodobnost = (j == 0 && l == 0) ? 0 : stav->pravdepodobnosti[j][l] / (i + 1)
                };
                posliSpravu(&d);
            }
        }
    }
    }
    free(data);

     for(int i = 0; i < stav->pam.vyska; i++){
            for(int j = 0; j < stav->pam.sirka; j++){
                if(i == 0 && j == 0){
                    continue;
                } 
                stav->priemery[i][j] /= stav->replikacie;
                stav->pravdepodobnosti[i][j] /= stav->replikacie;

            }
        }

    for(int i = 0; i < stav->pam.vyska; i++){
            for(int j = 0; j < stav->pam.sirka; j++){
                Msg d = {
                    .msgTyp = SUM,
                    .xP = j,
                    .yP = i,
                    .priemer = stav->priemery[i][j],
                    .pravdepodobnost = stav->pravdepodobnosti[i][j]
                };
                posliSpravu(&d);
            }
        }
        ulozVysledky(stav->out, &stav->pam, stav->priemery, stav->pravdepodobnosti, stav->replikacie);
        Msg koniec = {.msgTyp = KONIEC};
        posliSpravu(&koniec);
        stav->bezi = 0;
    
}

void* funkcie(void* arg){
    ThreadData* data = (ThreadData*)arg;
    for(int i = data->startY; i < data->koniecY && *data->bezi; i++){
        for(int j = 0; j < data->pam->sirka && *data->bezi; j++){
            if(i == 0 && j ==0){
                continue;
            }
            int kroky = cesta(j,i, data->pam, data->callback, data->callbackData);
            pthread_mutex_lock(data->mutex);
            if(kroky > 0){
                data->priemery[i][j] += kroky;
            }
            if(kroky > 0 && kroky <= data->pam->maxKroky){
                data->pravdepodobnosti[i][j] += 1.0;
            }

            pthread_mutex_unlock(data->mutex);
        }
    }
    return NULL;
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
    if(stav->pam.mapa != NULL){
        free_mapa(stav->pam.mapa, stav->pam.vyska);
        stav->pam.mapa = NULL;
    }
    pthread_mutex_destroy(&stav->mutex);  
}

int main(){
    srand(time(NULL));
    Stav stav;
    inicializacia(&stav);
    Msg msg;
    while(read(STDIN_FILENO, &msg, sizeof(Msg)) > 0){
        prijmiSpravu(&stav, &msg);
    }

    cleanUp(&stav);
    return 0;
}