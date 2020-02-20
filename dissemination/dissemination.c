#include <kilombo.h>
#include "dissemination.h"
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
  case STOP:
    mydata->direction = STOP;
    smooth_set_motors(0,0);
    break;
  case FORWARD:
    mydata->direction = FORWARD;
    smooth_set_motors(kilo_straight_left, kilo_straight_right);
    break;
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
  default:
    break;
  }
}

void loop() {
  // Update distance estimate with every message
  if (mydata->new_message) {
    mydata->new_message = 0; // On remet le flag à O

    /* Gestion des collisions entre les kilobots */
    mydata->neighbor_distance = estimate_distance(&mydata->dist);
    if (mydata->neighbor_distance < TOOCLOSE_DISTANCE) {
      mydata->collision_state = NEIGHBOR_TOOCLOSE;
    }
    else {
      mydata->collision_state = ALONE;
    }
    
    /* GESTION DES OPINIONS DE SES VOISINS */
    if (mydata->flag_a_listener)
      mydata->opinion_a ++;
    else
      mydata->opinion_b ++;
  }
  
  int temp;
  switch(mydata->collision_state) {
    case ALONE:
      temp = rand() % 4;
      //printf("%d",temp);
      set_motion(temp);
      break;
    case NEIGHBOR_TOOCLOSE:
      set_motion(FORWARD);
      break;
    default:
      break;
  }

  /* CHOSES A AJOUTER :
  - IDENTIFIANT UNIQUE POUR CHACUN DES KILOBOTS AFIN DE D'EVITER LES AVIS EN DOUBLE PROVENANT D'UN MEME VOISIN
  - APPLIQUE LE MAJORITY RULE DANS LES 3 DERNIERES SECONDES SEULEMENT
  */

  /* MAJORITY RULE */
  if (mydata->opinion_a > mydata->opinion_b) {
    set_color(RGB(3,0,0));
    mydata->opinion_a = 1;
    mydata->opinion_b = 0;
    mydata->flag_a_speaker = 1;
  }
  else if (mydata->opinion_a < mydata->opinion_b) {
    set_color(RGB(0,0,3));
    mydata->opinion_a = 0;
    mydata->opinion_b = 1;
    mydata->flag_a_speaker = 0;
  }
  else { // mydata->opinion_a == mydata->opinion_b --- en cas d'égalité, on garde son opinion
    if(mydata->flag_a_speaker) {
      set_color(RGB(3,0,0));
      mydata->opinion_a = 1;
      mydata->opinion_b = 0;
      mydata->flag_a_speaker = 1;
    }
    else {
      set_color(RGB(0,0,3));
      mydata->opinion_a = 0;
      mydata->opinion_b = 1;
      mydata->flag_a_speaker = 0;
    }
  }
  
  /*
  const uint8_t twobit_mask = 0b00000011;
  uint8_t rand_direction = rand_soft()&twobit_mask;
  //printf("%d",rand_direction);
  if (rand_direction == 0 || rand_direction == 1)
    set_motion(FORWARD);
  else if (rand_direction == 2)
    set_motion(LEFT);
  else if (rand_direction == 3)
    set_motion(RIGHT);
  delay(1000);
  */
}

void stallCollision() {
  if (kilo_uid < mydata->msgID) {
    set_motion(FORWARD);
  } else {
    set_motion(STOP);
  }
}

/* SPEAKER */

void setup_message(void)
{
  /*
  mydata->transmit_msg.type = NORMAL;
  mydata->transmit_msg.crc = message_crc(&mydata->transmit_msg);
  */

  mydata->message_a.type = NORMAL;
  mydata->message_a.data[0] = 1;
  mydata->message_a.crc = message_crc(&mydata->message_a);

  mydata->message_b.type = NORMAL;
  mydata->message_b.data[0] = 0;
  mydata->message_b.crc = message_crc(&mydata->message_b);
}

/*
message_t *message_tx() {
  return &mydata->transmit_msg;
}
*/

message_t *message_tx() { // speaker
  if (mydata->flag_a_speaker)
      return &mydata->message_a;
  else
      return &mydata->message_b;
}

/*
void message_tx_succes() { // utile si l'on veut un accusé de réception pour le speaker
    message_sent = 1;
}
*/

/* LISTENER */
void message_rx(message_t *msg, distance_measurement_t *dist) {
    //mydata->rcvd_message = *msg; // store the incoming message
    mydata->new_message = 1; // set the flag to 1 to indicate that a new message arrived (en gros c'est un "accusé de réceptioné" mais pour le listener)
    mydata->dist = *dist;
    mydata->flag_a_listener = msg->data[0];
}

void setup() // initialisation au tout début, une seule fois
{
  mydata->collision_state = ALONE;
  mydata->neighbor_distance = 0;
  mydata->direction = FORWARD;
  mydata->new_message = 0;

  setup_message();

  /* MAJORITY RULE */

  if (rand() % 2 == 0) {
    /* OPINION A*/
    set_color(RGB(3,0,0));
    mydata->opinion_a = 1;
    mydata->opinion_b = 0;
    mydata->flag_a_speaker = 1;
  }
  else {
    /* OPINION B */
    set_color(RGB(0,0,3));
    mydata->opinion_a = 0;
    mydata->opinion_b = 1;
    mydata->flag_a_speaker = 0;
  }
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
  //p += sprintf (p, "Neighbor Distance: %d \n", mydata->neighbor_distance);
  //p += sprintf (p, "Message reçu: %d \n", mydata->rcvd_message);
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