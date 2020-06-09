#include <kilombo.h>
#include "randomwalk.h"
#include <math.h>

REGISTER_USERDATA(USERDATA)

// declare constants
static const uint8_t TOOCLOSE_DISTANCE = 40; // 40 mm

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
  /*case STOP:
    mydata->direction = STOP;
    smooth_set_motors(0,0);
    break;*/
  /*case FORWARD:
    mydata->direction = FORWARD;
    smooth_set_motors(kilo_straight_left, kilo_straight_right);
    break;*/
  case LEFT:
    mydata->direction = LEFT;
    smooth_set_motors(kilo_turn_left, 0);
    //printf("left");
    break;
  case RIGHT:
    mydata->direction = RIGHT;
    smooth_set_motors(0, kilo_turn_right); 
    //printf("right");
    break;
  }
}

void loop() {
  // Update distance estimate with every message
  if (mydata->new_message) {
    mydata->new_message = 0;
    mydata->neighbor_distance = estimate_distance(&mydata->dist);
    if (mydata->neighbor_distance < TOOCLOSE_DISTANCE) {
      mydata->collision_state = NEIGHBOR_TOOCLOSE;
    }
    else {
      mydata->collision_state = ALONE;
    }
  }
  switch(mydata->collision_state) {
    case ALONE:
      set_motion(rand() % 4);
      break;
    case NEIGHBOR_TOOCLOSE:
      set_motion(FORWARD);
      break;
  }
}

void stallCollision() {
  if (kilo_uid < mydata->msgID) {
    set_motion(FORWARD);
  } else {
    set_motion(STOP);
  }
} 

void message_rx(message_t *m, distance_measurement_t *d) {
    mydata->new_message = 1;
    mydata->dist = *d;
}

void setup_message(void)
{
  mydata->transmit_msg.type = NORMAL;
  mydata->transmit_msg.data[0] = kilo_uid & 0xff; //low byte of ID, currently not really used for anything
  
  //finally, calculate a message check sum
  mydata->transmit_msg.crc = message_crc(&mydata->transmit_msg);
}

message_t *message_tx() 
{
  return &mydata->transmit_msg;
}

void setup()
{
  mydata->collision_state = ALONE;
  mydata->neighbor_distance = 0;
  mydata->direction = FORWARD;
  mydata->new_message = 0;

  setup_message();

  if (kilo_uid == 0)
    set_color(RGB(0,0,0)); // color of the stationary bot
  else
    set_color(RGB(3,0,0)); // color of the moving bot
}

#ifdef SIMULATOR
/* provide a text string for the simulator status bar about this bot */
static char botinfo_buffer[10000];
char *cb_botinfo(void)
{
  char *p = botinfo_buffer;
  p += sprintf (p, "ID: %d \n", kilo_uid);  
  /*if (mydata->collision_state == NEIGHBOR_TOOCLOSE)
    p += sprintf (p, "Collision State: NEIGHBOR_TOOCLOSE \n");
  if (mydata->collision_state == ALONE)
    p += sprintf (p, "Collision State: ALONE \n");*/
  p += sprintf (p, "Neighbor Distance: %d \n", mydata->neighbor_distance);
  p += sprintf (p, "Direction: %d \n", mydata->direction);
  return botinfo_buffer;
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
  kilo_message_rx = message_rx;
  SET_CALLBACK(botinfo, cb_botinfo);
  SET_CALLBACK(obstacles, boundaries);
  kilo_message_tx = message_tx;
  kilo_start(setup, loop);
  return 0;
}