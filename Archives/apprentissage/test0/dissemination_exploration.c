#include <kilombo.h>
#include "dissemination_exploration.h"
#include <math.h>

#include <stdlib.h>
#include <stdio.h>

#define nombre_intervalle_temps_min 17 // 15

REGISTER_USERDATA(USERDATA)

// declare constants
static const uint8_t TOOCLOSE_DISTANCE = 40; // 40 mm
static const uint8_t STEP_TIME = 10;
static const uint16_t TICKS_TO_SUCCESS = 300;

static const uint8_t nbre_kilobots = 100; // nombre de kilobots
static const uint8_t NBRE_OPTIONS = 2;
static const uint8_t quality_of_site_a = 1;
static const uint8_t quality_of_site_b = 2;

static const uint32_t dissemination_time = 15624; // en kiloticks
static const uint32_t exploration_time = 11294; // en kiloticks
static const uint32_t taille_nest = 230;

// informations pour tracer les courbes
//static const uint8_t intervalle_temps_min = 15;
//static const uint32_t intervalle_temps_kiloticks = intervalle_temps_min * 1860;
// 1860 kiloticks = 1 min

// temps limite
static const uint8_t temps_limite_min = 180;
static const uint32_t temps_limite_kiloticks = temps_limite_min * 1860;

// mEDEA
static const uint8_t penalite_recompense = 5;

/*
dissemination_time = 15624; // 8,4 minutes
exploration_time = 11294; // 6,072 minutes
dissemination_time = 310; // 10 secondes
exploration_time = 124; // 4 secondes
dissemination_time = 310;
exploration_time = 310;
18600 kiloticks = 10 minutes = 600 secondes
*/

// informations pour tracer les courbes
uint8_t consensus_courant = 0;
double qualite_consensus_a[nombre_intervalle_temps_min+1];
double qualite_consensus_b[nombre_intervalle_temps_min+1];
double qualite_consensus_global[nombre_intervalle_temps_min+1];
uint8_t tableau_nbre_agents[2] = {50,50};
uint8_t dernier_indice_temps = 0;

static const double quality_total = (double)quality_of_site_a + (double)quality_of_site_b;
double nbre_theorique_a = (double)nbre_kilobots * ((double)quality_of_site_a / quality_total);
double nbre_theorique_b = (double)nbre_kilobots * ((double)quality_of_site_b / quality_total);
double genome_theorique[2] = {(double)quality_of_site_a / quality_total,(double)quality_of_site_b / quality_total};

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
    else printf("ERREUR SET_BEHAVIOR \n");
    case_light();
    mydata->stepTicks = kilo_ticks;
  }  
}

void explore() { 
  if (kilo_ticks > (mydata->last_time_update + mydata->exploration_time)) {
    mydata->state = DISSEMINATION;
    mydata->last_time_update = kilo_ticks; // YES ATTENTION TRES IMPORTANT
    if(mydata->flag_speaker == 1) set_color(RGB(2,0,0));
    else if(mydata->flag_speaker == 2) set_color(RGB(0,2,0));
    else printf("ERREUR EXPLORE \n");
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

void affichage_debugage_0() {
  printf("kilo_uid = %d \n", kilo_uid);
  mydata->flag_bug = 1;
}

void affichage_debugage_1() {
  if (mydata->flag_bug == 1) {
    printf("kilo_uid %d : site %d \n", kilo_uid,mydata->flag_speaker);
  }
}

void set_opinion_a() {
    mydata->opinion_a = 1;
    mydata->opinion_b = 0;
    tableau_nbre_agents[mydata->flag_speaker-1] --;
    mydata->flag_speaker = 1;
    tableau_nbre_agents[mydata->flag_speaker-1] ++;
    mydata->quality_of_site = quality_of_site_a;
    set_color(RGB(2,0,0));
}

void set_opinion_b() {
    mydata->opinion_a = 0;
    mydata->opinion_b = 1;
    tableau_nbre_agents[mydata->flag_speaker-1] --;
    mydata->flag_speaker = 2;
    tableau_nbre_agents[mydata->flag_speaker-1] ++;
    mydata->quality_of_site = quality_of_site_b;
    set_color(RGB(0,2,0));
}

void vider_tableau_uid() {
  uint8_t i;
  for(i=0;i<nbre_kilobots;i++) {
    mydata->tab_uid[i] = 232;
  }
  mydata->cpt_voisins = 0;
  mydata->state = EXPLORATION;
  //mydata->flag_speaker = 0;
  mydata->indice_genomeList = 0; // inutile de réinitialiser totalement soi-même mydata->genomeList
  explore();
}


double frand_a_b(double a, double b) {
  return ( rand()/(double)RAND_MAX ) * (b-a) + a;
}

void mEDEA() {
  uint32_t nb_ticks;
  nb_ticks = mydata->dissemination_time;
  if (kilo_ticks > (mydata->last_time_update + nb_ticks - 93)) { // during the last three seconds
    if (kilo_ticks > (mydata->last_time_update + nb_ticks)) {
      mydata->last_time_update = kilo_ticks;
      //double total;
      uint8_t proba_a;
      uint8_t proba_b;
      uint8_t tirage;
      double opinion_a_courante;
      double opinion_b_courante;
      opinion_a_courante = (double)mydata->opinion_a / ((double)mydata->opinion_a + (double)mydata->opinion_b);
      opinion_b_courante = (double)mydata->opinion_b / ((double)mydata->opinion_a + (double)mydata->opinion_b);

      printf("kilo_uid : %d ; (avant) mydata->flag_speaker : %d \n", kilo_uid, mydata->flag_speaker);
      /*
      // affiche genomeList
      printf("%d \n",kilo_uid);
      uint8_t i;
      for(i=0;i<nbre_kilobots;i++) {
        printf("mydata->genomeList[%d][0] : %d : mydata->genomeList[%d][1] : %d \n",i,mydata->genomeList[i][0],i,mydata->genomeList[i][1]);
      }
      */

      //proba_a = genome_theorique[0]; // test
      //proba_b = genome_theorique[0]; // test
      uint8_t selection_operator = (int)(rand() / (double)RAND_MAX * (mydata->indice_genomeList - 1));
      proba_a = mydata->genomeList[selection_operator][0];
      proba_b = mydata->genomeList[selection_operator][1];
      proba_a = 35;
      proba_b = 65;
      /*
      printf("kilo_uid : %d ; selection_operator : %d ; mydata->indice_genomeList : %d ; mydata->cpt_voisins : %d \n", kilo_uid, selection_operator,mydata->indice_genomeList,mydata->cpt_voisins);
      printf("mydata->genomeList[%d][0] : %d \n", selection_operator,mydata->genomeList[selection_operator][0]);
      printf("mydata->genomeList[%d][1] : %d \n", selection_operator,mydata->genomeList[selection_operator][1]);
      */

      
      printf("kilo_uid : %d \n",kilo_uid);
      printf("mydata->indice_genomeList : %d ; mydata->cpt_voisins : %d \n", mydata->indice_genomeList, mydata->cpt_voisins);
      printf("proba_a avant : %d \n", proba_a);
      printf("proba_b avant : %d \n", proba_b);
      printf("opinion_a_courante : %f ; genome_theorique[0] : %f \n", opinion_a_courante, genome_theorique[0]);
      printf("opinion_b_courante : %f ; genome_theorique[1] : %f \n", opinion_b_courante, genome_theorique[1]);
      
      /*
      if (opinion_a_courante > genome_theorique[0] && opinion_b_courante < genome_theorique[1]) {
        if (proba_a <= mydata->penalite_recompense) {
          proba_a = 0;
          proba_b = nbre_kilobots;
        }
        else {
          proba_a -= mydata->penalite_recompense;
          proba_b += mydata->penalite_recompense;
        }
        //mydata->penalite_recompense = mydata->penalite_recompense * 2;
      }
      else if (opinion_a_courante < genome_theorique[0] && opinion_b_courante > genome_theorique[1]) {
        if (proba_b <= mydata->penalite_recompense) {
          proba_a = nbre_kilobots;
          proba_b = 0;
        }
        else {
          proba_a += mydata->penalite_recompense;
          proba_b -= mydata->penalite_recompense;
        }
        //mydata->penalite_recompense = mydata->penalite_recompense * 2;
      }
      else if (opinion_a_courante == genome_theorique[0] && opinion_b_courante == genome_theorique[1]) {
        proba_a = proba_a;
        proba_b = proba_b;
        //mydata->penalite_recompense = mydata->penalite_recompense / 2;
        //printf("%f \n", pow(2,-1/4));
        //mydata->penalite_recompense = mydata->penalite_recompense * pow(2.0,-0.25);
        //mydata->penalite_recompense = mydata->penalite_recompense - 6;
        //printf("mydata->penalite_recompense : %d \n", mydata->penalite_recompense);
        //exit(0);
      }
      else {
        printf("ERREUR mEDEA 1\n");
      }
      */

      //printf("mydata->penalite_recompense : %d \n", mydata->penalite_recompense);

      /*
      // si on dépasse les limites des bornes [0;nbre_kilobots]
      if (proba_a < 0 || proba_b > nbre_kilobots) {
        proba_a = 0;
        proba_b = nbre_kilobots;
      }
      else if (proba_a > nbre_kilobots || proba_b < 0) {
        proba_a = nbre_kilobots;
        proba_b = 0;
      }
      */
      
      tirage = (int)(rand() / (double)RAND_MAX * (nbre_kilobots - 1));
      
      printf("tirage : %d \n",tirage);
      printf("proba_a après : %d \n", proba_a);
      printf("proba_b après : %d \n", proba_b);
      
      if (tirage <= proba_a) set_opinion_a();
      else if (tirage <= proba_a + proba_b) set_opinion_b();
      else printf("ERREUR mEDEA 2 \n"); // peut-être souci de nest trop petit qui provoque des NaN
      
      vider_tableau_uid();
      printf("kilo_uid : %d ; (apres) mydata->flag_speaker : %d \n", kilo_uid, mydata->flag_speaker);
      //exit(0);
    }
  }
  //exit(0);
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
  if (mydata->intensity <= taille_nest) {
    mydata->flag_nest = 1;
  }
  else mydata->flag_nest = 0;
}

void loop() {
  //printf("%d \n",kilo_uid);
  //if(kilo_uid == 0 && (kilo_ticks % intervalle_temps_kiloticks == 0 || kilo_ticks % intervalle_temps_kiloticks == 1) && kilo_ticks != 1) {
  //uint8_t toto[2][4] = {{1,2,3,4},{5,6,7,8}};
  //printf("%d \n", toto[1][3]);
  //exit(0);
  if(kilo_uid == 0 && (kilo_ticks % temps_limite_kiloticks == 0 || kilo_ticks % temps_limite_kiloticks == 1) && kilo_ticks >= temps_limite_kiloticks) {
  //if(kilo_uid == 0 && (kilo_ticks == temps_limite_kiloticks)) {
    printf("######################### TEMPS LIMITE ATTEINT ######################### \n");
    printf("Nombre d'agents théorique pour le site a : %f \n", nbre_theorique_a);
    printf("Nombre d'agents théorique pour le site b : %f \n", nbre_theorique_b);
    printf("Nombre d'agents réel pour le site a : %d \n", tableau_nbre_agents[0]);
    printf("Nombre d'agents réel pour le site b : %d \n", tableau_nbre_agents[1]);
    uint32_t min = 0;
    printf("Kiloticks du consensus : %d \n", kilo_ticks);
    min = kilo_ticks / 31 / 60;
    printf("Temps en min du consensus : %d \n", min);
    exit(0);
  }
  if(mydata->state == DISSEMINATION) {
    nest();
    if (mydata->new_message) {
      mydata->new_message = 0; // On remet le flag à O
      collisions();
      //nest();
      if(mydata->flag_nest) {     
        if (mydata->flag_listener == 1 && !mydata->flag_voisin_deja_rencontre) {
          mydata->opinion_a ++;
          mydata->cpt_voisins ++;
          mydata->indice_genomeList ++;
        }            
        else if (mydata->flag_listener == 2 && !mydata->flag_voisin_deja_rencontre) {
          mydata->opinion_b ++;
          mydata->cpt_voisins ++;
          mydata->indice_genomeList ++;
        }
        //else // rencontre un voisin en exploration, donc ne compte pas
      }  
    }
    else { // si aucun message reçu en cas de DISSEMINATION
      set_behavior();
    }
    mEDEA();
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
  else printf("ERREUR LOOP \n");
}

/* SPEAKER */

void setup_message(void) {
  mydata->message_a.type = NORMAL;
	mydata->message_a.data[0] = 1;
  mydata->message_a.data[1] = kilo_uid;
  mydata->message_a.data[2] = mydata->genome[0];
  mydata->message_a.data[3] = mydata->genome[1];
	mydata->message_a.crc = message_crc(&mydata->message_a);

  mydata->message_b.type = NORMAL;
	mydata->message_b.data[0] = 2;
  mydata->message_b.data[1] = kilo_uid;
  mydata->message_b.data[2] = mydata->genome[0];
  mydata->message_b.data[3] = mydata->genome[1];
	mydata->message_b.crc = message_crc(&mydata->message_b);

  mydata->message_exploration.type = NORMAL;
	mydata->message_exploration.data[0] = 0;
  mydata->message_exploration.data[1] = kilo_uid;
	mydata->message_exploration.crc = message_crc(&mydata->message_exploration);
  //printf("kilo_uid : %d ; mydata->message_a.data[2] : %f ; mydata->genome[0] : %f \n", kilo_uid, mydata->message_a.data[2],mydata->genome[0]);
  //printf("kilo_uid : %d ; mydata->message_a.data[2] : %d ; mydata->genome[0] : %d \n", kilo_uid, mydata->message_a.data[2],mydata->genome[0]);
  //printf("kilo_uid : %d ; mydata->message_a.data[2] : %f ; mydata->genome[0] : %f \n", kilo_uid, mydata->message_a.data[2],mydata->genome[0]);
  //printf("mydata->genome[0] : %d \n",mydata->genome[0]);
  //exit(0);
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
    else printf("ERREUR MESSAGE_TX 1 \n");
  }
  else if(mydata->state == EXPLORATION) {
  	return &mydata->message_exploration;
  }
  else printf("ERREUR MESSAGE_TX 2 \n");
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
    if(recherche(mydata->tab_uid,100,msg->data[1])) { // voisin déjà rencontré
      mydata->flag_voisin_deja_rencontre = 1;
    }
    else { // si le voisin n'est pas encore rencontré
      mydata->flag_voisin_deja_rencontre = 0;
      mydata->tab_uid[mydata->cpt_voisins] = msg->data[1];
      mydata->genomeList[mydata->indice_genomeList][0] = msg->data[2];
      mydata->genomeList[mydata->indice_genomeList][1] = msg->data[3];
      //mydata->cpt_voisins ++;
    }
  }
  else if (mydata->state == EXPLORATION) {

  }
  else {
    printf("ERREUR MESSAGE_RX \n");
  }
}

void setup_light() { 
  mydata->intensity = 1023;
  mydata->lastIntensity = 0;
  mydata->search_state = NONE;
  mydata->lastTicks = 0;
  mydata->stepTicks = 0;
}

void setup() {// initialisation au tout début, une seule fois
  mydata->flag_nest = 0;
  mydata->behavior_state = PHOTOTAXIS;

  mydata->neighbor_distance = 0;
  mydata->direction = FORWARD;
  mydata->new_message = 0;

  mydata->state = DISSEMINATION;

  mydata->cpt_voisins = 0;
  mydata->flag_voisin_deja_rencontre = 0;

  mydata->last_time_update = kilo_ticks;

  mydata->quality_of_site = 2;

  // conversion en nombre de ticks
  mydata->dissemination_time = dissemination_time;
  mydata->exploration_time = exploration_time;

  //setup_message(); // A METTRE APRES L'INITIALISATION DU GENOME
  setup_light();

  /* INITIALISATION */

  if (kilo_uid % NBRE_OPTIONS == 0) {
    /* OPINION A*/
    set_color(RGB(3,0,0));
    mydata->opinion_a = 1;
    mydata->opinion_b = 0;
    mydata->flag_speaker = 1;
  }
  else if (kilo_uid % NBRE_OPTIONS == 1) {
    /* OPINION B */
    set_color(RGB(0,3,0));
    mydata->opinion_a = 0;
    mydata->opinion_b = 1;
    mydata->flag_speaker = 2;
  }
  else printf("ERREUR SETUP INITIALISATION \n");

  mydata->flag_bug = 0;

  mydata->indice_temps = 0;

  /* Initialisation du génome */

  uint8_t tirage = (int)(rand() / (double)RAND_MAX * (nbre_kilobots - 1));
  //uint8_t tirage;
  //printf("kilo_uid : %d ; tirage : %d \n",kilo_uid,tirage);
  uint8_t i;
  for(i=0;i<NBRE_OPTIONS;i++) {
    if (i == NBRE_OPTIONS-1 || i == 0) {
      tirage = nbre_kilobots - tirage;
    }
    else if (i < NBRE_OPTIONS-1) {
      tirage = (int)(rand() / (double)RAND_MAX * ((nbre_kilobots - tirage) - 1));
      //tirage = nbre_kilobots - tirage;
    }
    else printf("ERREUR SETUP GENOME\n");
    mydata->genome[i] = tirage;
    //printf("kilo_uid : %d ; mydata->genome[%d] : %d \n", kilo_uid, i, mydata->genome[i]);
  }

  mydata->indice_genomeList = 0;
  mydata->penalite_recompense = penalite_recompense;

  setup_message(); // attention à mettre après l'initialisation du génome
  //exit(0);
}

#ifdef SIMULATOR

static char botinfo_buffer[10000];
char *cb_botinfo(void) {
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
      p += sprintf (p, "Opinion a: %f ;", proba_a);
      p += sprintf (p, "Opinion b: %f ;", proba_b);
      p += sprintf (p, "cpt voisins : %d \n", mydata->cpt_voisins);
      p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
      p += sprintf (p, "Genome courant : [ %d , %d ] \n", mydata->genome[0], mydata->genome[1]);
  }
  else if (mydata->state == EXPLORATION) {
    p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
    p += sprintf (p, "Genome courant : [ %d , %d ] \n", mydata->genome[0], mydata->genome[1]);
  }
  else printf("ERREUR CB_BOTINFO \n");
  
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
      double light_x = 600.0;
      double light_y = 0.0;
      double dist_x = pow(light_x + x, 2);
      double dist_y = pow(light_y + y, 2);
      double dist_c = sqrt(dist_x + dist_y);
      return (int16_t)dist_c;
    }
    else if (mydata->flag_speaker == 2) { // aller vers le site b
      double light_x = -600.0;
      double light_y = 0.0;
      double dist_x = pow(light_x + x, 2);
      double dist_y = pow(light_y + y, 2);
      double dist_c = sqrt(dist_x + dist_y);
      return (int16_t)dist_c;
    }
    else printf("ERREUR CB_LIGHTING 1 \n");
  }
  else printf("ERREUR CB_LIGHTING 2 \n");
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

  json_t* genome_a = json_integer(mydata->genome[0]);
  json_object_set(json_state, "genome_a", genome_a);

  json_t* genome_b = json_integer(mydata->genome[1]);
  json_object_set(json_state, "genome_b", genome_b);

  return json_state;
}

#endif

int main() {
  kilo_init();
  kilo_message_rx = message_rx;
  SET_CALLBACK(botinfo, cb_botinfo);
  SET_CALLBACK(lighting, cb_lighting);
  //SET_CALLBACK(obstacles, boundaries);
  SET_CALLBACK(json_state, json_state);
  //SET_CALLBACK(obstacles, circle_barrier);
  //kilo_message_tx = message_tx_uid;
  kilo_message_tx = message_tx;
  kilo_start(setup, loop);
  return 0;
}


// arrondir au centième près
// modifier la penalite_recompense
// quand le consensus est atteint ?