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
    MAX_ANSWER = 32,
    MAX_LINE = 1000,
    MAX_CSV_FIELD = 100,
    ITEM_ATTR = 7,
    MAX_DP = 12,
    MAX_HP = 24,
    MAX_LP = 12,
    ADD_VALUE = 6
};

#define roll_dice(n) (rand() % (n) + 1)
#define toint(s) ((int) strtol((s), NULL, 10))

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
char *answer(char*); /* ask player a question, store the answer in the buffer */
item *itemmenu(player*, int); /* handle items in the inventory */
void consume(player*, item*); /* consume an item */
item *drop(item*, item*); /* drop an item from inventory (decrease its quantity) */
item *purge(item*); /* remove all 0-quantity items from inventory */
void repr_item(item*, int); /* short, numbered representation of an item in one line */
void luckmenu(player*); /* handle any dice roll related tasks */
void lucktrial(player*); /* try your luck according to game rules */

struct item {
    char name[MAX_ANSWER];
    int quantity;
    int initial_charge; // how many times the item can be used, -1 for unlimited usage
    int charge; // current charge, -1 for unlimited usage
    int mod_dp; // possible modifications to the player's attributes
    int mod_hp;
    int mod_lp;
    item *next; // pointer to the next item
};

struct enemy {
    char name[MAX_ANSWER];
    int dp;
    int hp;
    enemy *next;
};

struct player {
    char name[MAX_ANSWER];
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
char answerbuffer[MAX_ANSWER];
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
        switch (menu_of(5, "új játékos indítása", "harc", "felszerelés", "ellenségek", "dobókocka")) {
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
                luckmenu(&player);
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
    char **p, name[MAX_ANSWER];
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
        n = getcsv(fp) / ITEM_ATTR; /* an item has seven attributes */
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
    printf("kalandor: %15s | ügyesség: %2d/%2d | életerő: %2d/%2d | szerencse:  %2d/%2d\n",
    player->name, player->dp, player->initial_dp, player->hp, player->initial_hp, player->lp, player->initial_lp);
    puts(TAGGED_LINE);
}

int menu_of(int argc, ...) {
    int i, choice;
    va_list menup;
    va_start(menup, argc);
    puts("választási lehetőségeid:");
    puts(LINE);
    for (i = 0; i < argc; ++i) {
        printf("[%d] %s\n", i + 1, va_arg(menup, char*));
    }
    printf("[%d] kilépés\n", i + 1);
    puts(LINE);
    while (1) {
        choice = toint(answer("választásod"));
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

    player->initial_dp = roll_dice(6) + ADD_VALUE;
    player->dp = player->initial_dp;

    player->initial_hp = roll_dice(6) + roll_dice(6) + ADD_VALUE*2;
    player->hp = player->initial_hp;

    player->initial_lp =roll_dice(6) + ADD_VALUE;
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
    switch (menu_of(3, "ügyesség", "életerő", "szerencse")) {
        case 1:
            head = take(head, new("ügyesség-varázsital", 1, 2, 2, MAX_DP, 0, 0));
            break;
        case 2:
            head = take(head, new("életerő-varázsital", 1, 2, 2, 0, MAX_HP, 0));
            break;
        case 3: /* fall through */
        default:
            head = take(head, new("szerencse-varázsital", 1, 2, 2, 0, 0, MAX_LP));
    }
    return head;
}

item *inventory_menu(player *player) {
    item *p;
    int i, choice;
        while (1) {
            i = 0, choice = -1;
            system("clear");
            status(player);
            puts("választási lehetőségeid:");
            puts(LINE);

            /* option for new equipment */
            printf("[%d] új felszerelés\n", i++);

            /* list all items currently in inventory */
            for (p = player->inventory; p != NULL; p = p->next, ++i) {
                repr_item(p, i);
            }        

            /* exit from inventory menu */
            printf("[%d] kilépés\n", i);

            puts(LINE);
            choice = toint(answer("választásod"));

            if (choice == i) { /* exit inventory menu */
                break;
            } else if (choice == 0) { /* create and take a new item */
                player->inventory = new2inventory(player->inventory);
                save(player);
            } else { /* proceed to item menu */
                player->inventory = itemmenu(player, choice);
                player->inventory = purge(player->inventory);
                save(player);
            }
        }
    return player->inventory;
}

item *new2inventory(item *head) {
    char name[MAX_ANSWER];
    int quantity, initial_charge, mod_dp, mod_hp, mod_lp;

    system("clear");
    
    strcpy(name, answer("tárgy neve"));
    quantity = toint(answer("mennyiség"));
    initial_charge = toint(answer("töltet (-1, ha nem használódik el)"));
    mod_dp = toint(answer("ügyesség-módosító"));
    mod_hp = toint(answer("életerő-módosító"));
    mod_lp = toint(answer("szerencse-módosító"));

    return take(head, new(name, quantity, initial_charge, initial_charge, mod_dp, mod_hp, mod_lp));
}

char *answer(char *question) {
    char *p;
    printf("%s: ", question);
    fgets(answerbuffer, MAX_ANSWER-1, stdin);
    if ((p = strchr(answerbuffer, '\n')) != NULL) {
        *p = '\0';
    }
    return answerbuffer;
}

item *itemmenu(player *player, int choice) {
    item *item = player->inventory;
    /* identify item */
    while (--choice > 0) {
        item = item->next;
    }
    /* bring up item menu */
    while (1) {
        system("clear");
        status(player);
        repr_item(item, -1);
        puts(LINE);
        switch (menu_of(2, "elfogyasztás", "eldobás")) {
            case 1:
                consume(player, item);
                break;
            case 2:
                return drop(player->inventory, item);
            default:
                return player->inventory;

        }
    }
}

void consume(player *player, item *item) {
    if (item->charge > 0 && item->quantity > 0) { /* only item with a valid charge value can be consumed */
        --item->charge;
        if (item->mod_dp) {
            player->dp += item->mod_dp;
            if (player->dp > player->initial_dp) {
                player->dp = player->initial_dp;
            }
        }
        if (item->mod_hp) {
            player->hp += item->mod_hp;
            if (player->hp > player->initial_hp) {
                player->hp = player->initial_hp;
            }
        }
        if (item->mod_lp) {
            player->lp += item->mod_lp;
            if (player->lp > player->initial_lp) {
                player->lp = player->initial_lp;
            }
        }
        if (strcmp(item->name, "szerencse-varázsital") == 0) {
            ++(player->initial_lp);
            player->lp = player->initial_lp;
        }
        if (item->charge <= 0) {
            if (--item->quantity > 0) {
                item->charge = item->initial_charge;
            }
        }
    }
}

item *drop(item *head, item *item) {
    --item->quantity;
    return purge(head);
}

item *purge(item *head) {
    item *tmp;
    if (head != NULL) {
        if (head->quantity <= 0) {
            tmp = head->next;
            free(head);
            return purge(tmp);
        } else {
            head->next = purge(head->next);
        }
    }
    return head;
}

void repr_item(item *item, int i) {
    if (i >= 0) {
        printf("[%d] ", i);
    }
    printf("%s: %ddb", item->name, item->quantity);
    if (item->initial_charge > 0) {
        printf(" %d/%d", item->charge, item->initial_charge);
    }
    if (item->mod_dp != 0) {
        printf(" %+dÜ", item->mod_dp);
    }
    if (item->mod_hp != 0) {
        printf(" %+dÉ", item->mod_hp);
    }
    if (item->mod_lp != 0) {
        printf(" %+dSz", item->mod_lp);
    }
    putchar('\n');
}

void luckmenu(player *player) {
    while (1) {
        system("clear");
        status(player);
        switch (menu_of(2, "szerencse-próba", "kockadobás")) {
            case 1:
                lucktrial(player);
                save(player);
                break;
            case 2:
                // dice_roll();
                break;
            default:
                return;
        }
    }
}

void lucktrial(player *player) {
    if (player->lp < 1) {
        puts("Nem tehetsz szerencsepróbát!");
    } else {
        int trial = roll_dice(6) + roll_dice(6);
        printf("A dobás (%d) %s, mint a szerencse (%d).\n", trial, trial > player->lp ? "nagyobb" : "kisebb vagy egyenlő", player->lp);
        printf("A szerencse-próbát %s!\n", trial > player->lp ? "ELBUKTAD" : "MEGNYERTED");
        --player->lp;
    }
    puts("Nyomj Enter-t!");
    while ((getchar() != '\n'));
}