#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

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
    ADD_VALUE = 6,
    ENEMY_ATTR = 5,
    BASE_HIT = 2
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
void free_inventory(item*); /* delete all items in the inventory */
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
int lucktrial(player*); /* try your luck according to game rules */
void dice_roll(void); /* roll two dices, display them and their sum */
bool fight(player*); /* fighting procedure, returns true if player wins */
enemy *encounter(char*, int, int, int, int); /* create a new enemy struct */
enemy *enlist(enemy*, enemy*); /* add a new enemy to list */
void repr_player(player*); /* short representation of the player */
void repr_enemy(enemy*); /* short representation of an enemy */
enemy *dereference(enemy*, enemy*); /* remove reference of an enemy from list (without freeing) */
bool enemy_kills(player*, int); /* check wether enemy kills the player with its blow */
void enemies2csv(enemy*, FILE*); /* convert enemy struct to csv */
void chronicle(enemy*); /* list all beaten enemies */
void progress(player*); /* save and show player's progress (paragraph) in the game */
void free_beaten(enemy*); /* delete list of beaten enemies */

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
    int initial_dp;
    int initial_hp;
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
    int progress;
    item *inventory; // implemented as a linked list
    enemy *roster; // linked list holding current enemies
    enemy *beaten; // linked list holding all defeated enemies
};

char csvbuffer[MAX_LINE];
char answerbuffer[MAX_ANSWER];
char *csvfield[MAX_CSV_FIELD];

int main() {
    srand(time(NULL));
    player player = {};
    player.inventory = NULL;
    player.roster = NULL;
    player.beaten = NULL;
    player.progress = 1;
    load(&player);

    /* main menu */
    while (1) {
        system("clear");
        title(TITLE);
        status(&player);
        switch (menu_of(6, "új játékos indítása", "harc", "felszerelés", "legyőzőtt ellenségek", "dobókocka", "játékállás")) {
            case 1:
                create(&player);
                player.inventory = setup(player.inventory);
                save(&player);
                break;
            case 2:
                if (player.hp > 0) {
                    if (fight(&player)) {
                        save(&player);
                        break;
                    } else {
                        puts("A kalandod sajnos véget ért!");
                        save(&player);
                        exit(0);
                    }
                }
                break;
            case 3:
                if (player.hp > 0) {
                    player.inventory = inventory_menu(&player);
                }
                break;
            case 4:
                chronicle(player.beaten);
                break;
            case 5:
                if (player.hp > 0) {
                    luckmenu(&player);
                }
                break;
            case 6:
                if (player.hp > 0) {
                    progress(&player);
                    save(&player);
                }
                break;
            case 7:
                exit(0);
        }
    }

    return 0;
}

void load(player *player) {
    FILE *fp;
    char **p, name[MAX_ANSWER];
    int n, quantity, initial_charge, charge, mod_dp, mod_hp, mod_lp, dp, hp;
    fp = fopen(SAVEFILE, "r");
    if (fp != NULL) {
        /* restore player's attributes */
        getcsv(fp);
        p = csvfield;
        strcpy(player->name, *p);
        player->dp = toint(*++p);
        player->hp = toint(*++p);
        player->lp = toint(*++p);
        player->initial_dp = toint(*++p);
        player->initial_hp = toint(*++p);
        player->initial_lp = toint(*++p);
        player->progress = toint(*++p);
        /* restore inventory */
        n = getcsv(fp) / ITEM_ATTR; /* an item has seven attributes */
        p = csvfield;
        while (n--) {
            strcpy(name, *p++);
            quantity = toint(*p++);
            initial_charge = toint(*p++);
            charge = toint(*p++);
            mod_dp = toint(*p++);
            mod_hp = toint(*p++);
            mod_lp= toint(*p++);
            player->inventory = take(player->inventory,
                new(name, quantity, initial_charge, charge, mod_dp, mod_hp, mod_lp));
        }
        /* restore beaten enemies */
        n = getcsv(fp) / ENEMY_ATTR; /* enemies have five attributes */
        p = csvfield;
        while (n--) {
            strcpy(name, *p++);
            mod_dp = toint(*p++);
            mod_hp = toint(*p++);
            dp = toint(*p++);
            hp = toint(*p++);
            player->beaten = enlist(player->beaten, encounter(name, mod_dp, mod_hp, dp, hp));
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
    printf("[%d] vissza\n", i + 1);
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
    player->progress = 1;
    free_beaten(player->beaten);
    
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
        fprintf(fp, "%s;%d;%d;%d;%d;%d;%d;%d\n",
            player->name, player->dp, player->hp, player->lp,
            player->initial_dp, player->initial_hp, player->initial_lp, player->progress);
        /* save inventory in the second line */
        items2csv(player->inventory, fp);
        enemies2csv(player->beaten, fp);
        fclose(fp);
    } else {
        puts("Some really nasty error occured.");
        puts("Unable to save to file.");
        exit(1);
    }    
}

item *new(char *name, int quantity, int initial_charge, int charge, int mod_dp, int mod_hp,
    int mod_lp) {
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
        newitem->next = NULL;
        return newitem;
    } else {
        puts("Some really nasty error occured.");
        puts("Unable allocate enough memory.");
        exit(1);
    }
}

item *take(item *head, item *newitem) {
    item *p;
    if (head == NULL) {
        return newitem; // first item in the inventory
    } else {
        item *exist = lookup(head, newitem->name);
        if (exist == NULL) { // new item
            for (p = head; p->next !=NULL; p = p->next);
            p->next = newitem; // add item to end of list
        } else { // duplicate entry
            exist->quantity += newitem->quantity; // increase quantity
        }
        return head;
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

item *lookup(item *head, char *name) {
    if (head != NULL && strcmp(head->name, name)) {
        return lookup(head->next, name);
    }
    return head;
}

void items2csv(item* head, FILE *fp) {
    for (; head != NULL; head = head->next) {
        fprintf(fp, "%s;%d;%d;%d;%d;%d;%d;",
            head->name, head->quantity, head->initial_charge, head->charge, head->mod_dp, 
            head->mod_hp, head->mod_lp);
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
            printf("[%d] vissza\n", i);

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
    /* only item with a valid charge value may be consumed */
    if (item->charge > 0 && item->quantity > 0) {
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
                dice_roll();
                break;
            default:
                return;
        }
    }
}

int lucktrial(player *player) {
    bool result;
    if (player->lp < 1) {
        puts("Nem tehetsz szerencsepróbát!");
        return 0;
    } else {
        int trial = roll_dice(6) + roll_dice(6);
        result = trial > player->lp;
        printf("A dobás (%d) %s, mint a szerencse (%d).\n", trial, result ? "nagyobb" : "kisebb vagy egyenlő", player->lp);
        printf("A szerencse-próbát %s!\n", result ? "ELBUKTAD" : "MEGNYERTED");
        --player->lp;
    }
    puts("Nyomj Enter-t!");
    while ((getchar() != '\n'));
    return result ? 1 : 2;
}

void dice_roll(void) {
    int first = roll_dice(6), second = roll_dice(6);
    printf("Első kockadobás: %d\n", first);
    printf("Második kockadobás: %d\n", second);
    printf("Összegük: %d\n", first + second);
    puts("Nyomj Enter-t!");
    while ((getchar() != '\n'));
}

bool fight(player *player) {
    bool detailled = true, manually = false, separately = true;
    int enemies, i, hit, player_attack, enemy_attack;
    char name[MAX_ANSWER], dp, hp;
    enemy *current_enemy, *next_enemy;

    system("clear");
    status(player);

    /* choose battle mod: just ending result or show rounds or detailled
        the first two don't allow the lucky trial or to escape from battle */
    puts("Hogyan szeretnéd a csatát?");
    switch (menu_of(3, "csak végeredmény", "részletek is", "kézi")) {
        case 1:
            detailled = false;
            break;
        case 3:
            manually = true;
            break;
        case 4:
            return true;
    }

    /* enter number of attacking enemies */
    if ((enemies = toint(answer("ellenfeleid száma"))) < 1) {
        enemies = 1;
    }

    /* multiple enemies can attack at the same time or one after the other */
    if (enemies > 1) {
        puts("Hogyan támadnak az ellenfeleid?");
        if (menu_of(2, "egyszerre", "egymás után") == 1) {
            separately = false;
        }
    }

    /* enter enemy data: name, dexterity and hit points */
    for (i = 1; i <= enemies; ++i) {
        if (enemies > 1) {
            printf("%d. ", i);
        }
        strcpy(name, answer("ellenfeled neve"));
        dp = toint(answer("    - ügyessége"));
        hp = toint(answer("    - életereje"));
        player->roster = enlist(player->roster, encounter(name, dp, hp, dp, hp));
    }
    
    /* battle loop */
    next_enemy = player->roster;
    while (next_enemy != NULL) {

        /* one round in the battle */
        current_enemy = next_enemy;
        if (detailled) {
            puts(LINE);            
            repr_player(player);
            printf(" vs ");
            repr_enemy(current_enemy);
            puts("");
        }
        player_attack = roll_dice(6) + roll_dice(6) + player->dp;
        enemy_attack = roll_dice(6) + roll_dice(6) + current_enemy->dp;
        hit = BASE_HIT;
        if (detailled) {
            printf("%s támadóereje: %d, %s támadóereje: %d\n",
                player->name, player_attack, current_enemy->name, enemy_attack);
        }
        if (manually) {
            if (menu_of(1, "szökés") == 1) {
                puts("Próbára teszed a szerencséd?");
                if (menu_of(1, "szerencse-próba") == 1) {
                    switch (lucktrial(player)) {
                        case 1:
                            hit += 1;
                            break;
                        case 2:
                            hit -= 1;
                            break;
                        default:
                            break;
                    }
                }
                if (enemy_kills(player, hit)) {
                    puts("Menekülés közben az ellenfeled halálos csapást mért rád!");
                    return false;
                } else {
                    puts("Bár ellenfeled még utoljára megsebzett, sikerült elmenekülnöd!");
                    return true;
                }
            }
        }
        if (player_attack == enemy_attack) {
            if (detailled) {
                puts("Döntetlen kör!");
            }
        } else if (player_attack > enemy_attack) {
            if (detailled) {
                puts("Te nyerted a kört!");
            }
            if (manually) {
                if (menu_of(1, "szerencse-próba") == 1) {
                    switch (lucktrial(player)) {
                        case 1:
                            hit -= 1;
                            break;
                        case 2:
                            hit += 2;
                            break;
                        default:
                            break;
                    }
                }
            }
            current_enemy->hp -= hit;
            if (current_enemy->hp < 1) {
                if (detailled) {
                    puts("Megölted az ellenfeledet!");
                }
                next_enemy = current_enemy->next;
                player->beaten = enlist(player->beaten, current_enemy);
                player->roster = dereference(player->roster, current_enemy);
            }
        } else {
            if (detailled) {
                puts("Ellenfeled nyerte a kört!");
            }
            if (manually) {
                if (menu_of(1, "szerencse-próba") == 1) {
                    switch (lucktrial(player)) {
                        case 1:
                            hit += 1;
                            break;
                        case 2:
                            hit -= 1;
                            break;
                        default:
                            break;
                    }
                }
            }
            if (enemy_kills(player, hit)) {
                if (detailled) {
                    puts("Ellenfeled megölt a csatában!");
                }
                return false;
            }
        }
        if (!separately) {
            next_enemy = current_enemy->next == NULL ? player->roster : current_enemy->next;
        }
    }
    puts("Megnyerted a csatát!");
    while ((getchar() != '\n'));
    return true;
}

enemy *encounter(char *name, int dp, int hp, int initial_dp, int initial_hp) {
    enemy *newenemy;
    newenemy = malloc(sizeof(enemy));
    if (newenemy != NULL) {
        strcpy(newenemy->name, name);
        newenemy->initial_dp = initial_dp;
        newenemy->initial_hp = initial_hp;
        newenemy->dp = dp;
        newenemy->hp = hp;
        newenemy->next = NULL;
        return newenemy;
    } else {
        puts("Some really nasty error occured.");
        puts("Unable allocate enough memory.");
        exit(1);
    }
}

enemy *enlist(enemy *head, enemy *newenemy) {
    enemy *p;
    if (head == NULL) {
        return newenemy;
    } else {
        for (p = head; p->next != NULL; p = p->next);
        p->next = newenemy;
        return head;
    }
}

void repr_player(player *player) {
    printf("%s Ü%d/%d É%d/%d Sz%d/%d", player->name, player->dp, player->initial_dp, player->hp,
        player->initial_hp, player->lp, player->initial_lp);
}

void repr_enemy(enemy *enemy) {
    if (enemy == NULL) {
        puts("Nothing in the list!");
    } else {
        printf("%s Ü%d/%d É%d/%d",
            enemy->name, enemy->dp, enemy->initial_dp, enemy->hp, enemy->initial_hp);
    }
}

enemy *dereference(enemy *head, enemy *defeated) {
    /* it is assumed that enemies have unique names as in the book */
    enemy *p, *prev = NULL;
    for (p = head; p != NULL; p = p->next) {
        if (strcmp(p->name, defeated->name) == 0) {
            if (prev == NULL) { // first enemy in the list
                head = p->next;
            } else {
                prev->next = p->next;
            }
            p->next = NULL;
            return head;
        }
        prev = p;
    }
    return head;
}

bool enemy_kills(player *player, int hit) {
    player->hp -= hit;
    if (player->hp < 1) {
        return true;
    }
    return false;
}

void enemies2csv(enemy* head, FILE *fp) {
    if (head != NULL) {
        for (; head != NULL; head = head->next) {
            fprintf(fp, "%s;%d;%d;%d;%d;", head->name, head->dp, head->hp, head->initial_dp, 
                head->initial_hp);
        }
        fseek(fp, -1, SEEK_CUR); // remove ending semicolon
        fputc('\n', fp);
    }
}

void chronicle(enemy *head) {
    system("clear");
    puts("Legyőzött ellenségeid:");
    puts(LINE);
    for (; head != NULL; head = head->next) {
        repr_enemy(head);
        puts("");
    }
    puts(LINE);
    puts("Nyomj Enter-t!");
    while ((getchar() != '\n'));
}

void progress(player *player) {
    system("clear");
    printf("Jelenleg a %d. bekezdésnél tartasz.\n", player->progress);
    switch (menu_of(1, "állás mentése")) {
        case 1:
            player->progress = toint(answer("Új bekezdés"));
    }
}

void free_beaten(enemy *head) {
    enemy *next;
    for (; head != NULL; head = next) {
        next = head->next;
        free(head);
    }
}