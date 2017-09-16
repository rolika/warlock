#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#define SAVEFILE "player.dat"
#define TITLE "A  T Ű Z H E G Y   V A R Á Z S L Ó J A"
#define LINE "--------------------------------------------------------------------------------"
#define TAGGED_LINE "--------------------------+-----------------+----------------+------------------"

enum {
    NAME_LENGTH = 32,
    MAX_LINE = 1000,
    MAX_CSV_FIELD = 100
};

#define roll_dice(n) (rand() % (n) + 1)

typedef struct item item; /* describe an item in the inventory */
typedef struct enemy enemy; /* describe a defeated enemy */
typedef struct player player; /* describe the player character */

void load(player*); /* load player save file if present */
void title(char*); /* display title */
void status(player*); /* display player status */
int menu_of(int, ...); /* display a menu listed in the arguments */
void create(player*); /* create a new player according to the game rules */
void save(player*); /* save player's attributes to file as csv's */
item *new(char*, int, int, int, int, int, int); /* create new item */
item *take(item*, item*); /* add item to inventory (to a linked list) */
item *setup(item*); /* setup default inventory according to game rules */
item *lookup(item*, char*); /* look for item after it's name */
void items2csv(item*, FILE*); /* convert item data into csv */
int getcsv(FILE*); /* get a csv line from file */
void free_inventory(item*); /* delete all items in the inventory (before load) */
item *potion(item*); /* let the player to choose a potion */
item *inventory_menu(player*); /* handle the inventory: take, drop, use items */
item *new2inventory(item*); /* create and add a new item to the inventory */

/* TODO:
    inventory menu
     - consume potion or food (= use any item with a non-negative charge value)
     - drop item (considering first decreasing quantity)
     - take new item
*/

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

char csvbuffer[MAX_LINE];
char *csvfield[MAX_CSV_FIELD];

int main() {
    srand(time(NULL));
    player player = {};
    load(&player);

    /* main menu */
    while (1) {
        system("clear");
        title(TITLE);
        status(&player);
        switch (menu_of(5, "Új játékos indítása", "Harc", "Felszerelés", "Ellenségek", "Dobókocka")) {
            case 1:
                create(&player);
                player.inventory = setup(player.inventory);
                save(&player);
                break;
            case 2:
                puts("harc");
                break;
            case 3:
                player.inventory = inventory_menu(&player);
                break;
            case 4:
                puts("ellenségek");
                break;
            case 5:
                puts("dobókocka");
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
    char **p, name[NAME_LENGTH];
    int n, quantity, initial_charge, charge, mod_dp, mod_hp, mod_lp;
    fp = fopen(SAVEFILE, "r");
    if (fp != NULL) {
        /* restore player's attributes */
        getcsv(fp);
        p = csvfield;
        strcpy(player->name, *p);
        player->dp = atoi(*++p);
        player->hp = atoi(*++p);
        player->lp = atoi(*++p);
        player->initial_dp = atoi(*++p);
        player->initial_hp = atoi(*++p);
        player->initial_lp = atoi(*++p);
        /* restore inventory */
        free_inventory(player->inventory);
        n = getcsv(fp) / 7; /* an item has seven attributes */
        p = csvfield;
        while (n--) {
            strcpy(name, *p++);
            quantity = atoi(*p++);
            initial_charge = atoi(*p++);
            charge = atoi(*p++);
            mod_dp = atoi(*p++);
            mod_hp = atoi(*p++);
            mod_lp= atoi(*p++);
            player->inventory = take(player->inventory, new(name, quantity, initial_charge, charge, mod_dp, mod_hp, mod_lp));
        }
        fclose(fp);
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
    strcpy(player->name, "");
    player->dp = player->initial_dp = 0;
    player->hp = player->initial_hp = 0;
    player->lp = player->initial_lp = 0;
    free_inventory(player->inventory);
    
    system("clear");
    status(player);

    printf("Mi a neved, kalandor? ");
    scanf("%32s", player->name);

    player->initial_dp = roll_dice(6) + 6;
    player->dp = player->initial_dp;

    player->initial_hp = roll_dice(6) + roll_dice(6) + 12;
    player->hp = player->initial_hp;

    player->initial_lp =roll_dice(6) + 6;
    player->lp = player->initial_lp;

    system("clear");
    status(player);
}

void save(player *player) {
    FILE *fp;

    if ((fp = fopen(SAVEFILE, "w")) != NULL) {
        /* save basic attributes in the first line */
        fprintf(fp, "%s;%d;%d;%d;%d;%d;%d\n",
            player->name, player->dp, player->hp, player->lp,
            player->initial_dp, player->initial_hp, player->initial_lp);
        /* save inventory in the second line */
        items2csv(player->inventory, fp);
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
    head = take(head, new("élelem", 1, 10, 10, 0, 4, 0)); /* add ten units of food */
    puts("Megkaptad a kardodat, a bőrpáncélodat és a tíz adag élelmet.");
    puts("Válassz egyet a varázsitalok közül!");
    head = potion(head); /* choose a potion and add to inventory */
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

void items2csv(item* head, FILE *fp) {
    item *p; // preserve head
    for (p = head; p != NULL; p = p->next) {
        fprintf(fp, "%s;%d;%d;%d;%d;%d;%d;",
            p->name, p->quantity, p->initial_charge, p->charge, p->mod_dp, p->mod_hp, p->mod_lp);
    }
    fseek(fp, -1, SEEK_CUR); // remove ending semicolon
    fputc('\n', fp);
}

int getcsv(FILE *fp) {
    int n = 0;
    char *p, *q;
    if (fgets(csvbuffer, sizeof(csvbuffer), fp) == NULL) {
        return -1;
    }
    for (q = csvbuffer; (p = strtok(q, ";")) != NULL; q = NULL) {
        csvfield[n++] = p;
    }
    return n;
}

void free_inventory(item *head) {
    item *next;
    for (; head != NULL; head = next) {
        next = head->next;
        free(head);
    }
}

item *potion(item *head) {
    switch (menu_of(3, "Ügyesség", "Életerő", "Szerencse")) {
        case 1:
            head = take(head, new("ügyesség-varázsital", 1, 2, 2, 12, 0, 0));
            break;
        case 2:
            head = take(head, new("életerő-varázsital", 1, 2, 2, 0, 24, 0));
            break;
        case 3: /* fall through */
        default:
            head = take(head, new("szerencse-varázsital", 1, 2, 2, 0, 0, 12));
    }
    return head;
}

item *inventory_menu(player *player) {
    item *p;
    int i, choice;
    if (player->inventory != NULL) {
        while (1) {
            i = 0, choice = -1;
            system("clear");
            puts("Az alábbi lehetőségeid vannak:");
            puts(LINE);

            /* option for new equipment */
            printf("[%d] Új felszerelés\n", i++);

            /* list all items currently in inventory */
            for (p= player->inventory; p != NULL; p = p->next, ++i) {
                printf("[%d] %s: %ddb", i, p->name, p->quantity);
                if (p->initial_charge > 0) {
                    printf(" %d/%d", p->charge, p->initial_charge);
                }
                if (p->mod_dp != 0) {
                    printf(" %+dÜ", p->mod_dp);
                }
                if (p->mod_hp != 0) {
                    printf(" %+dÉ", p->mod_hp);
                }
                if (p->mod_lp != 0) {
                    printf(" %+dSz", p->mod_lp);
                }
                putchar('\n');
            }        

            /* exit from inventory menu */
            printf("[%d] Kilépés\n", i);

            puts(LINE);
            printf("Válassz egyet és nyomj Enter-t! ");
            choice = getchar() - '1' + 1;

            if (choice == i) { /* exit inventory menu */
                break;
            } else if (choice == 0) { /* create and take a new item */
                player->inventory = new2inventory(player->inventory);
                save(player);
            } else { /* proceed to item menu */
                ;
            }
        }
    }
    return player->inventory;
}

item *new2inventory(item *head) {
    char name[NAME_LENGTH];
    int quantity, initial_charge, mod_dp, mod_hp, mod_lp;

    system("clear");
    
    printf("Kérem a tárgy nevét: ");
    scanf("%s", name);
    printf("Hány darab? ");
    scanf("%d", &quantity);
    printf("Töltet (-1, ha nem használódik el): ");
    scanf("%d", &initial_charge);
    printf("Ügyesség-módosító: ");
    scanf("%d", &mod_dp);
    printf("Életerő-módosító: ");
    scanf("%d", &mod_hp);
    printf("Szerencse-módosító: ");
    scanf("%d", &mod_lp);

    return take(head, new(name, quantity, initial_charge, initial_charge, mod_dp, mod_hp, mod_lp));
}