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
    stav->pam.mapa = NULL;
    stav->pam.maPrekazky = 0;
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
        Msg msg;
        memset(&msg, 0, sizeof(Msg));
        msg.msgTyp = KROK;
        msg.xC = nX;
        msg.yC = nY;
        msg.krok = krok;
        write(STDOUT_FILENO, &msg, sizeof(Msg));
        usleep(50000); // 50ms delay
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
            stav->pam.sirka = msg->pam.sirka;
            stav->pam.vyska = msg->pam.vyska;
            stav->pam.hore = msg->pam.hore;
            stav->pam.dole = msg->pam.dole;
            stav->pam.vlavo = msg->pam.vlavo;
            stav->pam.vpravo = msg->pam.vpravo;
            stav->pam.maxKroky = msg->pam.maxKroky;
            stav->pam.maPrekazky = msg->pam.maPrekazky;
            stav->pam.mapa = NULL;
            
            stav->replikacie = msg->replikacie;
            strcpy(stav->out, msg->out);
            
            if(stav->pam.maPrekazky && strlen(msg->mapaSubor) > 0){
                int sirka, vyska;
                stav->pam.mapa = nacitajMapu(msg->mapaSubor, &sirka, &vyska);
                if(stav->pam.mapa){
                    stav->pam.sirka = sirka;
                    stav->pam.vyska = vyska;
                } else {
                    stav->pam.maPrekazky = 0;
                }
            }
            
            spustiSimulaciu(stav);
            break;
            
        case STOP:
            stav->bezi = 0;
            break;
            
        default:
            break;
    }
}

void spustiSimulaciu(Stav* stav){
    stav->bezi = 1;
    
    stav->priemery = alloc_pole(stav->pam.sirka, stav->pam.vyska);
    stav->pravdepodobnosti = alloc_pole(stav->pam.sirka, stav->pam.vyska);
    
    if(!stav->priemery || !stav->pravdepodobnosti){
        cleanUp(stav);
        return;
    }

    ThreadData* data = malloc(stav->pocet * sizeof(ThreadData));
    if(!data){
        cleanUp(stav);
        return;
    }

    for(int i = 0; i < stav->replikacie && stav->bezi; i++){
        kontrola(stav);
        
        if(!stav->bezi){
            break;
        }

        if(stav->mod == INTERACTIVE){
            for(int l = 0; l < stav->pam.vyska && stav->bezi; l++){
                for(int j = 0; j < stav->pam.sirka && stav->bezi; j++){
                    if(l == 0 && j == 0){
                        continue;
                    }
                    if(stav->pam.maPrekazky && stav->pam.mapa && stav->pam.mapa[l][j] == 1){
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

                if(pthread_create(&stav->vlakna[t], NULL, funkcia, &data[t]) != 0){
                    stav->bezi = 0;
                    break;
                }
            }

            for(int t = 0; t < stav->pocet; t++){
                pthread_join(stav->vlakna[t], NULL);
            }
        }

        // Posli priebezne vysledky
        if(stav->mod == SUMMARY && stav->bezi){
            for(int j = 0; j < stav->pam.vyska; j++){
                for(int l = 0; l < stav->pam.sirka; l++){
                    Msg d;
                    memset(&d, 0, sizeof(Msg));
                    d.msgTyp = SUM;
                    d.xP = l;
                    d.yP = j;
                    d.priemer = (j == 0 && l == 0) ? 0 : stav->priemery[j][l] / (i + 1);
                    d.pravdepodobnost = (j == 0 && l == 0) ? 0 : stav->pravdepodobnosti[j][l] / (i + 1);
                    posliSpravu(&d);
                }
            }
        }
        
        Msg priebeh;
        memset(&priebeh, 0, sizeof(Msg));
        priebeh.msgTyp = PROCES;
        priebeh.aktualnaRepl = i + 1;
        priebeh.sumRepl = stav->replikacie;
        posliSpravu(&priebeh);
    }
    
    free(data);

    // Finalne vysledky
    for(int i = 0; i < stav->pam.vyska; i++){
        for(int j = 0; j < stav->pam.sirka; j++){
            if(i == 0 && j == 0){
                continue;
            } 
            if(stav->replikacie > 0){
                stav->priemery[i][j] /= stav->replikacie;
                stav->pravdepodobnosti[i][j] /= stav->replikacie;
            }
        }
    }

    for(int i = 0; i < stav->pam.vyska; i++){
        for(int j = 0; j < stav->pam.sirka; j++){
            Msg d;
            memset(&d, 0, sizeof(Msg));
            d.msgTyp = SUM;
            d.xP = j;
            d.yP = i;
            d.priemer = stav->priemery[i][j];
            d.pravdepodobnost = stav->pravdepodobnosti[i][j];
            posliSpravu(&d);
        }
    }
    
    ulozVysledky(stav->out, &stav->pam, stav->priemery, stav->pravdepodobnosti, stav->replikacie);
    
    Msg koniec;
    memset(&koniec, 0, sizeof(Msg));
    koniec.msgTyp = KONIEC;
    posliSpravu(&koniec);
    stav->bezi = 0;
}

void* funkcia(void* arg){
    ThreadData* data = (ThreadData*)arg;
    
    for(int i = data->startY; i < data->koniecY && *data->bezi; i++){
        for(int j = 0; j < data->pam->sirka && *data->bezi; j++){
            if(i == 0 && j == 0){
                continue;
            }
            if(data->pam->maPrekazky && data->pam->mapa && data->pam->mapa[i][j] == 1){
                continue;
            }
            
            int kroky = cesta(j, i, data->pam, data->callback, data->callbackData);
            
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
    while(read(STDIN_FILENO, &msg, sizeof(Msg)) == sizeof(Msg)){
        prijmiSpravu(&stav, &msg);
    }
    
    cleanUp(&stav);
    return 0;
}