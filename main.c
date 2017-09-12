#include <stdio.h>

#define SAVEFILE "player.dat"

enum {
    NAME_LENGTH = 32
};

typedef struct item item;
typedef struct enemy enemy;
typedef struct player player;

void load(player*);

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
    return 0;
}

/* load player save file is present */
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