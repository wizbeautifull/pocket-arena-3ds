#include "battle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(void) {
 assert(effectiveness(ELECTRIC,GROUND)==0);
 assert(effectiveness(WATER,FIRE)==200);
 assert(effectiveness(FIRE,WATER)==50);
 Battle b;battle_init(&b,0,4,1);
 assert(b.player.hp==125);assert(b.bot.hp==150);
 /* Hard bot takes a certain lethal attack over a low-accuracy alternative. */
 battle_init(&b,2,0,9);b.player.hp=1;assert(ai_choose(&b,2)==0);
 b.player.pp[0]=0;int hp=b.bot.hp;assert(!battle_turn(&b,0,2));assert(b.bot.hp==hp);
 /* Fast guard acts before an attack; slower guard also has priority. */
 battle_init(&b,2,0,1);assert(battle_turn(&b,3,1));assert(b.player.guard==1);
 /* Recovery caps at max HP. */
 battle_init(&b,5,5,2);b.player.hp=150;b.bot.pp[0]=b.bot.pp[1]=b.bot.pp[2]=0;
 assert(battle_turn(&b,2,0));assert(b.player.hp==155);
 /* Dead fighters do not get a second action. */
 battle_init(&b,0,2,1);b.bot.hp=1;assert(battle_turn(&b,0,2));assert(b.result==1);assert(b.player.hp==125);
 /* Struggle works with exhausted PP. */
 battle_init(&b,0,2,3);for(int i=0;i<4;i++)b.player.pp[i]=0;
 assert(battle_turn(&b,0,0));assert(b.player.hp<125);
 int battles=0;
 for(int p=0;p<6;p++)for(int e=0;e<6;e++)for(int level=0;level<3;level++)for(int seed=1;seed<=40;seed++) {
  battle_init(&b,p,e,(unsigned)seed);
  while(!b.result) {
   int selected=(b.turn+seed)%4;
   for(int n=0;n<4 && !b.player.pp[selected];n++)selected=(selected+1)%4;
   assert(battle_turn(&b,selected,level));
   assert(b.player.hp>=0 && b.player.hp<=species[p].hp);
   assert(b.bot.hp>=0 && b.bot.hp<=species[e].hp);
   assert(b.log_count<=LOG_COUNT);
   for(int i=0;i<4;i++){assert(b.player.pp[i]>=0);assert(b.bot.pp[i]>=0);}
  }
  assert(b.turn<=201);assert(!battle_turn(&b,0,level));battles++;
 }
 Profile profile={1000,0,0};assert(profile_apply(&profile,1,1)>0);assert(profile_apply(&profile,1,0)<0);
 assert(profile_save(&profile,"test-save.txt"));Profile loaded;
 assert(profile_load(&loaded,"test-save.txt"));assert(!memcmp(&profile,&loaded,sizeof(profile)));
 /* Existing save replacement and malformed input. */
 profile.wins++;assert(profile_save(&profile,"test-save.txt"));assert(profile_load(&loaded,"test-save.txt"));assert(loaded.wins==profile.wins);
 FILE *f=fopen("test-save.txt","w");fputs("PA3DS1 -1 4 5",f);fclose(f);
 assert(profile_load(&loaded,"test-save.txt")); /* backup fallback */
 remove("test-save.txt.bak");
 assert(!profile_load(&loaded,"test-save.txt"));assert(loaded.rating==1000);
 assert(!profile_save(&profile,"/nonexistent-directory/save"));remove("test-save.txt");
 printf("PASS: %d simulated battles, type chart, AI, PP, priority, recovery, KO, rating, save/load.\n",battles);
 return 0;
}
