#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include "battle.h"
static const char *levels[]={"Easy","Normal","Hard"};
static const char *save_path="sdmc:/3ds/pocket-arena/profile.txt";
static PrintConsole top,bottom;
static Profile profile;
static Battle battle;
static int chosen=0,level=1,slot=0,state=0,delta=0,save_ok=1;
static unsigned sequence=0;
static char hint[40]="Choose your Pokemon.";
static void finish(int result) {
 delta=0;
 if(result!=3)delta=profile_apply(&profile,level,result==1);
 save_ok=profile_save(&profile,save_path);
 state=2;
}
static void draw(void) {
 consoleSelect(&top);consoleClear();
 printf("\x1b[36m  POCKET ARENA / 3DS\x1b[0m\n");
 printf("  Offline prototype 0.1 | no online play\n\n");
 if(state==0) {
  printf("  LOCAL LADDER\n  Rating: %d   W: %d / L: %d\n\n",profile.rating,profile.wins,profile.losses);
  const Species *s=&species[chosen];
  printf("  %s [%s]\n  HP %d  ATK %d  DEF %d  SPD %d\n\n",s->name,type_names[s->type],s->hp,s->attack,s->defense,s->speed);
  printf("  Simplified single-type battles.\n  Opponent is chosen randomly.\n  Difficulty: %s\n\n",levels[level]);
  printf("  Not affiliated with Nintendo / Pokemon.\n");
 } else {
  printf("  BOT   %-12s %3d / %3d HP\n",species[battle.bot.species].name,battle.bot.hp,species[battle.bot.species].hp);
  printf("  YOU   %-12s %3d / %3d HP\n\n",species[battle.player.species].name,battle.player.hp,species[battle.player.species].hp);
  printf("  Turn %d  |  %s AI\n\n",battle.turn,levels[level]);
  for(int i=0;i<battle.log_count;i++)printf("%s\n",battle.log[i]);
  if(state==2) {
   printf("\n  %s\n",battle.result==1?"YOU WIN!":battle.result==2?"YOU LOSE":"DRAW - no rating change");
   printf("  Rating: %d (%+d)\n",profile.rating,delta);
  }
 }
 consoleSelect(&bottom);consoleClear();
 if(state==0) {
  printf("  CHOOSE YOUR FIGHTER\n\n");
  for(int i=0;i<SPECIES_COUNT;i++)printf(" %s %-12s %s\n",chosen==i?">":" ",species[i].name,type_names[species[i].type]);
  printf("\n  UP/DOWN: Pokemon\n  LEFT/RIGHT: difficulty\n\n  A: start battle\n  START: save and exit\n");
 } else if(state==1) {
  printf("  CHOOSE A MOVE\n\n");
  for(int i=0;i<4;i++) {
   const Move *m=&moves[species[chosen].moves[i]];
   printf(" %s %-13s PP %2d\n",slot==i?">":" ",m->name,battle.player.pp[i]);
   printf("   %-8s Power %3d Acc %3d\n",type_names[m->type],m->power,m->accuracy);
  }
  printf("\n  UP/DOWN: select   A: use\n  X: forfeit (counts as loss)\n  START: forfeit, save, exit\n");
  printf("  No PP left? A uses Struggle.\n");
 } else {
  printf("  BATTLE COMPLETE\n\n  A: back to selection\n  START: exit\n");
 }
 printf("\n  %s\n",hint);
 if(!save_ok)printf("  SAVE FAILED: check SD card.\n");
}
int main(void) {
 gfxInitDefault(); consoleInit(GFX_TOP,&top);consoleInit(GFX_BOTTOM,&bottom);
 mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/pocket-arena",0777);
 profile_load(&profile,save_path);save_ok=profile_save(&profile,save_path);
 srand((unsigned)time(NULL));int dirty=1;
 while(aptMainLoop()) {
  hidScanInput();u32 keys=hidKeysDown();
  if(keys&KEY_START) {
   if(state==1){battle.result=2;finish(2);}else save_ok=profile_save(&profile,save_path);
   if(save_ok)break;
   snprintf(hint,sizeof(hint),"Save failed. START retries; B exits.");dirty=1;
   draw();gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();continue;
  }
  if((keys&KEY_B)&&!save_ok)break;
  if(state==0) {
   if(keys&KEY_UP){chosen=(chosen+SPECIES_COUNT-1)%SPECIES_COUNT;dirty=1;}
   if(keys&KEY_DOWN){chosen=(chosen+1)%SPECIES_COUNT;dirty=1;}
   if(keys&KEY_LEFT){level=(level+2)%3;dirty=1;}
   if(keys&KEY_RIGHT){level=(level+1)%3;dirty=1;}
   if(keys&KEY_A) {
    int bot=rand()%SPECIES_COUNT;
    battle_init(&battle,chosen,bot,(uint32_t)time(NULL)^((uint32_t)rand())^(++sequence));
    state=1;slot=0;snprintf(hint,sizeof(hint),"Guard halves damage this turn.");dirty=1;
   }
  } else if(state==1) {
   if(keys&KEY_UP){slot=(slot+3)%4;dirty=1;}
   if(keys&KEY_DOWN){slot=(slot+1)%4;dirty=1;}
   if(keys&KEY_A) {
    if(!battle_turn(&battle,slot,level))snprintf(hint,sizeof(hint),"No PP. Choose another move.");
    else snprintf(hint,sizeof(hint),"AI commits without reading your move.");
    if(battle.result)finish(battle.result);
    dirty=1;
   }
   if((keys&KEY_X) && state==1){battle.result=2;finish(2);dirty=1;}
  } else if(keys&KEY_A){state=0;snprintf(hint,sizeof(hint),"Choose your Pokemon.");dirty=1;}
  if(dirty){draw();dirty=0;}
  gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();
 }
 gfxExit();return 0;
}
