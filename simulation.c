#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "simulation.h"

int cesta(int xStart, int yStart, Parametre* pam, KrokCallback callBack, void* data){
    int x = xStart;
    int y = yStart;
    int kroky = 0; 
    int limit = pam->maxKroky * 100;
   

    while((x != 0 || y != 0) && kroky < limit){
        double nahodne = (double)rand() / RAND_MAX;
        int novyX = x;
        int novyY = y;
        
        if(nahodne < pam->hore){
            novyY++;
        } else if(nahodne < pam->hore + pam->dole){
            novyY--;
        } else if(nahodne < pam->hore + pam->dole + pam->vlavo){
            novyX--;
        } else {
            novyX++;
        }

        novyX = (novyX + pam->sirka) % pam->sirka;
        novyY = (novyY + pam->vyska) % pam->vyska;
        if(pam->maPrekazky && pam->mapa != NULL && pam->mapa[novyY][novyX] == 1){
            continue;
        }

        x = novyX;
        y = novyY;
        kroky++;
        
        if(callBack){
            callBack(x, y, kroky, data);
        }
    }

    if(x == 0 && y == 0){
        return kroky;
    } else {
        return -1;
    }
}

double priemernyPocet(int x, int y, int replikacie, Parametre* pam, KrokCallback callBack, void* data){
    int sum = 0;
    int uspech = 0;

    for(int i = 0; i < replikacie; i++){
        int krok = 0;
        krok = cesta(x, y, pam, callBack, data);
        if(krok > 0){
            sum += krok;
            uspech++;
        }
    }
    if(uspech > 0){
        return (double)sum / (double)uspech;
    } else {
        return -1;
    }

}

double pravdepodobnost(int x, int y, int k, int replikacie, Parametre* pam, KrokCallback callBack, void* data){
    double uspech = 0;
    for(int i = 0; i < replikacie; i++){
        int krok = 0;
        krok = cesta(x, y, pam, callBack, data);
        if(krok > 0 && krok <= pam->maxKroky){
            uspech++;
        }
    }

    return uspech / replikacie;
}


double** alloc_pole(int sirka, int vyska){
    double** pole = malloc(vyska * sizeof(double*));
    if(!pole){
        return NULL;
    }
    for(int i = 0; i < vyska; i++){
        pole[i] = calloc(sirka, sizeof(double));
        if(!pole){
            for(int j = 0; j < sirka; j++){
                free(pole[j]);
            }
            free(pole);
            return NULL;
        }
    }   
    return pole;
}

void free_pole(double** pole, int vyska){
    if(!pole){
        return;
    }
    for(int i = 0; i < vyska; i++){
        free(pole[i]);
    }
    free(pole);
}

int** alloc_mapa(int sirka, int vyska){
    int** mapa = malloc(vyska * sizeof(int*));
    if(!mapa){
        return NULL;
    }
    for(int i = 0; i < vyska; i++){
        mapa[i] = calloc(sirka, sizeof(int));
        if(!mapa[i]){
            for(int j = 0; j < i; j++){
                free(mapa[j]);
            }
            free(mapa[i]);
            return NULL;
        }
    }
    return mapa;
}

void free_mapa(int** mapa, int vyska){
    if(!mapa){
        return;
    }
    for(int i = 0; i < vyska; i++){
        free(mapa[i]);
    }
    free(mapa);
}

int** prekazky(int sirka, int vyska, double pocetnost){
    int** mapa = alloc_mapa(sirka, vyska);
    if(!mapa){
        return NULL;
    }

    int maxPokusov = 100;
    int pokus = 0;
    do{
        for(int i = 0; i < vyska; i++){
            for(int j = 0; j < sirka; j++){
                if(i == 0 && j == 0){
                    mapa[i][j] = 0;
                } else {
                    mapa[i][j] = ((double)rand() / RAND_MAX < pocetnost) ? 1 : 0;
                }
            }
        }
         pokus++;
        if(pokus >= maxPokusov){
            for(int i = 0; i < vyska; i++){
                for(int j = 0; j < sirka; j++){
                    mapa[i][j] = 0;
                }
            }
            break;
        }
    }while(!vsetkoOk(mapa, sirka, vyska));
    return mapa;
}

int** nacitajMapu(const char* subor, int* sirka, int* vyska){
    FILE* f = fopen(subor, "r");
    if(!f){
        return NULL;
    }
    if(fscanf(f, "%d %d", sirka, vyska) != 2){
        fclose(f);
        return NULL;
    }

    int** mapa = alloc_mapa(*sirka, *vyska);
    if(!mapa){
        fclose(f);
        return NULL;
    }
    for(int i = *vyska -1; i >= 0; i--){
        for(int j = 0; j < *sirka; j++){
            if(fscanf(f, "%d", &mapa[i][j]) != 1){
                free_mapa(mapa, *vyska);
                fclose(f);
                return NULL;
            }
        }
    }
    fclose(f);
    if(!vsetkoOk(mapa, *sirka, *vyska)){
        free_mapa(mapa, *vyska);
        return NULL;
    }
    return mapa;
}

int vsetkoOk(int** mapa, int sirka, int vyska){
    int** navstivene = alloc_mapa(sirka, vyska);
    if(!navstivene){
        return 0;
    }
    int* radX = malloc(sirka * vyska * sizeof(int));
    int* radY = malloc(sirka * vyska * sizeof(int));
    if(!radX || !radY){
        free(radX);
        free(radY);
        free_mapa(navstivene, vyska);
        return 1;
    }

    int zaciatok = 0;
    int koniec = 0;
    radX[koniec] = 0;
    radY[koniec] = 0;
    koniec++;
    navstivene[0][0] = 1;

    int dx[] = {0,0,-1,1};
    int dy[] = {-1,1,0,0};

    while(zaciatok < koniec){
        int x = radX[zaciatok];
        int y = radY[zaciatok];
        zaciatok++;
        for(int i = 0; i < 4; i++){
            int nx = (x + dx[i] + sirka) % sirka;
            int ny = (y + dy[i] + vyska) % vyska;
            if(!navstivene[nx][ny] && mapa[nx][ny] == 0){
                navstivene[nx][ny] = 1;
                radX[koniec] = nx;
                radY[koniec] = ny;
                koniec++;
            }
        }
    }
    int ok = 1;
    for(int i = 0; i < vyska; i++){
        for(int j = 0; j < sirka; j++){
            if(mapa[i][j] == 0 && !navstivene[i][j]){
                ok = 0;
            }
        }
    }
    free_mapa(navstivene, vyska);
    free(radX);
    free(radY);
    return ok;
}

int ulozVysledky(const char* subor, Parametre* pam, double** priemery, double** pravdepodobnosti, int replikacie){
    FILE* f = fopen(subor, "w");
    if(!f){
        return 1;
    }
    fprintf(f, " sirka a vyska: %d %d\n", pam->sirka, pam->vyska);
    fprintf(f, "pravdepodobnost smery : %.6f %.6f %.6f %.6f\n", pam->hore, pam->dole, pam->vlavo, pam->vpravo);
    fprintf(f, "Max kroky a replikacie: %d %d\n", pam->maxKroky, replikacie);
    fprintf(f, "Ma prekazky? %d\n", pam->maPrekazky);

    if(pam->maPrekazky && pam->mapa){
        for(int i = pam->vyska - 1; i >= 0; i--){
            for(int j = 0; j <pam->sirka; j++){
                fprintf(f, "%d", pam->mapa[i][j]);
            }
            fprintf(f, "\n");
        }
    }
    printf("priemery:");
    for(int i = pam->vyska - 1; i >= 0; i--){
        for(int j = 0; j <pam->sirka; j++){
            fprintf(f, "%.2f", priemery[i][j]);
        }
        fprintf(f, "\n");
    }
    printf("pravdepodobnosti:");
    for(int i = pam->vyska - 1; i >= 0; i--){
        for(int j = 0; j <pam->sirka; j++){
            fprintf(f, "%.2f", pravdepodobnosti[i][j]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 0;
    
}


int nacitajVysledky(const char* subor, Parametre* pam, double** priemery, double** pravdepodobnosti, int replikacie){
FILE* f = fopen(subor, "r");
    if(!f){
        return 1;
    }
if(fscanf(f, "%d %d", &pam->sirka, &pam->vyska) != 2){
    fclose(f);
    return 1; 
}
if(fscanf(f, "%lf %lf %lf %lf", &pam->hore, &pam->dole, &pam->vlavo, &pam->vpravo) != 4){
    fclose(f);
    return 1; 
}
if(fscanf(f, "%d %d", &pam->maxKroky, replikacie) != 2){
    fclose(f);
    return 1; 
}
if(fscanf(f, "%d", &pam->maPrekazky) != 1){
    fclose(f);
    return 1; 
}

if(pam->maPrekazky){
        pam->mapa = alloc_mapa(pam->sirka, pam->vyska);
        if(!pam->mapa){
            fclose(f);
            return 1;
        }
        
        for(int i = pam->vyska - 1; i >= 0; i--){
            for(int j = 0; j < pam->sirka; j++){
                if(fscanf(f, "%d", &pam->mapa[i][j]) != 1){
                    free_mapa(pam->mapa, pam->vyska);
                    pam->mapa = NULL;
                    fclose(f);
                    return 1;
                }
            }
        }
    } else {
        pam->mapa = NULL;
    }
    
    *priemery = alloc_pole(pam->sirka, pam->vyska);
    *pravdepodobnosti = alloc_pole(pam->sirka, pam->vyska);
    
    if(!*priemery || !*pravdepodobnosti){
        if(*priemery){
            free_pole(*priemery, pam->vyska);
        }
        if(*pravdepodobnosti){
            free_pole(*pravdepodobnosti, pam->vyska);
        }
        if(pam->mapa){
            free_mapa(pam->mapa, pam->vyska);
        }
        fclose(f);
        return 1;
    }
    double** pr = *priemery;
    double** prav = *pravdepodobnosti;
    
    for(int i = pam->vyska - 1; i >= 0; i--){
        for(int j = 0; j < pam->sirka; j++){
            if(fscanf(f, "%lf", &pr[i][j]) != 1){
                free_pole(*priemery, pam->vyska);
                free_pole(*pravdepodobnosti, pam->vyska);
                *priemery = NULL;
                *pravdepodobnosti = NULL;
                if(pam->mapa){
                    free_mapa(pam->mapa, pam->vyska);
                    pam->mapa = NULL;
                }
                fclose(f);
                return 1;
            }
        }
    }
    
    for(int i = pam->vyska - 1; i >= 0; i--){
        for(int j = 0; j < pam->sirka; j++){
            if(fscanf(f, "%lf", &prav[i][j]) != 1){
                free_pole(*priemery, pam->vyska);
                free_pole(*pravdepodobnosti, pam->vyska);
                *priemery = NULL;
                *pravdepodobnosti = NULL;
                if(pam->mapa){
                    free_mapa(pam->mapa, pam->vyska);
                    pam->mapa = NULL;
                fclose(f);
                return 1;
            }
        }
    }
    
    fclose(f);
    return 0;
}
