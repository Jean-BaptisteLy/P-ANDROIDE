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
    break;
  case RIGHT:
    mydata->direction = RIGHT;
    smooth_set_motors(0, kilo_turn_right); 
    break;
  default:
    break;
  }
}

int recherche (uint8_t tableau[], uint8_t tailleTab, uint8_t nombre) {
  uint8_t i;
  for (i = 0; i < tailleTab; ++i) {
      if (tableau[i]==nombre)
          return 1;
  }          
  return 0;
}

void majority_tule() { /* GESTION DES OPINIONS DE SES VOISINS */
  uint32_t nb_ticks;
  nb_ticks = mydata->quality_of_site * mydata->dissemination_time;
  if (kilo_ticks > (mydata->last_time_update + nb_ticks - 93)) { // during the last three seconds
    if (kilo_ticks > (mydata->last_time_update + nb_ticks)) {
      mydata->last_time_update = kilo_ticks;  
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
      mydata->state = EXPLORATION;
      // VIDER LE TAB_UID
      uint8_t i;
      for(i=0;i<100;i++) {
        mydata->tab_uid[i] = 232;
      }
      mydata->cpt_voisins = 0;
    }
  }
}

void random_walk() {
  uint8_t temp;
  temp = rand() % 4;
  set_motion(temp);
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

void explore() {
  //random_walk();
  if (kilo_ticks > (mydata->last_time_update + mydata->exploration_time)) {
    mydata->state = DISSEMINATION;
  }
}

void collisions() { /* Gestion des collisions entre les kilobots */
  mydata->neighbor_distance = estimate_distance(&mydata->dist);
  if (mydata->neighbor_distance < TOOCLOSE_DISTANCE) {
    set_motion(FORWARD);
  }
  else {
    random_walk();
  }
}

void maj() {

}

void loop() {

  if(mydata->state == DISSEMINATION) {
    if (mydata->new_message) {
      mydata->new_message = 0; // On remet le flag à O
      collisions();      
      if (mydata->flag_a_listener) {
        if(!mydata->flag_voisin_deja_rencontre) {
          mydata->opinion_a ++;
        }            
      }
      else {
        if(!mydata->flag_voisin_deja_rencontre) {
          mydata->opinion_b ++;
        }  
      }  
    }
    else { // si aucun message reçu en cas de DISSEMINATION
      random_walk();
    }
    majority_tule();
  }
  else if(mydata->state == EXPLORATION) {
    if (mydata->new_message) {
      mydata->new_message = 0; // On remet le flag à O
      collisions();
    }
    else {
      random_walk();
    }
    explore();
  }    
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
  mydata->message_a.data[1] = kilo_uid;
  mydata->message_a.crc = message_crc(&mydata->message_a);

  mydata->message_b.type = NORMAL;
  mydata->message_b.data[0] = 0;
  mydata->message_b.data[1] = kilo_uid;
  mydata->message_b.crc = message_crc(&mydata->message_b);
}

/*
message_t *message_tx() {
  return &mydata->transmit_msg;
}
*/

message_t *message_tx() { // speaker pour envoyer son opinion
  if (mydata->flag_a_speaker)
      return &mydata->message_a;
  else
      return &mydata->message_b;
}

/*
message_t *message_tx_uid() { // speaker pour envoyer son uid unique
  return &mydata->uid;
}
*/

/*
void message_tx_succes() { // utile si l'on veut un accusé de réception pour le speaker
    message_sent = 1;
}
*/

/* LISTENER */
void message_rx(message_t *msg, distance_measurement_t *dist) {
  //mydata->rcvd_message = *msg; // store the incoming message
  mydata->new_message = 1; // set the flag to 1 to indicate that a new message arrived (en gros c'est un "accusé de réception" mais pour le listener)
  mydata->dist = *dist;
  mydata->flag_a_listener = msg->data[0];
  // à remplir le tableau des uids ici
  if(mydata->state == DISSEMINATION) {
    if(recherche(mydata->tab_uid,100,msg->data[1])) {
      mydata->flag_voisin_deja_rencontre = 1;
    }
    else {
      mydata->flag_voisin_deja_rencontre = 0;
      mydata->tab_uid[mydata->cpt_voisins] = msg->data[1];
      mydata->cpt_voisins ++;
    }
  }
}

void setup() // initialisation au tout début, une seule fois
{
  //mydata->collision_state = ALONE;
  mydata->neighbor_distance = 0;
  mydata->direction = FORWARD;
  mydata->new_message = 0;

  mydata->state = DISSEMINATION;
  mydata->uid = kilo_uid;

  mydata->cpt_voisins = 0;
  mydata->flag_voisin_deja_rencontre = 0;

  mydata->last_time_update = kilo_ticks;

  mydata->quality_of_site = 1;
  // conversion en nombre de ticks
  mydata->dissemination_time = 15624; // 8,4 minutes
  mydata->exploration_time = 11294; // 6,072 minutes

  mydata->dissemination_time = 310; // 10 secondes
  mydata->exploration_time = 124; // 4 secondes

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
  //p += sprintf (p, "ID: %d \n", kilo_uid);  
  if (mydata->state == DISSEMINATION)
    p += sprintf (p, "State: DISSEMINATION \n");
  if (mydata->state == EXPLORATION)
    p += sprintf (p, "State: EXPLORATION \n");
  //p += sprintf (p, "Neighbor Distance: %d \n", mydata->neighbor_distance);
  //p += sprintf (p, "Message reçu: %d \n", mydata->rcvd_message);
  //p += sprintf (p, "Direction: %d \n", mydata->direction);
  p += sprintf (p, "Opinion a: %d \n", mydata->opinion_a);
  p += sprintf (p, "Opinion b: %d \n", mydata->opinion_b);

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

int16_t circle_barrier(double x, double y, double * dx, double * dy) {
  double d = sqrt(x*x + y*y);
  if (d < 500.0)
    return 0;
  *dx = -x/d;
  *dy = -y/d;
  return 1;
}
#endif

int main() {
  kilo_init();
  kilo_message_rx = message_rx;
  SET_CALLBACK(botinfo, cb_botinfo);
  //SET_CALLBACK(obstacles, boundaries);
  SET_CALLBACK(obstacles, circle_barrier);
  //kilo_message_tx = message_tx_uid;
  kilo_message_tx = message_tx;
  //kilo_message_tx = message_tx_uid;
  kilo_start(setup, loop);
  return 0;
}