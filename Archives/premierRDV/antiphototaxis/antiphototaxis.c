#include <kilombo.h>
#include "antiphototaxis.h"
#include <math.h>

// declare constants
static const uint8_t STEP_TIME = 10;
static const uint32_t TICKS_TO_SUCCESS = 300;

REGISTER_USERDATA(USERDATA)

/* Helper function for setting motor speed smoothly
 */
void smooth_set_motors(uint8_t ccw, uint8_t cw)
{
  // OCR2A = ccw;  OCR2B = cw;  
#ifdef KILOBOT 
  uint8_t l = 0, r = 0;
  if (ccw && !OCR2A) // we want left motor on, and it's off
    l = 0xff;
  if (cw && !OCR2B)  // we want right motor on, and it's off
    r = 0xff;
  if (l || r)        // at least one motor needs spin-up
    {
      set_motors(l, r);
      delay(15);
    }
#endif
  // spin-up is done, now we set the real value
  set_motors(ccw, cw);
}

void set_motion(motion_t new_motion)
{
  switch(new_motion) {
  case STOP:
    smooth_set_motors(0,0);
    break;
  case FORWARD:
    smooth_set_motors(kilo_straight_left, kilo_straight_right);
    break;
  case LEFT:
    smooth_set_motors(kilo_turn_left, 0); 
    break;
  case RIGHT:
    smooth_set_motors(0, kilo_turn_right); 
    break;
  }
}

void set_state() {
  mydata->intensity = get_ambientlight();
  if (kilo_ticks >= mydata->stepTicks + STEP_TIME) {
    if (mydata->intensity > mydata->lastIntensity) {
      mydata->lastIntensity = mydata->intensity;
      mydata->lastTicks = kilo_ticks;
      mydata->search_state = BETTER;
    } else if (mydata->intensity < mydata->lastIntensity) {
      mydata->lastIntensity = mydata->intensity;
      mydata->lastTicks = kilo_ticks;
      mydata->search_state = WORSE;
    } else {
      if (kilo_ticks >= mydata->lastTicks + TICKS_TO_SUCCESS) {
        mydata->search_state = SUCCESS;
      } else {
        mydata->search_state = NONE;
      }
    }
    mydata->stepTicks = kilo_ticks;
  }
}


void setup() { 
  mydata->intensity = 1023;
  mydata->lastIntensity = 0;
  mydata->search_state = NONE;
  mydata->lastTicks = 0;
  mydata->stepTicks = 0;
  set_color(RGB(0,3,0)); // color of the stationary bot
}

void loop() {
  set_state();
  switch(mydata->search_state) {
    case SUCCESS:
      set_motion(STOP);
      break;
    case NONE:
      set_motion(LEFT);
      break;
    case BETTER:
      set_motion(FORWARD);
      break;
    case WORSE:
      set_motion(LEFT);
  }     
}

#ifdef SIMULATOR
/* provide a text string for the simulator status bar about this bot */
static char botinfo_buffer[10000];
char *cb_botinfo(void)
{
  char *p = botinfo_buffer;
  p += sprintf (p, "Light intensity: %d \n", mydata->intensity);
  p += sprintf (p, "Search state %d \n", mydata->search_state);
  return botinfo_buffer;
}

/*
 * Uses euclidean distance to setup a lighting callback function
 * Lower values returned values correspond to higher light
 * Light readings degrade linearly
 */
int16_t cb_lighting(double x, double y){
  double light_x = -400;
  double light_y = 0;
  double dist_x = pow(light_x + x, 2);
  double dist_y = pow(light_y + y, 2);
  double dist_c = sqrt(dist_x + dist_y);
  return (int16_t)dist_c;
}

int16_t boundaries(double x, double y, double * dx, double * dy) {
  double d = x;
  double d2 = y;
  if (d < 475.0 && d2 < 475.0) {
    if (d > -475.0 && d2 > -475.0) {
      return 0; // pas d'obstacle
    }
    else { // attention au signe !
      *dx = x/d;
      *dy = y/d2;
      return 1; // obstacle à gauche et/ou en haut
    }
  }
  else {
    *dx = -x/d;
    *dy = -y/d2;
    return 1; // obstacle à droite et/ou en bas
  }  
}
#endif

int main() {
  kilo_init();

  SET_CALLBACK(botinfo, cb_botinfo);
  SET_CALLBACK(lighting, cb_lighting);
  SET_CALLBACK(obstacles, boundaries);

  kilo_start(setup, loop);
  return 0;
}