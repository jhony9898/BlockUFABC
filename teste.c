
//#link "music_dangerstreets.s"

//#link "music_dangerstreets"

//#link "famitone5.s"
/*
A character-based surround-the-opponent game.
Reads from nametable RAM to determine collisions, and also
to help the AI avoid walls.
For more information, see "Making Arcade Games in C".
*/
void __fastcall__ famitone_update(void);
// APU (sound) support
#include "apu.h"
//#link "apu.c"
#include <stdlib.h>
#include <string.h>
#include <nes.h>
#include <joystick.h>

#include "neslib.h"
#include "title_nam.h"

// VRAM buffer module
#include "vrambuf.h"
//#link "vrambuf.c"

// link the pattern table into CHR ROM
//#link "chr_generic.s"


void __fastcall__ famitone_update(void);



#define COLS 32
#define ROWS 27

static int iy,dy;
static unsigned char wait;
static unsigned char frame_cnt;
static unsigned char bright;

//#link "Concha.s"


// read a character from VRAM.
// this is tricky because we have to wait
// for VSYNC to start, then set the VRAM
// address to read, then set the VRAM address
// back to the start of the frame.
byte getchar(byte x, byte y) {
  // compute VRAM read address
  word addr = NTADR_A(x,y);
  // result goes into rd
  byte rd;
  // wait for VBLANK to start
  ppu_wait_nmi();
  // set vram address and read byte into rd
  vram_adr(addr);
  vram_read(&rd, 1);
  // scroll registers are corrupt
  // fix by setting vram address
  vram_adr(0x0);
  return rd;
}

void cputcxy(byte x, byte y, char ch) {
  vrambuf_put(NTADR_A(x,y), &ch, 1);
}

void cputsxy(byte x, byte y, const char* str) {
  vrambuf_put(NTADR_A(x,y), str, strlen(str));
}

void clrscr() {
  vrambuf_clear();
  ppu_off();
  vram_adr(0x2000);
  vram_fill(0, 32*28);
  vram_adr(0x0);
  ppu_on_bg();
}

////////// GAME DATA

typedef struct {
  byte x;
  byte y;
  byte dir;
  word score;
  char head_attr;
  char tail_attr;
  int collided:1;
  int human:1;
} Player;

Player players[2];

byte attract;
byte gameover;
byte frames_per_move;

int START_SPEED =12;
int MAX_SPEED =5;
int MAX_SCORE =7;
int MAX_BLOCK =10;

///////////

const char BOX_CHARS[8] = { '+','+','+','+','-','-','!','!' };

void draw_box(byte x, byte y, byte x2, byte y2, const char* chars) {
  byte x1 = x;
  cputcxy(x, y, chars[2]);
  cputcxy(x2, y, chars[3]);
  cputcxy(x, y2, chars[0]);
  cputcxy(x2, y2, chars[1]);
  while (++x < x2) {
    cputcxy(x, y, chars[5]);
    cputcxy(x, y2, chars[4]);
  }
  while (++y < y2) {
    cputcxy(x1, y, chars[6]);
    cputcxy(x2, y, chars[7]);
  }
}
void set_sounds() {

  // these channels decay, so ok to always enable
  byte enable = ENABLE_PULSE0|ENABLE_PULSE1|ENABLE_NOISE;
  // missile fire sound



     APU_PULSE_DECAY(0,440, DUTY_50, 4,20);
	
    //APU_PULSE_SET_VOLUME(0, DUTY_50, 0);
 
  // enemy explosion sound
    //APU_NOISE_DECAY(8 , 5, 15);
 
    //APU_NOISE_DECAY(8 , 2, 8);
  
  // set diving sounds for spaceships

  
  APU_ENABLE(enable);
}
void set_sounds_Music() {
    byte enable = ENABLE_PULSE0|ENABLE_PULSE1|ENABLE_NOISE;
    //APU_PULSE_SUSTAIN(0,1000, DUTY_50,4);
    //APU_PULSE_SUSTAIN(1,666, DUTY_50,4);
  //APU_PULSE_SWEEP(0,100,1,1);
 
    APU_PULSE_DECAY(0,440, DUTY_50, 4,20);
    APU_PULSE_DECAY(0,220, DUTY_50, 4,20);
    APU_PULSE_DECAY(0,880, DUTY_50, 4,20);
    APU_PULSE_DECAY(1,440, DUTY_50, 4,20);
    APU_PULSE_DECAY(1,220, DUTY_50, 4,20);
    APU_PULSE_DECAY(1,880, DUTY_50, 4,20);
  APU_PULSE_SWEEP(0,0,20,30);
  
  

  APU_ENABLE(enable);
 
}
void set_sounds_win1() {
  byte enable = ENABLE_PULSE0|ENABLE_PULSE1|ENABLE_NOISE;

    APU_PULSE_SUSTAIN(0,440, DUTY_50,4);
    APU_PULSE_SUSTAIN(0,240, DUTY_50,4);
    APU_PULSE_SWEEP(0,0,20,30);	
  APU_ENABLE(enable);
}
void set_sounds_win2() {
  byte enable = ENABLE_PULSE0|ENABLE_PULSE1|ENABLE_NOISE;

    APU_PULSE_SUSTAIN(0,466, DUTY_50,4);
    APU_PULSE_SUSTAIN(0,233, DUTY_50,4);
  	APU_PULSE_SWEEP(0,4000,20,0);
  APU_ENABLE(enable);
}
void set_sounds_win3() {
  byte enable = ENABLE_PULSE0|ENABLE_PULSE1|ENABLE_NOISE;

    APU_PULSE_SUSTAIN(0,880, DUTY_50,4);
    APU_PULSE_SUSTAIN(0,440, DUTY_50,4);
    APU_PULSE_SWEEP(0,0,20,30);	
  APU_ENABLE(enable);
}


void set_sounds_move() {
  byte enable = ENABLE_PULSE0|ENABLE_PULSE1|ENABLE_NOISE;

  APU_NOISE_DECAY(400 , 2, 18);
  


 
  APU_ENABLE(enable);
}


void set_sounds_colision() {
  byte enable = ENABLE_PULSE0|ENABLE_PULSE1|ENABLE_NOISE;

  APU_NOISE_DECAY(3 , 2, 18);      
APU_PULSE_SUSTAIN(0,50, DUTY_50, 4);
  APU_PULSE_SET_VOLUME(0, DUTY_50, 0);
 
  APU_ENABLE(enable);
}
void set_sounds_Desliga() {
   byte enable = ENABLE_PULSE0|ENABLE_PULSE1|ENABLE_NOISE;
  APU_PULSE_SET_VOLUME(0,DUTY_50,0);
  APU_PULSE_SET_VOLUME(1,DUTY_50,0);
}
void draw_random_blocks(int num_blocks) {
  int i = 0;
  for (i = 0; i < num_blocks; i++) {
    int x = rand() % (COLS - 3) + 2; // Gera X aleatório dentro do campo
    int y = rand() % (ROWS - 3) + 2; // Gera Y aleatório dentro do campo
    cputcxy(x, y, '#'); // Desenha o bloco (ou caractere aleatório)
  }
}

void draw_playfield() {
  draw_box(1, 2, COLS - 2, ROWS - 1, BOX_CHARS);
  cputcxy(9, 1, players[0].score + '0');
  cputcxy(28, 1, players[1].score + '0');

  if (attract) {
    cputsxy(3, ROWS - 1, "ATTRACT MODE - PRESS ENTER");
  } else {
    cputsxy(1, 1, "PLYR1:");
    cputsxy(20, 1, "PLYR2:");
  }

  // Gera blocos aleatórios no campo
  draw_random_blocks(MAX_BLOCK); // Exemplo: desenha 10 blocos aleatórios
}

typedef enum { D_RIGHT, D_DOWN, D_LEFT, D_UP } dir_t;
const char DIR_X[4] = { 1, 0, -1, 0 };
const char DIR_Y[4] = { 0, 1, 0, -1 };

void init_game() {
  scroll(0,0);
  memset(players, 0, sizeof(players));
  players[0].head_attr = '1';
  players[1].head_attr = '2';
  players[0].tail_attr = 0x06;
  players[1].tail_attr = 0x07;
  frames_per_move = START_SPEED;
}

void reset_players() {
  players[0].x = players[0].y = 5;
  players[0].dir = D_RIGHT;
  players[1].x = COLS-6;
  players[1].y = ROWS-6;
  players[1].dir = D_LEFT;
  players[0].collided = players[1].collided = 0;
}



void set_player_colors() {
  // Define índices diferentes no Palette_Table
  pal_col(5, 0x06);  // Cor da cauda do jogador 1
  pal_col(7, 0x07);  // Cor da cauda do jogador 2
}

void draw_player(Player* p) {
  // Aplique a cor da cauda específica do jogador com base em `p->head_attr`
  if (p == &players[0]) {
    cputcxy(p->x, p->y, p->head_attr); // Use o índice 6 para jogador 1
  } else {
    cputcxy(p->x, p->y, p->head_attr); // Use o índice 7 para jogador 2
  }
}


void move_player(Player* p) {
  if (p == &players[0]) {
    cputcxy(p->x, p->y, p->tail_attr); // Use o índice 6 para jogador 1
  } else {
    cputcxy(p->x, p->y, p->tail_attr); // Use o índice 7 para jogador 2
  }
  p->x += DIR_X[p->dir];
  p->y += DIR_Y[p->dir];
  if (getchar(p->x, p->y) != 0)
    p->collided = 1;
  	
  draw_player(p);
}

void human_control(Player* p) {
  byte dir = 0xff;
  byte joy;
  joy = joy_read (JOY_1);
  // start game if attract mode
  if (attract && (joy & JOY_START_MASK))
    gameover = 1;
  // do not allow movement unless human player
  if (!p->human) return;
  if (joy & JOY_LEFT_MASK) {
         sfx_play(2,2);
    	dir = D_LEFT;
    	set_sounds_move();

  }
  if (joy & JOY_RIGHT_MASK) {

    sfx_play(2,2);
    dir = D_RIGHT;
    set_sounds_move();
  }
  if (joy & JOY_UP_MASK) {

       sfx_play(2,2);
    dir = D_UP;
    set_sounds_move();
  }
  if (joy & JOY_DOWN_MASK) {

       sfx_play(2,2);
    dir = D_DOWN;
    set_sounds_move();
  }
  // don't let the player reverse
  if (dir < 0x80 && dir != (p->dir ^ 2)) {
    p->dir = dir;
  }
}

byte ai_try_dir(Player* p, dir_t dir, byte shift) {
  byte x,y;
  dir &= 3;
  x = p->x + (DIR_X[dir] << shift);
  y = p->y + (DIR_Y[dir] << shift);
  if (x < COLS && y < ROWS && getchar(x, y) == 0) {
    p->dir = dir;
    return 1;
  } else {
    return 0;
  }
}

void ai_control(Player* p) {
  dir_t dir;
  if (p->human) return;
  dir = p->dir;
  if (!ai_try_dir(p, dir, 0)) {
    ai_try_dir(p, dir+1, 0);
    ai_try_dir(p, dir-1, 0);
  } else {
    ai_try_dir(p, dir-1, 0) && ai_try_dir(p, dir-1, 1+(rand() & 3));
    ai_try_dir(p, dir+1, 0) && ai_try_dir(p, dir+1, 1+(rand() & 3));
    ai_try_dir(p, dir, rand() & 3);
  }
}

void flash_colliders() {
  byte i;
  // flash players that collided
  for (i=0; i<56; i++) {
    //cv_set_frequency(CV_SOUNDCHANNEL_0, 1000+i*8);
    //cv_set_attenuation(CV_SOUNDCHANNEL_0, i/2);
    if (players[0].collided) players[0].head_attr ^= 0x80;
    if (players[1].collided) players[1].head_attr ^= 0x80;
    vrambuf_flush();
    vrambuf_flush();
    draw_player(&players[0]);
    draw_player(&players[1]);
  }
  //cv_set_attenuation(CV_SOUNDCHANNEL_0, 28);
}

void make_move() {
  byte i;
  for (i=0; i<frames_per_move; i++) {
    human_control(&players[0]);
    vrambuf_flush();
  }
  ai_control(&players[0]);
  ai_control(&players[1]);
  // if players collide, 2nd player gets the point
  move_player(&players[1]);
  move_player(&players[0]);
}

void declare_winner(byte winner) {
  byte i;
  clrscr();
  set_sounds_win1();
  for (i=0; i<ROWS/2-3; i++) {
    draw_box(i,i,COLS-1-i,ROWS-1-i,BOX_CHARS);
    vrambuf_flush();
  }
  cputsxy(12,10,"VENCEDOR:");
  cputsxy(12,13,"JOGADOR ");
  cputcxy(12+7, 13, '1'+winner);
  vrambuf_flush();
  if(winner==1){
  delay(100);
  set_sounds_Desliga();
  set_sounds_win2();
  delay(100);
  set_sounds_Desliga();
  set_sounds;
  gameover = 1;
    
  }else{
      delay(100);
  set_sounds_Desliga();
  set_sounds_win3();
  delay(100);
  set_sounds_Desliga();
  set_sounds;
  gameover = 1;
  
  }

}

#define AE(tl,tr,bl,br) (((tl)<<0)|((tr)<<2)|((bl)<<4)|((br)<<6))

// this is attribute table data, 
// each 2 bits defines a color palette
// for a 16x16 box
const unsigned char Attrib_Table[0x40]={
AE(3,3,1,0),AE(3,3,0,0),AE(3,3,0,0),AE(3,3,0,0), AE(2,2,0,0),AE(2,2,0,0),AE(2,2,0,0),AE(2,2,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,1,1,1),AE(1,1,1,1),AE(1,1,1,1),AE(1,1,1,1), AE(1,1,1,1),AE(1,1,1,1),AE(1,1,1,1),AE(1,1,1,1),
};

/*{pal:"nes",layout:"nes"}*/
const unsigned char Palette_Table[16]={ 
  0x0A,
  0x06,0x36,0x0E,0x00,
  0x2C,0x11,0x12,0x00,
  0x09,0x2C,0x0C,0x00,
  0x06,0x11,0x36
};

const unsigned char Palette_Table2[16]={ 
  0x0A,
  0x06,0x36,0x0E,0x00,
  0x2C,0x11,0x12,0x00,
  0x09,0x2C,0x0C,0x00,
  0x06,0x11,0x36
};

// put 8x8 grid of palette entries into the PPU
void setup_attrib_table() {
  vram_adr(0x23c0);
  vram_write(Attrib_Table, 0x40);
}

void setup_palette() {
  int i;
  // Apenas define as entradas de paleta 0-15 (fundo e objetos)
  for (i = 0; i < 16; i++)
    pal_col(i, Palette_Table[i] ^ attract);
}

void setup_palette2() {
  int i;
  // Apenas define as entradas de paleta 0-15 (fundo e objetos)
  for (i = 0; i < 16; i++)
    pal_col(i, Palette_Table2[i] ^ attract);
}

void play_round() {
  ppu_off();
  setup_attrib_table();
  setup_palette();
  setup_palette2();
  clrscr();
  draw_playfield();
  reset_players();
  while (1) {
    make_move();
    if (gameover) return; // attract mode -> start
    if (players[0].collided || players[1].collided) {
      set_sounds_colision();
      break;
    }
  }
  flash_colliders();
  // add scores to players that didn't collide
  if (players[0].collided) players[1].score++;
  if (players[1].collided) players[0].score++;
  // increase speed
  if (frames_per_move > MAX_SPEED) frames_per_move--;
  // game over?
  if (players[0].score != players[1].score) {
    if (players[0].score >= MAX_SCORE)
      declare_winner(0);
    else if (players[1].score >= MAX_SCORE)
      declare_winner(1);
  }
}

void play_game() {
  gameover = 0;
  init_game();
  if (!attract)
    players[0].human = 1;
  while (!gameover) {
    set_sounds();
     set_sounds_Music();
    play_round();
  }
}

void title_screen(void)
{

  scroll(-8,240);//title is aligned to the color attributes, so shift it a bit to the right

  vram_adr(NAMETABLE_A);
  vram_unrle(title_nam);

  vram_adr(NAMETABLE_C);//clear second nametable, as it is visible in the jumping effect
  vram_fill(0,1024);

  pal_bg(Palette_Table);
  pal_bright(4);
  ppu_on_bg();
  delay(20);//delay just to make it look better

  iy=240<<4;
  dy=-8<<4;
  frame_cnt=0;
  wait=160;
  bright=4;
	
  while(1)
  {
    ppu_wait_frame();

    scroll(-8,iy>>4);

    if(pad_trigger(0)&PAD_START) break;

    iy+=dy;

    if(iy<0)
    {
      iy=0;
      dy=-dy>>1;
    }

    if(dy>(-8<<4)) dy-=2;

    if(wait)
    {
      --wait;
    }
  }
}

void select_difficulty(void){
  int difficulty=0;
  
  if(difficulty==0){
    START_SPEED =12;
    MAX_SPEED =5;
    MAX_SCORE =3;
    MAX_BLOCK =0;
  }
  if(difficulty==1){
      START_SPEED =10;
      MAX_SPEED =4;
      MAX_SCORE =7;
      MAX_BLOCK =10;
  }
  if(difficulty==2){
    START_SPEED =8;
    MAX_SPEED =3;
    MAX_SCORE =10;
    MAX_BLOCK =20;
  }
    if(difficulty==3){
    START_SPEED =5;
    MAX_SPEED =2;
    MAX_SCORE =20;
    MAX_BLOCK =40;
  }
}


void main() {
  apu_init();
  set_sounds();
  //set_sounds_Music();
  // set music callback function for NMI
  joy_install (joy_static_stddrv);
  title_screen();
  select_difficulty();
  //vrambuf_clear();
  set_vram_update(updbuf);
  scroll(0,0);	
  while (1) {
      
	//set_sounds_Music();
    attract = 1;
    play_game();
    attract = 0;
    play_game();
  }
}