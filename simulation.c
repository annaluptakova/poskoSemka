#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "simulations.h"

int cesta(int xStart, int yStart, Parametre* pam){
    int x = xStart;
    int y = yStart;
    int kroky = 0; 
    int limit = pam->maxKroky * 20;
   // srand(time(NULL));

    while((x != 0 || y != 0) || kroky < limit){
        double nahodne = (double)rand() / RAND_MAX;
        if(nahodne < pam->hore){
            y++;
        } else if(nahodne < pam->hore + pam->dole){
            y--;
        } else if (nahodne < pam->hore + pam->dole + pam->vlavo){
            x--;
        } else {
            x++;
        }

        //aby sme nevyskocili z mapy hopefully
        x = (x + pam->sirka) % pam->sirka;
        y = (y + pam->vyska) % pam->vyska;

        kroky++;
    }

    if(x == 0 && y == 0){
        return kroky;
    } else {
        return -1;
    }
}

double priemernyPocet(int x, int y, int replikacie, Parametre* pam){
    int sum = 0;
    int uspech = 0;

    for(int i = 0; i < replikacie; i++){
        int krok = 0;
        krok = cesta(x, y, pam);
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

double pravdepodobnost(int x, int y, int k, int replikacie, Parametre* pam){
    double uspech = 0;
    for(int i = 0; i < replikacie; i++){
        int krok = 0;
        krok = cesta(x, y, pam);
        if(krok > 0 && krok < pam->maxKroky){
            uspech++;
        }
    }

    return uspech / replikacie;
}


double** alloc_pole(int sirka, int vyska){
    double** pole = malloc(vyska * sizeof(double*));
    for(int i = 0; i < vyska; i++){
        pole[i] = malloc(sirka * sizeof(double));
        for(int j = 0; j < sirka; j++){
            pole[i][j] = 0.0;
        }
    }
    return pole;
}

void free_pole(double** pole, int vyska){
    for(int i = 0; i < vyska; i++){
        free(pole[i]);
    }
    free(pole);
}
