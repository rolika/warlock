#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#define SAVEFILE "player.dat"
#define TITLE "A  T Ű Z H E G Y   V A R Á Z S L Ó J A"
#define LINE "--------------------------------------------------------------------------------"
#define TAGGED_LINE "--------------------------+-----------------+----------------+------------------"
#define CREATE_NEW_PLAYER "Új játékos indítása"
#define FIGHT "Harc"
#define INVENTORY "Felszerelés"
#define ENEMIES "Ellenségek"
#define ROLL_DICE "Dobókocka"


enum {
    NAME_LENGTH = 32
};

typedef struct item item; /* describe an item in the inventory */
typedef struct enemy enemy; /* describe a defeated enemy */
typedef struct player player; /* describe the player character */

void load(player*); /* load player save file if present */
void title(char*); /* display title */
void status(player*); /* display player status */
int menu_of(int, ...); /* display a menu listed in the arguments */
void create(player*); /* create a new player according to the game rules */
int roll_dice(int); /* roll an n-sided dice */
void save(player*); /* save player's attributes to file as csv's */
item *new(char*, int, int, int, int, int, int); /* create new item */
item *take(item*, item*); /* add item to inventory (to a linked list) */
item* setup(item*); /* setup default inventory according to game rules */
void apply(item*, void (*fn) (item*, char*), char*); /* apply function to all items in inventory */
void print(item*, char*); /* print item of the inventory using given format */
item *lookup(item*, char*); /* look for item after it's name */

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
    srand(time(NULL));
    player player = {};
    load(&player);

    /* main menu */
    while (1) {
        system("clear");
        title(TITLE);
        status(&player);
        switch (menu_of(5, CREATE_NEW_PLAYER, FIGHT, INVENTORY, ENEMIES, ROLL_DICE)) {
            case 1:
                create(&player);
                player.inventory = setup(player.inventory);
                save(&player);
                break;
            case 2:
                puts(FIGHT);
                break;
            case 3:
                apply(player.inventory, print, "%s: %2d db\n");
                break;
            case 4:
                puts(ENEMIES);
                break;
            case 5:
                puts(ROLL_DICE);
                break;
            case 6:
                puts("Good bye!");
                exit(0);
        }
    }

    return 0;
}

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

void title(char *title) {
    puts(LINE);
    printf("%60s\n", title);
}

void status(player* player) {
    puts(TAGGED_LINE);
    printf("Kalandor: %15s | Ügyesség: %2d/%2d | Életerő: %2d/%2d | Szerencse:  %2d/%2d\n",
    player->name, player->dp, player->initial_dp, player->hp, player->initial_hp, player->lp, player->initial_lp);
    puts(TAGGED_LINE);
}

int menu_of(int argc, ...) {
    int i, choice;
    va_list menup;
    va_start(menup, argc);
    puts("Választási lehetőségeid:");
    puts(LINE);
    for (i = 0; i < argc; ++i) {
        printf("[%d] %s\n", i + 1, va_arg(menup, char*));
    }
    printf("[%d] Kilépés\n", i + 1);
    puts(LINE);
    printf("Válassz egyet és nyomj Enter-t! ");
    while (1) {
        choice = getchar() - '1' + 1;
        if (0 < choice && choice <= argc + 1) {
            return choice;
        }
    }
}

void create(player *player) {
    system("clear");
    status(player);

    printf("Mi a neved, kalandor? ");
    scanf("%15s", player->name);

    player->initial_dp = roll_dice(6) + 6;
    player->dp = player->initial_dp;

    player->initial_hp = roll_dice(6) + roll_dice(6) + 12;
    player->hp = player->initial_hp;

    player->initial_lp =roll_dice(6) + 6;
    player->lp = player->initial_lp;

}

int roll_dice(int n) {
    return rand() % n + 1;
}

void save(player *player) {
    FILE *fp;

    if ((fp = fopen(SAVEFILE, "w")) != NULL) {
        /* saving basic attributes in the first line */
        fprintf(fp, "%s;%d;%d;%d;%d;%d;%d\n",
            player->name, player->dp, player->hp, player->lp,
            player->initial_dp, player->initial_hp, player->initial_lp);
        fclose(fp);
    } else {
        puts("Some really nasty error occured.");
        puts("Unable to save to file.");
        exit(1);
    }

    
}

item *new(char *name, int quantity, int initial_charge, int charge, int mod_dp, int mod_hp, int mod_lp) {
    item *newitem;
    newitem = malloc(sizeof(item));
    if (newitem != NULL) {
        strcpy(newitem->name, name);
        newitem->quantity = quantity;
        newitem->initial_charge = initial_charge;
        newitem->charge = charge;
        newitem->mod_dp = mod_dp;
        newitem->mod_hp = mod_hp;
        newitem->mod_lp = mod_lp;
        return newitem;
    } else {
        puts("Some really nasty error occured.");
        puts("Unable allocate enough memory.");
        exit(1);
    }
}

item *take(item *head, item *newitem) {
    item *exist = lookup(head, newitem->name);
    if (exist == NULL) {
        newitem->next = head; // add item to the front of list (as new first item)
        return newitem;
    } else {
        exist->quantity += newitem->quantity; // increase quantity
        return head; // head is untouched
    }
}

item *setup(item* head) {
    head = new("kard", 1, -1, -1, 0, 0, 0); /* add sword */
    head = take(head, new("bőrpáncél", 1, -1, -1, 0, 0, 0)); /* add leather armour */
    head = take(head, new("kard", 1, -1, -1, 0, 0, 0));
    head = take(head, new("élelem", 10, 1, 1, 0, 4, 0)); /* add ten units of food */
    return head;
}

void apply(item* head, void (*fn) (item*, char*), char* arg) {
    item *p; // preserve head
    for (p = head; p != NULL; p = p->next) {
        (*fn)(p, arg);
    }
}

void print(item *head, char *fmt) {
    printf(fmt, head->name, head->quantity);
}

item *lookup(item *head, char *name) {
    if (head != NULL && strcmp(head->name, name)) {
        return lookup(head->next, name);
    }
    return head;
}