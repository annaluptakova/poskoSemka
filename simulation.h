#ifndef SIMULATION_H
#define SIMULATION_H

typedef struct{
    int sirka;
    int vyska;
    double hore;
    double dole;
    double vlavo;
    double vpravo;
    int maxKroky; // -> k

} Parametre;

int cesta(int xStart, int yStart, Parametre* pam);
double priemernyPocet(int x, int y, int replikacie, Parametre* pam);
double pravdepodobnost(int x, int y, int k, int replikacie, Parametre* pam);
double** alloc_pole(int sirka, int vyska);
void free_pole(double** pole, int vyska);

#endif