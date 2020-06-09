#include <kilombo.h>
#include "dissemination_exploration.h"
#include <math.h>

#include <stdlib.h>
#include <stdio.h>

#define nombre_intervalle_temps_min 17 // 15 17

REGISTER_USERDATA(USERDATA)

// declare constants
static const uint8_t TOOCLOSE_DISTANCE = 40; // 40 mm
static const uint8_t STEP_TIME = 10;
static const uint32_t TICKS_TO_SUCCESS = 300;

static const uint8_t NBRE_OPTIONS = 2;
static const uint8_t quality_of_site_a = 1;
static const uint8_t quality_of_site_b = 2;
static const uint8_t quality_of_site_a_bis = 2;
static const uint8_t quality_of_site_b_bis = 1;
static const uint32_t temps_changement_consensus = 260400;
// 260400 kiloticks = 140min

static const uint8_t nbre_initial_agents_tetus = NBRE_OPTIONS;
static const uint32_t dissemination_time = 15624; // en kiloticks
static const uint32_t exploration_time = 11294; // en kiloticks
static const uint32_t taille_nest = 230;
static const uint8_t seuil_voter_model = 60;

// informations pour tracer les courbes
static const uint8_t intervalle_temps_min = 30;
static const uint32_t intervalle_temps_kiloticks = intervalle_temps_min * 1860;
// 1860 kiloticks = 1 min

// variables globales
uint8_t consensus_site_a = nbre_initial_agents_tetus / NBRE_OPTIONS;
uint8_t consensus_site_b = nbre_initial_agents_tetus / NBRE_OPTIONS;
uint8_t consensus_courant = nbre_initial_agents_tetus;
uint8_t consensus_atteint = 99; // nombre de kilobots

double nbre_theorique_a = 0;
double nbre_theorique_b = 99;

// informations pour tracer les courbes
double qualite_consensus_a[nombre_intervalle_temps_min+1];
double qualite_consensus_b[nombre_intervalle_temps_min+1];
double qualite_consensus_global[nombre_intervalle_temps_min+1];
uint8_t tableau_nbre_agents[2] = {50, 50};
uint8_t dernier_indice_temps = 0;

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
    if(!mydata->tetu) {
      if(mydata->flag_speaker == 1) set_color(RGB(3,0,0));
      else if(mydata->flag_speaker == 2) set_color(RGB(0,3,0));
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
  if (kilo_ticks >= temps_changement_consensus*2 && (consensus_atteint == tableau_nbre_agents[0] || consensus_atteint == tableau_nbre_agents[1])) {
    printf("######################### CONSENSUS ATTEINT ######################### \n");
    printf("Kiloticks du consensus : %d \n", kilo_ticks);
    uint32_t min = 0;
    min = kilo_ticks / 31 / 60;
    printf("Temps en min du consensus : %d \n", min);

    FILE* fichier = NULL;
    fichier = fopen("resultats_bruts.txt","w");
    if (fichier != NULL) {
      fprintf(fichier, "nbre_theorique_a\n%f\n", nbre_theorique_a);
      fprintf(fichier, "nbre_theorique_b\n%f\n", nbre_theorique_b);
      fprintf(fichier, "nbre_experimental_a\n%d\n", tableau_nbre_agents[0]);
      fprintf(fichier, "nbre_experimental_b\n%d\n", tableau_nbre_agents[1]);
      fprintf(fichier, "Kiloticks du consensus\n%d\n", kilo_ticks);
      fprintf(fichier, "Temps en min du consensus\n%d\n", min);
      fclose(fichier);
    }
    printf("dernier_indice_temps = %d \n", dernier_indice_temps);  
    if ((double)tableau_nbre_agents[0] / 2.0 <= nbre_theorique_a)
      qualite_consensus_a[dernier_indice_temps] = fabs((nbre_theorique_a - fabs(nbre_theorique_a - tableau_nbre_agents[0])) / nbre_theorique_a);
    else qualite_consensus_a[dernier_indice_temps] = 0.0;
    
    if ((double)tableau_nbre_agents[1] / 2.0 <= nbre_theorique_b)
      qualite_consensus_b[dernier_indice_temps] = fabs((nbre_theorique_b - fabs(nbre_theorique_b - tableau_nbre_agents[1])) / nbre_theorique_b);
    else qualite_consensus_b[dernier_indice_temps] = 0.0;

    qualite_consensus_global[dernier_indice_temps] = (qualite_consensus_a[dernier_indice_temps] + qualite_consensus_b[dernier_indice_temps]) / NBRE_OPTIONS;

    printf("######################### QUALITES FINALES ######################### \n");
    //printf("qualite_consensus_a = %f\n", qualite_consensus_a[dernier_indice_temps]);
    printf("qualite_consensus_b = %f\n", qualite_consensus_b[dernier_indice_temps]);
    //printf("qualite_consensus_global = %f\n", qualite_consensus_global[dernier_indice_temps]);

    FILE* fichier2 = NULL;
    fichier2 = fopen("resultats_nets.txt","w");
    if (fichier2 != NULL) {

      uint8_t i;

      fprintf(fichier2, "qualite_consensus_a\n");
      for (i = 0; i < nombre_intervalle_temps_min; i++) {
        if (i == nombre_intervalle_temps_min-1) fprintf(fichier2, "%f \n", qualite_consensus_a[dernier_indice_temps]);
        else {
          if (i > dernier_indice_temps) fprintf(fichier2, "%f ", qualite_consensus_a[dernier_indice_temps]);
          else fprintf(fichier2, "%f ", qualite_consensus_a[i]);
        }
      }

      fprintf(fichier2, "qualite_consensus_b\n");
      for (i = 0; i < nombre_intervalle_temps_min; i++) {
        if (i == nombre_intervalle_temps_min-1) fprintf(fichier2, "%f \n", qualite_consensus_b[dernier_indice_temps]);
        else {
          if (i > dernier_indice_temps) fprintf(fichier2, "%f ", qualite_consensus_b[dernier_indice_temps]);
          else fprintf(fichier2, "%f ", qualite_consensus_b[i]);
        }
      }

      fprintf(fichier2, "qualite_consensus_global\n");
      for (i = 0; i < nombre_intervalle_temps_min; i++) {
        if (i == nombre_intervalle_temps_min) fprintf(fichier2, "%f \n", qualite_consensus_global[dernier_indice_temps]);
        else {
          if (i > dernier_indice_temps) fprintf(fichier2, "%f ", qualite_consensus_global[dernier_indice_temps]);
          else fprintf(fichier2, "%f ", qualite_consensus_global[i]);
        }
      }

      fprintf(fichier2, "Kiloticks du consensus\n%d\n", kilo_ticks);
      fprintf(fichier2, "Temps en min du consensus\n%d\n", min);
      fclose(fichier2);
    }
    exit(0);
  }
}

void set_opinion_a() {
  if (!mydata->tetu) {
    mydata->opinion_a = 1;
    mydata->opinion_b = 0;
    tableau_nbre_agents[mydata->flag_speaker-1] --;
    mydata->flag_speaker = 1;
    tableau_nbre_agents[mydata->flag_speaker-1] ++;
    mydata->quality_of_site = quality_of_site_a;
    if (kilo_ticks >= temps_changement_consensus) mydata->quality_of_site = quality_of_site_a_bis;
    set_color(RGB(2,0,0));
    affichage_resultats();
  }
}

void set_opinion_b() {
  if (!mydata->tetu) {
    mydata->opinion_a = 0;
    mydata->opinion_b = 1;
    tableau_nbre_agents[mydata->flag_speaker-1] --;
    mydata->flag_speaker = 2;
    tableau_nbre_agents[mydata->flag_speaker-1] ++;
    mydata->quality_of_site = quality_of_site_b;
    if (kilo_ticks >= temps_changement_consensus) mydata->quality_of_site = quality_of_site_b_bis;
    set_color(RGB(0,2,0));
    affichage_resultats();
  }
}

void vider_tableau_uid() {
  uint8_t i;
  for(i=0;i<100;i++) {
    mydata->tab_uid[i] = 232;
  }
  mydata->cpt_voisins = 0;
  mydata->state = EXPLORATION;
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
        else printf("ERREUR VOTER_MODEL \n");
        vider_tableau_uid();
      }
      else {// agent têtu
        mydata->state = EXPLORATION;
        explore();
        if(kilo_ticks >= temps_changement_consensus) {
          if (mydata->flag_speaker == 1) mydata->quality_of_site = quality_of_site_a_bis;
          else if (mydata->flag_speaker == 2) mydata->quality_of_site = quality_of_site_b_bis;
        }
        else {
          if (mydata->flag_speaker == 1) mydata->quality_of_site = quality_of_site_a;
          else if (mydata->flag_speaker == 2) mydata->quality_of_site = quality_of_site_b;
        }        
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
  
    if (mydata->intensity <= taille_nest) {
      mydata->flag_nest = 1;
    }
    else mydata->flag_nest = 0;
}

void loop() {

  if(kilo_uid == 0 && (kilo_ticks % intervalle_temps_kiloticks == 0 || kilo_ticks % intervalle_temps_kiloticks == 1) && kilo_ticks != 1) {
    if ((double)tableau_nbre_agents[0] / 2.0 <= nbre_theorique_a)
      qualite_consensus_a[dernier_indice_temps] = fabs((nbre_theorique_a - fabs(nbre_theorique_a - tableau_nbre_agents[0])) / nbre_theorique_a);
    else qualite_consensus_a[dernier_indice_temps] = 0.0;
    
    if ((double)tableau_nbre_agents[1] / 2.0 <= nbre_theorique_b)
      qualite_consensus_b[dernier_indice_temps] = fabs((nbre_theorique_b - fabs(nbre_theorique_b - tableau_nbre_agents[1])) / nbre_theorique_b);
    else qualite_consensus_b[dernier_indice_temps] = 0.0;
    
    qualite_consensus_global[mydata->indice_temps] = (qualite_consensus_a[mydata->indice_temps] + qualite_consensus_b[mydata->indice_temps]) / NBRE_OPTIONS;
    
    //printf("mydata->indice_temps = %d \n", mydata->indice_temps);
    mydata->indice_temps ++;
    dernier_indice_temps = mydata->indice_temps;
  }

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

}

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

/* LISTENER */
void message_rx(message_t *msg, distance_measurement_t *dist) {
  mydata->new_message = 1; // set the flag to 1 to indicate that a new message arrived (en gros c'est un "accusé de réception" mais pour le listener)
  mydata->dist = *dist;
  mydata->flag_listener = msg->data[0];
  if(mydata->state == DISSEMINATION && mydata->flag_listener != 0) {
    if(recherche(mydata->tab_uid,100,msg->data[1])) {
      mydata->flag_voisin_deja_rencontre = 1;
    }
    else {
      mydata->flag_voisin_deja_rencontre = 0;
      mydata->tab_uid[mydata->cpt_voisins] = msg->data[1];
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

  if (kilo_uid % NBRE_OPTIONS == 0) {
    /* OPINION A*/
    set_color(RGB(3,0,0));
    mydata->opinion_a = 1;
    mydata->opinion_b = 0;
    mydata->flag_speaker = 1;
    mydata->tetu = 0;
  }
  else if (kilo_uid % NBRE_OPTIONS == 1) {
    /* OPINION B */
    set_color(RGB(0,3,0));
    mydata->opinion_a = 0;
    mydata->opinion_b = 1;
    mydata->flag_speaker = 2;
    mydata->tetu = 0;
  }
  else printf("ERREUR SETUP INITIALISATION \n");
  /* seuil du voter model */
  mydata->seuil_voter_model = seuil_voter_model;

  /* AGENTS TETUS DEPUIS LE DEBUT */
  if (kilo_uid < nbre_initial_agents_tetus) {
    if (kilo_uid % NBRE_OPTIONS == 0) {
      mydata->tetu = 1;
      mydata->opinion_a = 1;
      mydata->opinion_b = 0;
      mydata->flag_speaker = 1;
      //mydata->quality_of_site = 2;
      set_color(RGB(1,0,0));
    }
    else if (kilo_uid % NBRE_OPTIONS == 1) {
      mydata->tetu = 1;
      mydata->opinion_a = 0;
      mydata->opinion_b = 1;
      mydata->flag_speaker = 2;
      //mydata->quality_of_site = 2;
      set_color(RGB(0,1,0));
    }
  }
}

#ifdef SIMULATOR

static char botinfo_buffer[10000];
char *cb_botinfo(void)
{
  char *p = botinfo_buffer;

  double total;
  double proba_a;
  double proba_b;
  total = mydata->opinion_a + mydata->opinion_b;
  proba_a = mydata->opinion_a / total;
  proba_b = mydata->opinion_b / total;
  
  if (mydata->state == DISSEMINATION) {
    if (!mydata->tetu) {
      p += sprintf (p, "Agent non tetu \n");
      p += sprintf (p, "Opinion a: %f ", proba_a);
    	p += sprintf (p, "Opinion b: %f \n", proba_b);
      p += sprintf (p, "cpt voisins : %d \n", mydata->cpt_voisins);
    }
    else {
      p += sprintf (p, "Agent tetu \n");
      p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
    }
  }
  else if (mydata->state == EXPLORATION) {
    if (!mydata->tetu) {
      p += sprintf (p, "Agent non tetu \n");
      p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
    }
    else {
      p += sprintf (p, "Agent tetu \n");
      p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
    }
  }
  else printf("ERREUR IMPOSSIBLE \n");

  return botinfo_buffer;
}

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

/* Saving bot state as JSON. */
json_t *json_state() {
  json_t* json_state = json_object();
  json_t* nb_flag_speaker = json_integer(mydata->flag_speaker);
  json_object_set(json_state, "Opinion", nb_flag_speaker);
  json_t* seuil = json_integer(mydata->seuil_voter_model);
  json_object_set(json_state, "Seuil", seuil);
  return json_state;
}

#endif

int main() {

  kilo_init();
  kilo_message_rx = message_rx;
  SET_CALLBACK(botinfo, cb_botinfo);
  SET_CALLBACK(lighting, cb_lighting);
  SET_CALLBACK(json_state, json_state);
  kilo_message_tx = message_tx;
  kilo_start(setup, loop);
  return 0;
}