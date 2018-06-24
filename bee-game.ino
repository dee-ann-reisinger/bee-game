#include <Arduboy2.h>
#include <Sprites.h>
#include <ArduboyTones.h>
#include "images.h"

//#define VOLUME TONE_HIGH_VOLUME
#define VOLUME 0

// Arduino boilerplate
Arduboy2 arduboy;
Sprites sprites;
ArduboyTones sound(arduboy.audio.enabled);

// cursor stuff
Point curs = {63, 31}; // cursor's coordinates
Point target; // where the bees are directed to go
Rect* grabbed; // the currently grabbed wall, if any
bool is_grabbing = false;

const int coff = 2; // cursor draw offset
const int poff = 7; // ping draw offset
int pframe = -1; // frame in ping animation
int px, py; // ping coordinates

// Level stuff
#define N_WALLS 4
Rect walls[N_WALLS];

#define N_BEES 2
Point bees[N_BEES];
bool is_tgt[N_BEES]; // tracks whether each bee has a target to chase

// moveable walls
#define N_MWALLS 3
Rect mwalls[N_MWALLS];

void setup() {

  arduboy.begin();
  arduboy.initRandomSeed();
  arduboy.setFrameRate(30);

  walls[0] = {10,10,40,10};
  walls[1] = {10,44,40,10};
  walls[2] = {50, 0,10,20};
  walls[3] = {50,44,10,20};

  mwalls[0] = {60, 15, 10, 34};
  mwalls[1] = {70, 15, 10, 34};
  mwalls[2] = {80, 15, 8, 34};

  bees[0] = {30, 5};
  bees[1] = {30, 58};
  for(int i = 0; i < N_BEES; i++) {
    is_tgt[i] = false;
  }
}

void loop() {
  if(!arduboy.nextFrame()) return;
  arduboy.clear();
  arduboy.pollButtons();

  // Move cursor
  if(arduboy.pressed(UP_BUTTON) && curs.y > 0) {
    curs.y -= 1;
    if(is_grabbing) { // also move mwall if grabbed
      grabbed->y -= 1;
    }
  }
  if(arduboy.pressed(DOWN_BUTTON) && curs.y < 63) {
    curs.y += 1;
    if(is_grabbing) { // also move mwall if grabbed
      grabbed->y += 1;
    }
  }
  if(arduboy.pressed(LEFT_BUTTON) && curs.x > 0) {
    curs.x -= 1;
  }
  if(arduboy.pressed(RIGHT_BUTTON) && curs.x < 127) {
    curs.x += 1;
  }

  // calling bees
  if(arduboy.justPressed(A_BUTTON)) {
    sound.tone(350 + VOLUME, 105, 345 + VOLUME, 30);
    for(int i = 0; i < N_BEES; i++) {
      is_tgt[i] = true;
    }
    target = curs;
    // create ping
    px = curs.x;
    py = curs.y;
    pframe = 0;
  }

  // stop grabbing walls when you're not pressing the B button
  if(!arduboy.pressed(B_BUTTON)) {
    is_grabbing = false;
  }

  // grab wall on pressing B
  if(arduboy.justPressed(B_BUTTON)) {
    for (int i = 0; i < N_MWALLS; i++) {
      if(arduboy.collide(curs, mwalls[i])) {
        is_grabbing = true;
        grabbed = &mwalls[i];
        break;
      }
    }
  }

  // move bees
  for(int i = 0; i < N_BEES; i++) {
    if(is_tgt[i]) { // if the bee is pursuing a target, move towards the target
      int x_offset = bias_draw(target.x - bees[i].x);
      if(!collides({bees[i].x + x_offset, bees[i].y}, true)) {
        bees[i].x += x_offset;
      }
      int y_offset = bias_draw(target.y - bees[i].y);
      if(!collides({bees[i].x, bees[i].y + y_offset}, true)) {
        bees[i].y += y_offset;
      }
    } else {
      int x_offset = bias_draw(0);
      if(!collides({bees[i].x + x_offset, bees[i].y}, true)) {
        bees[i].x += x_offset;
      }
      int y_offset = bias_draw(0);
      if(!collides({bees[i].x, bees[i].y + y_offset}, true)) {
        bees[i].y += y_offset;
      }
    }
  
    // stop chasing when you've the target
    if (bees[i].x == target.x && bees[i].y == target.y) {
      is_tgt[i] = false;
    }
  
    // keep bees on screen
    bees[i].x = mid(0, bees[i].x, 127);
    bees[i].y = mid(0, bees[i].y, 63);
  
    arduboy.drawPixel(bees[i].x, bees[i].y); // draw the bee
  }

  // Drawing code
  for(int i = 0; i < N_WALLS; i++) {
    draw_wall(walls[i]);
  }
  for(int i = 0; i < N_MWALLS; i++) {
    draw_mwall(mwalls[i]);
  }
  if(collides(curs, false)) {
    sprites.drawErase(curs.x - coff, curs.y - coff, cursor_sprite, 0);
  } else { 
    sprites.drawSelfMasked(curs.x - coff, curs.y - coff, cursor_sprite, 0);
  }
  if (pframe >= 0) {
    sprites.drawPlusMask(px - poff, py - poff, ping_plus_mask, pframe);
  }

 
  // advance the ping animation
  if (pframe >= 0) {
    pframe++;
    if (pframe > 2) {
      pframe = -1;
    }
  }
  arduboy.display();
}

int mid(int low, int x, int high) {
  if (x < low) {
    return low;
  } else if (x > high) {
    return high;
  }
  return x;
}

int bias_draw(int dir) {
  int should_follow = random(100) < 69;
  if (dir > 0 && should_follow) {
    return 1;
  } else if (dir < 0 && should_follow) {
    return -1;
  }
  return random(3) - 1;
}

void draw_wall(Rect w) {
  arduboy.fillRect(w.x, w.y, w.width, w.height);
}

void draw_mwall(Rect w) {
  arduboy.drawRect(w.x, w.y, w.width, w.height);
  arduboy.drawRect(w.x + 2, w.y + 2, w.width - 4, w.height - 4);
}

bool collides(Point p, bool count_mwalls) {
  for(int i = 0; i < N_WALLS; i++) {
    if(arduboy.collide(p, walls[i])) {
      return true;
    }
  }
  if (!count_mwalls) {
    return false;
  }
  for(int i = 0; i < N_MWALLS; i++) {
    if(arduboy.collide(p, mwalls[i])) {
      return true;
    }
  }
  return false;
}

