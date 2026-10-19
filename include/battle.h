#ifndef BATTLE_H
#define BATTLE_H
#include <stdint.h>
#define SPECIES_COUNT 6
#define MOVE_COUNT 4
#define LOG_COUNT 8
typedef enum { NORMAL, FIRE, WATER, GRASS, ELECTRIC, GROUND, TYPE_COUNT } Type;
typedef enum { DAMAGE, HEAL, GUARD } Effect;
typedef struct { const char *name; Type type; int power, accuracy, pp; Effect effect; } Move;
typedef struct { const char *name; Type type; int hp, attack, defense, speed; int moves[4]; } Species;
typedef struct { int species, hp, pp[4], guard; } Fighter;
typedef struct { Fighter player, bot; uint32_t rng; int turn, result; char log[LOG_COUNT][50]; int log_count; } Battle;
typedef struct { int rating, wins, losses; } Profile;
extern const Move moves[];
extern const Species species[SPECIES_COUNT];
extern const char *type_names[TYPE_COUNT];
void battle_init(Battle *, int, int, uint32_t);
int battle_turn(Battle *, int, int);
int ai_choose(Battle *, int);
int effectiveness(Type, Type); /* percentage */
int profile_apply(Profile *, int difficulty, int won);
int profile_load(Profile *, const char *);
int profile_save(const Profile *, const char *);
#endif
