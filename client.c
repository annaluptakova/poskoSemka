#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "client.h"

void inicializaciaClient(StavC* stav){
    stav->pid = -1;
    stav->priemery = NULL;
    stav->pravdepodobnosti = NULL;
    stav->napojeni = 0;
}

void cleanUpClient(StavC* stav){
    if(stav->priemery != NULL){
        free_pole(stav->priemery, stav->pam.vyska);
        stav->priemery = NULL;
    }
    if(stav->pravdepodobnosti != NULL){
        free_pole(stav->pravdepodobnosti, stav->pam.vyska);
        stav->pravdepodobnosti = NULL;
    }
    if(stav->napojeni == 1){
        clientStop(stav);
    }
}

void clientSpusti(StavC* stav){
    if(pipe(stav->pipe_to) == -1){
        printf("pipe err");
        exit(1);
    }
    if(pipe(stav->pipe_from) == -1){
        printf("pipe err");
        exit(1);
    }

    pid_t pid1 = fork();
    if(pid1 < 0){
        printf("fork err");
        exit(1);
    }
    if(pid1 == 0){
        close(stav->pipe_to[1]);
        close(stav->pipe_from[0]);

        dup2(stav->pipe_to[0], STDIN_FILENO);
        dup2(stav->pipe_from[1], STDOUT_FILENO);

        close(stav->pipe_to[0]);
        close(stav->pipe_from[1]);
        execl("./server", "server", NULL);
        perror("execl err");
        exit(1);

    }
    close(stav->pipe_to[0]);
    close(stav->pipe_from[1]);
    stav->pid = pid1;
    stav->napojeni = 1;
}

void clientStop(StavC* stav){
    if(stav->napojeni == 0){
        return;
    }
    close(stav->pipe_to[1]);
    close(stav->pipe_from[0]);
    waitpid(stav->pid, NULL, 0);
    stav->napojeni = 0;
    stav->pid = -1;
}

void clientPosliSpravu(StavC* stav, Msg* msg){
    write(stav->pipe_to[1], msg, sizeof(Msg));
}

int clientPrijmiSpravu(StavC* stav, Msg* msg){
    if(read(stav->pipe_from[0], msg, sizeof(Msg)) > 0){
        return 1;
    } else {
        return 0;
    }
    
}

void getParametre(Parametre* pam, int* replikacie, char* outFile){
    printf("Podla nasledovneho zadajte parametre simulacie:\n");

    printf("Šírka sveta: ");
    scanf("%d", &pam->sirka);
    printf("Výška sveta: ");
    scanf("%d", &pam->vyska);

    printf("Pravdepodobnosť hore: ");
    scanf("%lf", &pam->hore);
    printf("Pravdepodobnosť dole: ");
    scanf("%lf", &pam->dole);
    printf("Pravdepodobnosť vlavo: ");
    scanf("%lf", &pam->vlavo);
    printf("Pravdepodobnosť vpravo: ");
    scanf("%lf", &pam->vpravo);
    
    double sumPrav = pam->vpravo + pam->dole + pam->hore + pam->vlavo;
    if(sumPrav < 0.99 || sumPrav > 1.01){
        printf("Pozor sucet pravdepodobnosti by mal tvorit dokopy 1");
        printf("Pravdepodobnosť hore: ");
        scanf("%lf", &pam->hore);
        printf("Pravdepodobnosť dole: ");
        scanf("%lf", &pam->dole);
        printf("Pravdepodobnosť vlavo: ");
        scanf("%lf", &pam->vlavo);
        printf("Pravdepodobnosť vpravo: ");
        scanf("%lf", &pam->vpravo);
    }

    printf("Max počet krokov: ");
    scanf("%d", &pam->maxKroky);

    printf("Počet replikacii: ");
    scanf("%d", replikacie);

    printf("Výstupný súbor: ");
    scanf("%s", outFile);
}

void vysledky(StavC* stav, int priemer){
    printf("VYSLEDKY:\n");
    printf("Zobrazujem: %s\n\n", priemer ? "Priemerný počet krokov" : "Pravdepodobnosť");
    for (int i = stav->pam.vyska - 1; i >= 0; i--) {
        for (int j = 0; j < stav->pam.sirka; j++) {
            if (j == 0 && i == 0) {
                printf("  [0,0]  ");
            } else {
                double value = priemer ? stav->priemery[i][j] : stav->pravdepodobnosti[i][j];
                printf("%8.2f ", value);
            }
        }
        printf("\n");
    }
    printf("\n");
}

void simulacia(StavC* stav){
    Parametre pam;
    int repl;
    char outFile[256];

    getParametre(&pam, &repl, outFile);
    stav->pam = pam;
    clientSpusti(stav);
    Msg start = {
        .msgTyp = START,
        .pam = pam,
        .replikacie = repl
    };
    strcpy(start.out, outFile);
    clientPosliSpravu(stav, &start);
    stav->priemery = alloc_pole(pam.sirka, pam.vyska);
    stav->pravdepodobnosti = alloc_pole(pam.sirka, pam.vyska);
    Msg msg;
    int count = 0;

    while(clientPrijmiSpravu(stav, &msg)){
        switch(msg.msgTyp){
            case PROCES:    
                printf("Proces?");
                fflush(stdout);
            break;
                
            case SUM:
                stav->priemery[msg.yP][msg.xP] = msg.priemer;
                stav->pravdepodobnosti[msg.yP][msg.xP] = msg.pravdepodobnost;
                count++;
            break;
                
            case KONIEC:
                vysledky(stav, 1);  
                clientStop(stav);
            return;
                
            default:
                printf("Neznámy typ správy\n");
            break;
        }
    }

}

void clientMenu(StavC* stav){
    while(1){
        printf("RANDOM WALK SIMULACIA - POSKO SEMKA");
        printf("1. Nova simulacie\n");
        printf("2. Nacitaj zo suboru\n");
        printf("3. Koniec\n");
        printf("Napis volbu: \n");
        int a; 
        scanf("%d", &a);
        switch(a){
            case 1:
                simulacia(stav);
            break;
            case 2:
                //nacitanie zo suboru lovely
            break;
            case 3:
                exit(1);
            break;
            default:
                printf("Neplatna volba\n");
            break;
            }
    }
}

int main(){
    srand(time(NULL));
    StavC stav;
    inicializaciaClient(&stav);
    clientMenu(&stav);
    cleanUpClient(&stav);
    return 0;
}
