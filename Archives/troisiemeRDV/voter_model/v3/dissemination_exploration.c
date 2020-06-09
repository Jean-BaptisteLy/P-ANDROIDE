#include <kilombo.h>
#include "dissemination_exploration.h"
#include <math.h>

REGISTER_USERDATA(USERDATA)

// declare constants
static const uint8_t TOOCLOSE_DISTANCE = 40; // 40 mm
static const uint8_t STEP_TIME = 10;
static const uint32_t TICKS_TO_SUCCESS = 300;

//static const uint8_t N_OPTIONS = 3;
static const uint32_t seuil_voter_model = 3;
static const uint8_t quality_of_site_a = 1;
static const uint8_t quality_of_site_b = 5;
static const uint8_t nbre_initial_agents_tetus = 6;
static const uint8_t seuil_changements_opinions = 8;
static const uint32_t dissemination_time = 15624;
static const uint32_t exploration_time = 11294;
/*
dissemination_time = 15624; // 8,4 minutes
exploration_time = 11294; // 6,072 minutes
dissemination_time = 310; // 10 secondes
exploration_time = 124; // 4 secondes
dissemination_time = 310;
exploration_time = 310;
*/

// variables globales
uint8_t consensus_site_a = nbre_initial_agents_tetus / 2;
uint8_t consensus_site_b = nbre_initial_agents_tetus / 2;
uint8_t consensus_courant = nbre_initial_agents_tetus;
uint8_t consensus_atteint = 100; // nombre de kilobots

/* Helper function for setting motor speed smoothly
 */
void smooth_set_motors(uint8_t ccw, uint8_t cw) {
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

void set_motion(motion_t new_motion) {
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

void case_light() {
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

void set_behavior() {
  mydata->intensity = get_ambientlight();
  if (kilo_ticks >= mydata->stepTicks + STEP_TIME) {
    if (mydata->behavior_state == PHOTOTAXIS) {
      if (mydata->intensity < mydata->lastIntensity) {
        mydata->lastIntensity = mydata->intensity;
        mydata->lastTicks = kilo_ticks;
        mydata->search_state = BETTER;
      } else if (mydata->intensity > mydata->lastIntensity) {
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
    }
    else printf("ERREUR IMPOSSIBLE");
    case_light();
    mydata->stepTicks = kilo_ticks;
  }  
}

void explore() { 
  if (kilo_ticks > (mydata->last_time_update + mydata->exploration_time)) {
    mydata->state = DISSEMINATION;
    mydata->last_time_update = kilo_ticks; // YES ATTENTION TRES IMPORTANT
    if(!mydata->tetu) {
      if(mydata->flag_speaker == 1) set_color(RGB(3,0,0));
      else if(mydata->flag_speaker == 2) set_color(RGB(0,0,3));
    }
  }
  if (mydata->state == EXPLORATION) {
    set_behavior();
  }
}

uint8_t recherche (uint8_t tableau[], uint8_t tailleTab, uint8_t nombre) {
  uint8_t i;
  for (i = 0; i < tailleTab; ++i) {
    if (tableau[i]==nombre)
      return 1;
  }          
  return 0;
}

void affichage_resultats() {
  consensus_courant ++;
  if (consensus_courant == consensus_atteint) {
    printf("######################### CONSENSUS ATTEINT ######################### \n");
    printf("Nombre d'agents pour le site a : %d \n", consensus_site_a);
    printf("Nombre d'agents pour le site b : %d \n", consensus_site_b);
    uint32_t min = 0;
    printf("Kiloticks du consensus : %d \n", kilo_ticks);
    min = kilo_ticks / 31 / 60;
    printf("Temps en min du consensus : %d \n", min);
    exit(0);
  }
}

void transition_tetu() {
  if (mydata->changements_opinions == mydata->seuil_changements_opinions) {
    if (mydata->changements_opinions_a == mydata->changements_opinions_b) {
      uint8_t temp = rand() % 2;
      if(temp == 0) mydata->changements_opinions_a ++;
      else if(temp == 1) mydata->changements_opinions_b ++;
      else printf("ERREUR IMPOSSIBLE TRANSITION TETU 1 \n");
    }
    if (mydata->changements_opinions_a > mydata->changements_opinions_b) {
      mydata->tetu = 1;
      mydata->opinion_a = 1;
      mydata->opinion_b = 0;
      mydata->flag_speaker = 1;
      mydata->quality_of_site = quality_of_site_a;
      set_color(RGB(3,1,1));
      //mydata->behavior_state = PHOTOTAXIS;
      consensus_site_a ++;
      affichage_resultats();
    }
    else if (mydata->changements_opinions_b > mydata->changements_opinions_a) {
      mydata->tetu = 1;
      mydata->opinion_a = 0;
      mydata->opinion_b = 1;
      mydata->flag_speaker = 2;
      mydata->quality_of_site = quality_of_site_b;
      set_color(RGB(1,1,3));
      //mydata->behavior_state = ANTI_PHOTOTAXIS;
      consensus_site_b ++;
      affichage_resultats();
    }
    else printf("ERREUR IMPOSSIBLE TRANSITION TETU 2 \n");
  }
}

void set_opinion_a() {
  if (!mydata->tetu) {
    mydata->opinion_a = 1;
    mydata->opinion_b = 0;
    mydata->flag_speaker = 1;
    //mydata->behavior_state = PHOTOTAXIS;
    mydata->quality_of_site = quality_of_site_a;
    set_color(RGB(3,3,0)); // jaune
    mydata->changements_opinions_a ++;
    mydata->changements_opinions ++;
    transition_tetu();
  }
}

void set_opinion_b() {
  if (!mydata->tetu) {
    mydata->opinion_a = 0;
    mydata->opinion_b = 1;
    mydata->flag_speaker = 2;
    //mydata->behavior_state = ANTI_PHOTOTAXIS;
    mydata->quality_of_site = quality_of_site_b;
    set_color(RGB(0,3,3)); // turquois
    mydata->changements_opinions_b ++;
    mydata->changements_opinions ++;
    transition_tetu();
  }
}

void vider_tableau_uid() {
  uint8_t i;
  for(i=0;i<100;i++) {
    mydata->tab_uid[i] = 232;
  }
  mydata->cpt_voisins = 0;
  mydata->state = EXPLORATION;
  //mydata->flag_speaker = 0;
  explore();
}


double frand_a_b(double a, double b) {
  return ( rand()/(double)RAND_MAX ) * (b-a) + a;
}


void voter_model() {
  uint32_t nb_ticks;
  nb_ticks = mydata->quality_of_site * mydata->dissemination_time / 2;
  if (kilo_ticks > (mydata->last_time_update + nb_ticks - 93)) { // during the last three seconds
    if (kilo_ticks > (mydata->last_time_update + nb_ticks)) {
      mydata->last_time_update = kilo_ticks;
      if (!mydata->tetu) {
        double total;
        double proba_a;
        double proba_b;
        double tirage;
        total = mydata->opinion_a + mydata->opinion_b;
        proba_a = mydata->opinion_a / total;
        proba_b = mydata->opinion_b / total;
        tirage = frand_a_b(0.0,1.0);
        if (tirage <= proba_a) set_opinion_a();
        else if (tirage <= proba_a + proba_b) set_opinion_b();
        else printf("ERREUR IMPOSSIBLE VOTER MODEL 1 \n");
        vider_tableau_uid();
      }
      else {// agent têtu
        mydata->state = EXPLORATION;
        explore();
        if (mydata->flag_speaker == 1) mydata->quality_of_site = quality_of_site_a;
        else if (mydata->flag_speaker == 2) mydata->quality_of_site = quality_of_site_b;
        else printf("ERREUR IMPOSSIBLE VOTER MODEL 2 \n");
      }
    }
  }
}

void collisions() { /* Gestion des collisions entre les kilobots */
  mydata->neighbor_distance = estimate_distance(&mydata->dist);
  if (mydata->neighbor_distance < TOOCLOSE_DISTANCE) {
    set_motion(FORWARD);
  }
  else {
    mydata->behavior_state = PHOTOTAXIS;
    set_behavior();
  }
}

void nest() {
	
    if (mydata->intensity <= 510) {
      mydata->flag_nest = 1;
    }
    else mydata->flag_nest = 0;
    
    //mydata->flag_nest = 1;
    //mydata->stepTicks = kilo_ticks;
}

void loop() {
  if(mydata->state == DISSEMINATION) {
    nest();
    if (mydata->new_message) {
      mydata->new_message = 0; // On remet le flag à O
      collisions();
      //nest();
      if(mydata->flag_nest && mydata->tetu == 0) {     
        if (mydata->flag_listener == 1) {
          if(!mydata->flag_voisin_deja_rencontre) {
            mydata->opinion_a ++;
            mydata->cpt_voisins ++;
          }            
        }
        else if (mydata->flag_listener == 2) {
          if(!mydata->flag_voisin_deja_rencontre) {
            mydata->opinion_b ++;
            mydata->cpt_voisins ++;
          }  
        }
        //else // rencontre un voisin en exploration, donc ne compte pas
      }
      if(mydata->cpt_voisins >= mydata->seuil_voter_model) {
        double total;
        double proba_a;
        double proba_b;
        double tirage;
        total = mydata->opinion_a + mydata->opinion_b;
        proba_a = mydata->opinion_a / total;
        proba_b = mydata->opinion_b / total;
        tirage = frand_a_b(0.0,1.0);
        if (tirage <= proba_a) set_opinion_a();
        else if (tirage <= proba_a + proba_b) set_opinion_b();
        else printf("ERREUR IMPOSSIBLE LOOP \n");
        vider_tableau_uid();
      }  
    }
    else { // si aucun message reçu en cas de DISSEMINATION
      set_behavior();
    }
    voter_model();
  }
  else if(mydata->state == EXPLORATION) {
    if (mydata->new_message) {
      mydata->new_message = 0; // On remet le flag à O
      collisions();
    }
    else {
      set_behavior();
    }
    explore();
  }
  else printf("ERREUR IMPOSSIBLE \n");
}

/* SPEAKER */

void setup_message(void) {
  /*
  mydata->transmit_msg.type = NORMAL;
  mydata->transmit_msg.crc = message_crc(&mydata->transmit_msg);
  */

  //if(mydata->state == DISSEMINATION) {
  	mydata->message_a.type = NORMAL;
  	mydata->message_a.data[0] = 1;
  	mydata->message_a.data[1] = kilo_uid;
  	mydata->message_a.crc = message_crc(&mydata->message_a);

  	mydata->message_b.type = NORMAL;
  	mydata->message_b.data[0] = 2;
  	mydata->message_b.data[1] = kilo_uid;
  	mydata->message_b.crc = message_crc(&mydata->message_b);

  	mydata->message_exploration.type = NORMAL;
  	mydata->message_exploration.data[0] = 0;
  	mydata->message_exploration.data[1] = kilo_uid;
  	mydata->message_exploration.crc = message_crc(&mydata->message_exploration);
  //}
  //else if(mydata->state == EXPLORATION) {
  	
  //}
  //else printf("ERREUR IMPOSSIBLE \n");
}

/*
message_t *message_tx() {
  return &mydata->transmit_msg;
}
*/

/* SPEAKER */
message_t *message_tx() { // speaker pour envoyer son opinion
  if (mydata->state == DISSEMINATION) {
  	if (mydata->flag_speaker == 1) return &mydata->message_a;
  	else if (mydata->flag_speaker == 2) return &mydata->message_b;
  }
  else if(mydata->state == EXPLORATION) {
  	return &mydata->message_exploration;
  }
  else printf("ERREUR IMPOSSIBLE \n");
  return &mydata->message_exploration;
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
  mydata->flag_listener = msg->data[0];
  // à remplir le tableau des uids ici
  if(mydata->state == DISSEMINATION && mydata->flag_listener != 0) {
    if(recherche(mydata->tab_uid,100,msg->data[1])) {
      mydata->flag_voisin_deja_rencontre = 1;
    }
    else {
      mydata->flag_voisin_deja_rencontre = 0;
      mydata->tab_uid[mydata->cpt_voisins] = msg->data[1];
      //mydata->cpt_voisins ++;
    }
  }
}

void setup_light() { 
  mydata->intensity = 1023;
  mydata->lastIntensity = 0;
  mydata->search_state = NONE;
  mydata->lastTicks = 0;
  mydata->stepTicks = 0;
}

void setup() // initialisation au tout début, une seule fois
{
  mydata->flag_nest = 0;
  mydata->behavior_state = PHOTOTAXIS;

  mydata->neighbor_distance = 0;
  mydata->direction = FORWARD;
  mydata->new_message = 0;

  mydata->state = DISSEMINATION;
  mydata->uid = kilo_uid;

  mydata->cpt_voisins = 0;
  mydata->flag_voisin_deja_rencontre = 0;

  mydata->last_time_update = kilo_ticks;

  mydata->quality_of_site = 2;

  // conversion en nombre de ticks
  mydata->dissemination_time = dissemination_time;
  mydata->exploration_time = exploration_time;

  setup_message();
  setup_light();

  /* VOTER MODEL */

  mydata->seuil_voter_model = seuil_voter_model;

  if (kilo_uid % 2 == 0) {
    /* OPINION A*/
    set_color(RGB(3,0,0));
    mydata->opinion_a = 0;
    mydata->opinion_b = 0;
    mydata->flag_speaker = 1;
  }
  else {
    /* OPINION B */
    set_color(RGB(0,0,3));
    mydata->opinion_a = 0;
    mydata->opinion_b = 0;
    mydata->flag_speaker = 2;
  }
  /* AGENT TÊTU */

  /* seuil du nombre de changements d'avis avant de devenir un agent têtu */
  mydata->seuil_changements_opinions = seuil_changements_opinions;
  mydata->changements_opinions = 0;
  mydata->changements_opinions_a = 0;
  mydata->changements_opinions_b = 0;
  mydata->tetu = 0;

  /* AGENTS TETUS DEPUIS LE DEBUT */
  if (kilo_uid < nbre_initial_agents_tetus) {
    if (kilo_uid % 2 == 0) {
      mydata->tetu = 1;
      mydata->opinion_a = 1;
      mydata->opinion_b = 0;
      mydata->flag_speaker = 1;
      //mydata->quality_of_site = 2;
      set_color(RGB(3,1,1));
    }
    else if (kilo_uid % 2 == 1) {
      mydata->tetu = 1;
      mydata->opinion_a = 0;
      mydata->opinion_b = 1;
      mydata->flag_speaker = 2;
      //mydata->quality_of_site = 2;
      set_color(RGB(1,1,3));
    }
  }
}

#ifdef SIMULATOR

static char botinfo_buffer[10000];
char *cb_botinfo(void)
{
  char *p = botinfo_buffer;
  //p += sprintf (p, "ID: %d \n", kilo_uid);
  //p += sprintf (p, "Light intensity: %d \n", mydata->intensity);
  //p += sprintf (p, "Search state %d \n", mydata->search_state);  
  //p += sprintf (p, "Behavior state %d \n", mydata->behavior_state);

  double total;
  double proba_a;
  double proba_b;
  total = mydata->opinion_a + mydata->opinion_b;
  proba_a = mydata->opinion_a / total;
  proba_b = mydata->opinion_b / total;
  
  if (mydata->state == DISSEMINATION) {
    if (!mydata->tetu) {
      p += sprintf (p, "Opinion a: %f ", proba_a);
    	p += sprintf (p, "Opinion b: %f \n", proba_b);
      p += sprintf (p, "cpt voisins : %d \n", mydata->cpt_voisins);
      p += sprintf (p, "Changements d'opinions au total : %d ", mydata->changements_opinions);
      p += sprintf (p, "Changements d'opinions en a : %d ", mydata->changements_opinions_a);
      p += sprintf (p, "Changements d'opinions en b : %d \n", mydata->changements_opinions_b);
    }
    else {
      p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
      p += sprintf (p, "Changements d'opinions au total : %d \n", mydata->changements_opinions);
      p += sprintf (p, "Changements d'opinions en a : %d ", mydata->changements_opinions_a);
      p += sprintf (p, "Changements d'opinions en b : %d \n", mydata->changements_opinions_b);
    }
  }
  else if (mydata->state == EXPLORATION) {
    p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
    p += sprintf (p, "Changements d'opinions au total : %d \n", mydata->changements_opinions);
    p += sprintf (p, "Changements d'opinions en a: %d ", mydata->changements_opinions_a);
    p += sprintf (p, "Changements d'opinions en b: %d \n", mydata->changements_opinions_b);
  }
  else printf("ERREUR IMPOSSIBLE \n");
  
  //p += sprintf (p, "Neighbor Distance: %d \n", mydata->neighbor_distance);
  //p += sprintf (p, "Message reçu: %d \n", mydata->rcvd_message);
  //p += sprintf (p, "Direction: %d \n", mydata->direction);
  //p += sprintf (p, "Intensity: %d \n", mydata->intensity);

  return botinfo_buffer;
}

/*
int16_t cb_lighting(double x, double y) {
  double light_x = -400.0;
  double light_y = 0;
  double dist_x = pow(light_x + x, 2);
  double dist_y = pow(light_y + y, 2);
  double dist_c = sqrt(dist_x + dist_y);
  return (int16_t)dist_c;
}
*/

/* 3 lumières pour représenter les 3 zones de l'arène : le site b, le nid, et le site a */

int16_t cb_lighting(double x, double y) {
  if (mydata->state == DISSEMINATION) {
    double light_x = 0;
    double light_y = 0;
    double dist_x = pow(light_x + x, 2);
    double dist_y = pow(light_y + y, 2);
    double dist_c = sqrt(dist_x + dist_y);
    return (int16_t)dist_c;
  }
  else if (mydata->state == EXPLORATION) {
    if (mydata->flag_speaker == 1) { // aller vers le site a
      double light_x = 800.0;
      double light_y = 0;
      double dist_x = pow(light_x + x, 2);
      double dist_y = pow(light_y + y, 2);
      double dist_c = sqrt(dist_x + dist_y);
      return (int16_t)dist_c;
    }
    else if (mydata->flag_speaker == 2) { // aller vers le site b
      double light_x = -800.0;
      double light_y = 0;
      double dist_x = pow(light_x + x, 2);
      double dist_y = pow(light_y + y, 2);
      double dist_c = sqrt(dist_x + dist_y);
      return (int16_t)dist_c;
    }
  }
  else printf("ERREUR IMPOSSIBLE \n");
  return 0;
}


int16_t boundaries(double x, double y, double * dx, double * dy) {
  double d = x;
  double d2 = y;
  if (d < 800.0 && d2 < 900.0) {
    if (d > -800.0 && d2 > -900.0) {
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
  if (d < 1000.0)
    return 0;
  *dx = -x/d;
  *dy = -y/d;
  return 1;
}

/* Saving bot state as JSON. */
json_t *json_state() {
  json_t* json_state = json_object();
  json_t* nb_flag_speaker = json_integer(mydata->flag_speaker);
  json_object_set(json_state, "Opinion", nb_flag_speaker);
  return json_state;
}

#endif



int main() {
  kilo_init();
  kilo_message_rx = message_rx;
  SET_CALLBACK(botinfo, cb_botinfo);
  SET_CALLBACK(lighting, cb_lighting);
  SET_CALLBACK(obstacles, boundaries);
  SET_CALLBACK(json_state, json_state);
  //SET_CALLBACK(obstacles, circle_barrier);
  //kilo_message_tx = message_tx_uid;
  kilo_message_tx = message_tx;
  kilo_start(setup, loop);
  return 0;
}