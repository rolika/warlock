#include <stdio.h>
#include <stdlib.h>

#define SAVEFILE "player.dat"
#define TITLE "A  T Ű Z H E G Y   V A R Á Z S L Ó J A"
#define LINE "--------------------------------------------------------------------------------"
#define TAGGED_LINE "--------------------------+-----------------+----------------+------------------"


enum {
    NAME_LENGTH = 32
};

typedef struct item item;
typedef struct enemy enemy;
typedef struct player player;

void load(player*);
void title(char*);
void status(player*);

struct item {
    char name[NAME_LENGTH];
    int quantity;
    int initial_charge; // how many times the item can be used, -1 for unlimited usage
    int charge; // current charge, -1 for unlimited usage
    int mod_dp; // possible modifications to the player's attributes
    int mod_hp;
    int mod_lp;
    item *next; // pointer to the next item
};

struct enemy {
    char name[NAME_LENGTH];
    int dp;
    int hp;
    enemy *next;
};

struct player {
    char name[NAME_LENGTH];
    int initial_dp;
    int initial_hp;
    int initial_lp;
    int dp;
    int hp;
    int lp;
    item *inventory; // implemented as a linked list
    enemy *enemies; // deto
};


int main() {
    player player = {};
    load(&player);

    system("clear");
    title(TITLE);
    status(&player);

    return 0;
}

/* load player save file if present */
void load(player *player) {
    FILE *fp;
    fp = fopen(SAVEFILE, "r");
    if (fp != NULL) {
        // load file
        fclose(fp);
    } else {
        puts("Player file not found yet.");
    }
}

/* display title */
void title(char *title) {
    system("clear");
    puts(LINE);
    printf("%60s\n", title);
}

/* display player status */
void status(player* player) {
    puts(TAGGED_LINE);
    printf("Kalandor: %15s | Ügyesség: %2d/%2d | Életerő: %2d/%2d | Szerencse:  %2d/%2d\n",
    player->name, player->dp, player->initial_dp, player->hp, player->initial_hp, player->lp, player->initial_lp);
    puts(TAGGED_LINE);
}