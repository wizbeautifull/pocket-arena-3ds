#include "battle.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
const char *type_names[] = {"Normal","Fire","Water","Grass","Electric","Ground"};
const Move moves[] = {
 {"Tackle",NORMAL,40,100,35,DAMAGE},
 {"Flamethrower",FIRE,90,100,15,DAMAGE},
 {"Water Pulse",WATER,60,100,20,DAMAGE},
 {"Energy Ball",GRASS,90,100,10,DAMAGE},
 {"Thunderbolt",ELECTRIC,90,100,15,DAMAGE},
 {"Earthquake",GROUND,100,100,10,DAMAGE},
 {"Recover",NORMAL,0,100,3,HEAL},
 {"Guard",NORMAL,0,100,8,GUARD},
 {"Fire Blast",FIRE,110,85,5,DAMAGE},
 {"Hydro Pump",WATER,110,80,5,DAMAGE},
 {"Thunder",ELECTRIC,110,70,10,DAMAGE},
 {"Razor Leaf",GRASS,55,95,25,DAMAGE},
 {"Slam",NORMAL,80,75,20,DAMAGE}
};
const Species species[] = {
 {"Pikachu",ELECTRIC,125,100,80,130,{4,10,0,7}},
 {"Charmander",FIRE,135,100,90,105,{1,8,0,7}},
 {"Squirtle",WATER,150,90,120,85,{2,9,0,7}},
 {"Bulbasaur",GRASS,145,100,100,90,{3,11,0,6}},
 {"Sandshrew",GROUND,150,115,120,75,{5,12,0,7}},
 {"Eevee",NORMAL,155,105,95,100,{0,12,6,7}}
};
static uint32_t rnd(Battle *b) { uint32_t x=b->rng; x^=x<<13; x^=x>>17; x^=x<<5; return b->rng=x; }
static void logline(Battle *b,const char *fmt,...) {
 if(b->log_count==LOG_COUNT) { memmove(b->log,b->log+1,sizeof(b->log[0])*(LOG_COUNT-1)); b->log_count--; }
 va_list a; va_start(a,fmt); vsnprintf(b->log[b->log_count++],50,fmt,a); va_end(a);
}
int effectiveness(Type a,Type d) {
 static const int table[TYPE_COUNT][TYPE_COUNT]={
 {100,100,100,100,100,100}, {100,50,50,200,100,100},
 {100,200,50,50,100,200}, {100,50,200,50,100,200},
 {100,100,200,50,50,0}, {100,200,100,50,200,100}};
 return table[a][d];
}
static Fighter fighter(int s) {
 Fighter f={0}; f.species=s; f.hp=species[s].hp;
 for(int i=0;i<4;i++) f.pp[i]=moves[species[s].moves[i]].pp;
 return f;
}
void battle_init(Battle *b,int p,int e,uint32_t seed) {
 memset(b,0,sizeof(*b)); b->rng=seed?seed:1;
 b->player=fighter(p); b->bot=fighter(e); b->turn=1;
 logline(b,"A wild challenger appears!");
}
static int has_pp(const Fighter *f) { for(int i=0;i<4;i++) if(f->pp[i]>0)return 1; return 0; }
static double expected(const Fighter *a,const Fighter *d,const Move *m) {
 const Species *s=&species[a->species],*t=&species[d->species];
 double v=((22.0*m->power*s->attack/t->defense)/50.0+2);
 if(m->type==s->type)v*=1.5;
 return v*effectiveness(m->type,t->type)/100.0;
}
int ai_choose(Battle *b,int level) {
 Fighter *a=&b->bot,*d=&b->player; const Species *s=&species[a->species];
 int available[4],n=0; for(int i=0;i<4;i++)if(a->pp[i])available[n++]=i;
 if(!n)return 0;
 if(level<=0)return available[rnd(b)%n];
 double best=-1; int choice=available[0];
 double threat=0;
 for(int i=0;i<4;i++)if(d->pp[i]) {
  const Move *m=&moves[species[d->species].moves[i]];
  if(m->effect==DAMAGE) { double x=expected(d,a,m)*m->accuracy/100.0; if(x>threat)threat=x; }
 }
 for(int k=0;k<n;k++) {
  int i=available[k]; const Move *m=&moves[s->moves[i]]; double score=0;
  if(m->effect==DAMAGE) {
   double hit=expected(a,d,m)*0.925;
   score=hit*m->accuracy/100.0;
   if(level==2 && hit>=d->hp)score+=90*m->accuracy/100.0;
  } else if(m->effect==HEAL) {
   int missing=s->hp-a->hp;
   score=(missing<s->hp/2?missing:s->hp/2)*(level==2?1.15:0.6);
   if(level==2 && a->hp<=threat && s->speed<species[d->species].speed)score*=0.1;
  } else if(level==2)score=threat*0.30;
  if(score>best) {best=score;choice=i;}
 }
 return choice;
}
static void act(Battle *b,Fighter *a,Fighter *d,int slot) {
 const Species *s=&species[a->species]; int struggle=!has_pp(a);
 Move fallback={"Struggle",NORMAL,50,100,0,DAMAGE};
 const Move *m=struggle?&fallback:&moves[s->moves[slot]];
 if(!struggle)a->pp[slot]--;
 logline(b,"%s: %s",s->name,m->name);
 if(m->effect==GUARD) { a->guard=1; return; }
 if(m->effect==HEAL) {
  a->hp+=s->hp/2; if(a->hp>s->hp)a->hp=s->hp;
  logline(b,"Health restored.");return;
 }
 if((int)(rnd(b)%100)>=m->accuracy) {logline(b,"Missed!");return;}
 double v=expected(a,d,m)*(85+(rnd(b)%16))/100.0;
 if(struggle) v=((22.0*50*s->attack/species[d->species].defense)/50+2)*0.925;
 if(d->guard)v*=0.5;
 int damage=(int)v; if(v>0 && damage<1)damage=1;
 if(damage>d->hp)damage=d->hp;
 d->hp-=damage; logline(b,"Damage: %d%s",damage,d->guard?" (guarded)":"");
 if(struggle) {a->hp-=s->hp/4; if(a->hp<0)a->hp=0;logline(b,"Struggle recoil.");}
}
int battle_turn(Battle *b,int slot,int level) {
 if(b->result || slot<0 || slot>3)return 0;
 if(has_pp(&b->player) && b->player.pp[slot]==0)return 0;
 /* AI commits before seeing the player's action. */
 int bot_slot=ai_choose(b,level);
 b->player.guard=b->bot.guard=0;
 int pguard=has_pp(&b->player)&&moves[species[b->player.species].moves[slot]].effect==GUARD;
 int bguard=has_pp(&b->bot)&&moves[species[b->bot.species].moves[bot_slot]].effect==GUARD;
 int first;
 if(pguard!=bguard)first=pguard;
 else if(species[b->player.species].speed==species[b->bot.species].speed)first=(int)(rnd(b)%2);
 else first=species[b->player.species].speed>species[b->bot.species].speed;
 Fighter *a=first?&b->player:&b->bot,*d=first?&b->bot:&b->player;
 act(b,a,d,first?slot:bot_slot);
 if(a->hp>0 && d->hp>0)act(b,d,a,first?bot_slot:slot);
 if(!b->player.hp && !b->bot.hp)b->result=3;
 else if(!b->bot.hp)b->result=1;
 else if(!b->player.hp)b->result=2;
 if(b->turn>=200 && !b->result)b->result=3;
 b->turn++; return 1;
}
int profile_apply(Profile *p,int difficulty,int won) {
 static const int rating[]={800,1100,1400};
 double expected_score=1.0/(1.0+pow(10.0,(rating[difficulty]-p->rating)/400.0));
 int delta=(int)lround(32*((won?1.0:0)-expected_score));
 if(p->rating+delta<0)delta=-p->rating;
 p->rating+=delta; if(won)p->wins++;else p->losses++; return delta;
}
static int load_one(Profile *p,const char *path) {
 *p=(Profile){1000,0,0}; FILE *f=fopen(path,"r");if(!f)return 0;
 int r,w,l; char version[16];
 int ok=fscanf(f,"%15s %d %d %d",version,&r,&w,&l)==4;
 fclose(f);
 if(!ok || strcmp(version,"PA3DS1") || r<0 || r>10000 || w<0 || w>10000000 || l<0 || l>10000000)return 0;
 *p=(Profile){r,w,l};return 1;
}
int profile_load(Profile *p,const char *path) {
 if(load_one(p,path))return 1;
 char backup[512];
 if(snprintf(backup,sizeof(backup),"%s.bak",path)>=(int)sizeof(backup))return 0;
 return load_one(p,backup);
}
int profile_save(const Profile *p,const char *path) {
 char temp[512]; if(snprintf(temp,sizeof(temp),"%s.tmp",path)>=(int)sizeof(temp))return 0;
 FILE *f=fopen(temp,"w");if(!f)return 0;
 int ok=fprintf(f,"PA3DS1 %d %d %d\n",p->rating,p->wins,p->losses)>0;
 if(fclose(f)!=0)ok=0;
 if(!ok){remove(temp);return 0;}
 /* FAT implementations may not replace an existing destination on rename.
    Move the current file aside first; load can recover the backup after an
    interrupted write. This is best-effort, not a power-loss guarantee. */
 char backup[512];
 if(snprintf(backup,sizeof(backup),"%s.bak",path)>=(int)sizeof(backup)){remove(temp);return 0;}
 int had_old=0; FILE *old=fopen(path,"r");
 if(old) {
  fclose(old);remove(backup);
  if(rename(path,backup)!=0){remove(temp);return 0;}
  had_old=1;
 }
 if(rename(temp,path)!=0){if(had_old)rename(backup,path);remove(temp);return 0;}
 return 1;
}
