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

static const uint8_t consensus_atteint = 100; // nombre de kilobots
static const uint8_t NBRE_OPTIONS = 4;
static const uint8_t quality_of_site_a = 1;
static const uint8_t quality_of_site_b = 2;
static const uint8_t quality_of_site_c = 3;
static const uint8_t quality_of_site_d = 4;

static const uint8_t nbre_initial_agents_tetus = 4; //12 = 3 têtus ; 16 = 4 têtus
static const uint8_t seuil_max_changements_opinions = 12; // 7 ou 8 max
static const uint8_t seuil_min_changements_opinions = 12; // ATTENTION MIN
static const uint8_t iteration_critique_decision = 2; // itération où le seuil est fixé
static const uint8_t poids_changements_opinions = 1;
static const uint8_t iteration_critique_changements_opinions = 3;

static const uint16_t dissemination_time = 15624; // en kiloticks
static const uint16_t exploration_time = 11294; // en kiloticks
static const uint8_t taille_nest = 230;

// informations pour tracer les courbes
static const uint8_t intervalle_temps_min = 10;
static const uint16_t intervalle_temps_kiloticks = intervalle_temps_min * 1860;
// 1860 kiloticks = 1 min

/*
dissemination_time = 15624; // 8,4 minutes
exploration_time = 11294; // 6,072 minutes
dissemination_time = 310; // 10 secondes
exploration_time = 124; // 4 secondes
dissemination_time = 310;
exploration_time = 310;
18600 kiloticks = 10 minutes = 600 secondes
*/

// variables globales
uint8_t consensus_site_a = nbre_initial_agents_tetus / NBRE_OPTIONS;
uint8_t consensus_site_b = nbre_initial_agents_tetus / NBRE_OPTIONS;
uint8_t consensus_site_c = nbre_initial_agents_tetus / NBRE_OPTIONS;
uint8_t consensus_site_d = nbre_initial_agents_tetus / NBRE_OPTIONS;
uint8_t consensus_courant = nbre_initial_agents_tetus;

// informations pour tracer les courbes
uint8_t qualite_consensus_a[nombre_intervalle_temps_min+1];
uint8_t qualite_consensus_b[nombre_intervalle_temps_min+1];
uint8_t qualite_consensus_c[nombre_intervalle_temps_min+1];
uint8_t qualite_consensus_d[nombre_intervalle_temps_min+1];
uint8_t qualite_consensus_global[nombre_intervalle_temps_min+1];
uint8_t tableau_nbre_agents[4] = {25, 25, 25, 25};
uint8_t dernier_indice_temps = 0;

uint8_t quality_total = quality_of_site_a + quality_of_site_b + quality_of_site_c + quality_of_site_d;
uint8_t nbre_theorique_a = 10;
uint8_t nbre_theorique_b = 20;
uint8_t nbre_theorique_c = 30;
uint8_t nbre_theorique_d = 40;

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
      if(mydata->flag_speaker == 1) set_color(RGB(2,0,0));
      else if(mydata->flag_speaker == 2) set_color(RGB(0,2,0));
      else if(mydata->flag_speaker == 3) set_color(RGB(0,0,2));
      else if(mydata->flag_speaker == 4) set_color(RGB(2,2,2));
      else printf("ERREUR EXPLORE \n");
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
  //printf("consensus_courant : %d \n",consensus_courant);
  if (consensus_courant == consensus_atteint) {
    printf("######################### CONSENSUS ATTEINT ######################### \n");
    printf("Nombre d'agents théorique pour le site a : %d \n", nbre_theorique_a);
    printf("Nombre d'agents théorique pour le site b : %d \n", nbre_theorique_b);
    printf("Nombre d'agents théorique pour le site c : %d \n", nbre_theorique_c);
    printf("Nombre d'agents théorique pour le site d : %d \n", nbre_theorique_d);
    printf("Nombre d'agents réel pour le site a : %d \n", consensus_site_a);
    printf("Nombre d'agents réel pour le site b : %d \n", consensus_site_b);
    printf("Nombre d'agents réel pour le site c : %d \n", consensus_site_c);
    printf("Nombre d'agents réel pour le site d : %d \n", consensus_site_d);
    uint16_t min = 0;
    printf("Kiloticks du consensus : %d \n", kilo_ticks);
    min = kilo_ticks / 31 / 60;
    printf("Temps en min du consensus : %d \n", min);

    FILE* fichier = NULL;
    fichier = fopen("resultats_bruts.txt","w");
    if (fichier != NULL) {
      fprintf(fichier, "nbre_theorique_a\n%d\n", nbre_theorique_a);
      fprintf(fichier, "nbre_theorique_b\n%d\n", nbre_theorique_b);
      fprintf(fichier, "nbre_theorique_c\n%d\n", nbre_theorique_c);
      fprintf(fichier, "nbre_theorique_d\n%d\n", nbre_theorique_d);
      fprintf(fichier, "nbre_experimental_a\n%d\n", consensus_site_a);
      fprintf(fichier, "nbre_experimental_b\n%d\n", consensus_site_b);
      fprintf(fichier, "nbre_experimental_c\n%d\n", consensus_site_c);
      fprintf(fichier, "nbre_experimental_d\n%d\n", consensus_site_d);
      fprintf(fichier, "Kiloticks du consensus\n%d\n", kilo_ticks);
      fprintf(fichier, "Temps en min du consensus\n%d\n", min);
      fclose(fichier);
    }

    printf("dernier_indice_temps = %d \n", dernier_indice_temps);
    
    qualite_consensus_a[dernier_indice_temps] = tableau_nbre_agents[0];
    qualite_consensus_b[dernier_indice_temps] = tableau_nbre_agents[1];
    qualite_consensus_c[dernier_indice_temps] = tableau_nbre_agents[2];
    qualite_consensus_d[dernier_indice_temps] = tableau_nbre_agents[3];
    qualite_consensus_global[dernier_indice_temps] = (qualite_consensus_a[dernier_indice_temps] + qualite_consensus_b[dernier_indice_temps] + qualite_consensus_c[dernier_indice_temps] + qualite_consensus_d[dernier_indice_temps]) / NBRE_OPTIONS;

    printf("######################### QUALITES FINALES ######################### \n");
    printf("qualite_consensus_a = %d\n", qualite_consensus_a[dernier_indice_temps]);
    printf("qualite_consensus_b = %d\n", qualite_consensus_b[dernier_indice_temps]);
    printf("qualite_consensus_c = %d\n", qualite_consensus_c[dernier_indice_temps]);
    printf("qualite_consensus_d = %d\n", qualite_consensus_d[dernier_indice_temps]);
    printf("qualite_consensus_global = %d\n", qualite_consensus_global[dernier_indice_temps]);

    FILE* fichier2 = NULL;
    fichier2 = fopen("resultats_nets.txt","w");
    if (fichier2 != NULL) {

      uint8_t i;

      fprintf(fichier2, "qualite_consensus_a\n");
      for (i = 0; i < nombre_intervalle_temps_min; i++) {
        if (i == nombre_intervalle_temps_min-1) fprintf(fichier2, "%d \n", qualite_consensus_a[dernier_indice_temps]);
        else {
          if (i > dernier_indice_temps) fprintf(fichier2, "%d ", qualite_consensus_a[dernier_indice_temps]);
          else fprintf(fichier2, "%d ", qualite_consensus_a[i]);
        }
      }

      fprintf(fichier2, "qualite_consensus_b\n");
      for (i = 0; i < nombre_intervalle_temps_min; i++) {
        if (i == nombre_intervalle_temps_min-1) fprintf(fichier2, "%d \n", qualite_consensus_b[dernier_indice_temps]);
        else {
          if (i > dernier_indice_temps) fprintf(fichier2, "%d ", qualite_consensus_b[dernier_indice_temps]);
          else fprintf(fichier2, "%d ", qualite_consensus_b[i]);
        }
      }

      fprintf(fichier2, "qualite_consensus_c\n");
      for (i = 0; i < nombre_intervalle_temps_min; i++) {
        if (i == nombre_intervalle_temps_min-1) fprintf(fichier2, "%d \n", qualite_consensus_c[dernier_indice_temps]);
        else {
          if (i > dernier_indice_temps) fprintf(fichier2, "%d ", qualite_consensus_c[dernier_indice_temps]);
          else fprintf(fichier2, "%d ", qualite_consensus_c[i]);
        }
      }

      fprintf(fichier2, "qualite_consensus_d\n");
      for (i = 0; i < nombre_intervalle_temps_min; i++) {
        if (i == nombre_intervalle_temps_min-1) fprintf(fichier2, "%d \n", qualite_consensus_d[dernier_indice_temps]);
        else {
          if (i > dernier_indice_temps) fprintf(fichier2, "%d ", qualite_consensus_d[dernier_indice_temps]);
          else fprintf(fichier2, "%d ", qualite_consensus_d[i]);
        }
      }

      fprintf(fichier2, "qualite_consensus_global\n");
      for (i = 0; i < nombre_intervalle_temps_min; i++) {
        if (i == nombre_intervalle_temps_min) fprintf(fichier2, "%d \n", qualite_consensus_global[dernier_indice_temps]);
        else {
          if (i > dernier_indice_temps) fprintf(fichier2, "%d ", qualite_consensus_global[dernier_indice_temps]);
          else fprintf(fichier2, "%d ", qualite_consensus_global[i]);
        }
      }

      fprintf(fichier2, "Kiloticks du consensus\n%d\n", kilo_ticks);
      fprintf(fichier2, "Temps en min du consensus\n%d\n", min);
      fclose(fichier2);
    }
    exit(0);
  }
}

void affichage_debugage_0() {
  printf("kilo_uid = %d \n", kilo_uid);
  printf("changements_opinions_a = %d \n", mydata->changements_opinions_a);
  printf("changements_opinions_b = %d \n", mydata->changements_opinions_b);
  printf("changements_opinions_c = %d \n", mydata->changements_opinions_c);
  printf("changements_opinions_d = %d \n", mydata->changements_opinions_d);
  mydata->flag_bug = 1;
}

void affichage_debugage_1() {
  if (mydata->flag_bug == 1) {
    printf("kilo_uid %d : site %d \n", kilo_uid,mydata->flag_speaker);
  }
}

void analyse_iteration_ideale() {
  double quality_total = (double)quality_of_site_a + (double)quality_of_site_b + (double)quality_of_site_c + (double)quality_of_site_d;
  if (mydata->flag_speaker == 1 && (double)consensus_atteint * ((double)quality_of_site_a / quality_total) == (double)consensus_site_a) printf("Kilo_uid : %d : Nombre idéale d'itérations du site %d : %d => %d \n",kilo_uid,mydata->flag_speaker,mydata->changements_opinions,consensus_site_a);
  else if (mydata->flag_speaker == 2 && (double)consensus_atteint * ((double)quality_of_site_b / quality_total) == (double)consensus_site_b) printf("Kilo_uid : %d : Nombre idéale d'itérations du site %d : %d => %d \n",kilo_uid,mydata->flag_speaker,mydata->changements_opinions,consensus_site_b);
  else if (mydata->flag_speaker == 3 && (double)consensus_atteint * ((double)quality_of_site_c / quality_total) == (double)consensus_site_c) printf("Kilo_uid : %d : Nombre idéale d'itérations du site %d : %d => %d \n",kilo_uid,mydata->flag_speaker,mydata->changements_opinions,consensus_site_c);
  else if (mydata->flag_speaker == 4 && (double)consensus_atteint * ((double)quality_of_site_d / quality_total) == (double)consensus_site_d) printf("Kilo_uid : %d : Nombre idéale d'itérations du site %d : %d => %d \n",kilo_uid,mydata->flag_speaker,mydata->changements_opinions,consensus_site_d);
  //else printf("ERREUR ANALYSE_ITERATION_IDEALE\n");
}

void transition_tetu_a() {
  mydata->tetu = 1;
  mydata->opinion_a = 1;
  mydata->opinion_b = 0;
  mydata->opinion_c = 0;
  mydata->opinion_d = 0;
  tableau_nbre_agents[mydata->flag_speaker-1] --;
  mydata->flag_speaker = 1;
  tableau_nbre_agents[mydata->flag_speaker-1] ++;
  mydata->quality_of_site = quality_of_site_a;
  set_color(RGB(1,0,0));
  consensus_site_a ++;
  affichage_resultats();
  //analyse_iteration_ideale();
}

void transition_tetu_b() {
  mydata->tetu = 1;
  mydata->opinion_a = 0;
  mydata->opinion_b = 1;
  mydata->opinion_c = 0;
  mydata->opinion_d = 0;
  tableau_nbre_agents[mydata->flag_speaker-1] --;
  mydata->flag_speaker = 2;
  tableau_nbre_agents[mydata->flag_speaker-1] ++;
  mydata->quality_of_site = quality_of_site_b;
  set_color(RGB(0,1,0));
  consensus_site_b ++;
  affichage_resultats();
  //analyse_iteration_ideale();
}

void transition_tetu_c() {
  mydata->tetu = 1;
  mydata->opinion_a = 0;
  mydata->opinion_b = 0;
  mydata->opinion_c = 1;
  mydata->opinion_d = 0;
  tableau_nbre_agents[mydata->flag_speaker-1] --;
  mydata->flag_speaker = 3;
  tableau_nbre_agents[mydata->flag_speaker-1] ++;
  mydata->quality_of_site = quality_of_site_c;
  set_color(RGB(0,0,1));
  consensus_site_c ++;
  affichage_resultats();
  //analyse_iteration_ideale();
}

void transition_tetu_d() {
  mydata->tetu = 1;
  mydata->opinion_a = 0;
  mydata->opinion_b = 0;
  mydata->opinion_c = 0;
  mydata->opinion_d = 1;
  tableau_nbre_agents[mydata->flag_speaker-1] --;
  mydata->flag_speaker = 4;
  tableau_nbre_agents[mydata->flag_speaker-1] ++;
  mydata->quality_of_site = quality_of_site_d;
  set_color(RGB(1,1,1));
  consensus_site_d ++;
  affichage_resultats();
  //analyse_iteration_ideale();
}

void transition_tetu() {
  if (mydata->changements_opinions == mydata->seuil_changements_opinions) {
    //affichage_debugage_0();

    /* ################################################ TROIS EGALITES ################################################ */
    if (mydata->changements_opinions_a == mydata->changements_opinions_b && mydata->changements_opinions_b == mydata->changements_opinions_c
      && mydata->changements_opinions_c == mydata->changements_opinions_d) {
      uint8_t temp = rand() % NBRE_OPTIONS;
      if(temp == 0) mydata->changements_opinions_a ++;
      else if(temp == 1) mydata->changements_opinions_b ++;
      else if(temp == 2) mydata->changements_opinions_c ++;
      else if(temp == 3) mydata->changements_opinions_d ++;
      else printf("ERREUR TRANSITION_TETU 1 \n");
    }

    /* ################################################ UNE EGALITE ################################################ */
    else if (mydata->changements_opinions_a == mydata->changements_opinions_b && mydata->changements_opinions_a > mydata->changements_opinions_c
      && mydata->changements_opinions_a > mydata->changements_opinions_d) {
      uint8_t temp = rand() % (NBRE_OPTIONS-2);
      if(temp == 0) mydata->changements_opinions_a ++;
      else if(temp == 1) mydata->changements_opinions_b ++;
      else printf("ERREUR TRANSITION_TETU 2 \n");
    }
    else if (mydata->changements_opinions_b == mydata->changements_opinions_c && mydata->changements_opinions_b > mydata->changements_opinions_a
      && mydata->changements_opinions_b > mydata->changements_opinions_d) {
      uint8_t temp = rand() % (NBRE_OPTIONS-2);
      if(temp == 0) mydata->changements_opinions_b ++;
      else if(temp == 1) mydata->changements_opinions_c ++;
      else printf("ERREUR TRANSITION_TETU 3 \n");
    }
    else if (mydata->changements_opinions_a == mydata->changements_opinions_c && mydata->changements_opinions_a > mydata->changements_opinions_b
      && mydata->changements_opinions_a > mydata->changements_opinions_d) {
      uint8_t temp = rand() % (NBRE_OPTIONS-2);
      if(temp == 0) mydata->changements_opinions_a ++;
      else if(temp == 1) mydata->changements_opinions_c ++;
      else printf("ERREUR TRANSITION_TETU 4 \n");
    }
    else if (mydata->changements_opinions_d == mydata->changements_opinions_a && mydata->changements_opinions_d > mydata->changements_opinions_b
      && mydata->changements_opinions_d > mydata->changements_opinions_c) {
      uint8_t temp = rand() % (NBRE_OPTIONS-2);
      if(temp == 0) mydata->changements_opinions_d ++;
      else if(temp == 1) mydata->changements_opinions_a ++;
      else printf("ERREUR TRANSITION_TETU 5 \n");
    }
    else if (mydata->changements_opinions_d == mydata->changements_opinions_b && mydata->changements_opinions_d > mydata->changements_opinions_a
      && mydata->changements_opinions_d > mydata->changements_opinions_c) {
      uint8_t temp = rand() % (NBRE_OPTIONS-2);
      if(temp == 0) mydata->changements_opinions_d ++;
      else if(temp == 1) mydata->changements_opinions_b ++;
      else printf("ERREUR TRANSITION_TETU 6 \n");
    }
    else if (mydata->changements_opinions_d == mydata->changements_opinions_c && mydata->changements_opinions_d > mydata->changements_opinions_a
      && mydata->changements_opinions_d > mydata->changements_opinions_b) {
      uint8_t temp = rand() % (NBRE_OPTIONS-2);
      if(temp == 0) mydata->changements_opinions_d ++;
      else if(temp == 1) mydata->changements_opinions_c ++;
      else printf("ERREUR TRANSITION_TETU 7 \n");
    }

    /* ################################################ DEUX EGALITES ################################################ */
    else if (mydata->changements_opinions_a == mydata->changements_opinions_b && mydata->changements_opinions_a == mydata->changements_opinions_c
      && mydata->changements_opinions_a > mydata->changements_opinions_d) {
      uint8_t temp = rand() % (NBRE_OPTIONS-1);
      if(temp == 0) mydata->changements_opinions_a ++;
      else if(temp == 1) mydata->changements_opinions_b ++;
      else if(temp == 2) mydata->changements_opinions_c ++;
      else printf("ERREUR TRANSITION_TETU 8 \n");
    }
    else if (mydata->changements_opinions_a == mydata->changements_opinions_b && mydata->changements_opinions_a == mydata->changements_opinions_d
      && mydata->changements_opinions_a > mydata->changements_opinions_c) {
      uint8_t temp = rand() % (NBRE_OPTIONS-1);
      if(temp == 0) mydata->changements_opinions_a ++;
      else if(temp == 1) mydata->changements_opinions_b ++;
      else if(temp == 2) mydata->changements_opinions_d ++;
      else printf("ERREUR TRANSITION_TETU 9 \n");
    }
    else if (mydata->changements_opinions_a == mydata->changements_opinions_c && mydata->changements_opinions_a == mydata->changements_opinions_d
      && mydata->changements_opinions_a > mydata->changements_opinions_b) {
      uint8_t temp = rand() % (NBRE_OPTIONS-1);
      if(temp == 0) mydata->changements_opinions_a ++;
      else if(temp == 1) mydata->changements_opinions_c ++;
      else if(temp == 2) mydata->changements_opinions_d ++;
      else printf("ERREUR TRANSITION_TETU 10 \n");
    }
    else if (mydata->changements_opinions_b == mydata->changements_opinions_c && mydata->changements_opinions_b == mydata->changements_opinions_d
      && mydata->changements_opinions_b > mydata->changements_opinions_a) {
      uint8_t temp = rand() % (NBRE_OPTIONS-1);
      if(temp == 0) mydata->changements_opinions_b ++;
      else if(temp == 1) mydata->changements_opinions_c ++;
      else if(temp == 2) mydata->changements_opinions_d ++;
      else printf("ERREUR TRANSITION_TETU 11 \n");
    }

    /* ################################################ ON DEPARTAGE ################################################ */
    if (mydata->changements_opinions_a > mydata->changements_opinions_b && mydata->changements_opinions_a > mydata->changements_opinions_c
      && mydata->changements_opinions_a > mydata->changements_opinions_d) {
      transition_tetu_a();
    }
    else if (mydata->changements_opinions_b > mydata->changements_opinions_a && mydata->changements_opinions_b > mydata->changements_opinions_c
      && mydata->changements_opinions_b > mydata->changements_opinions_d) {
      transition_tetu_b();
    }
    else if (mydata->changements_opinions_c > mydata->changements_opinions_a && mydata->changements_opinions_c > mydata->changements_opinions_b
      && mydata->changements_opinions_c > mydata->changements_opinions_d) {
      transition_tetu_c();
    }
    else if (mydata->changements_opinions_d > mydata->changements_opinions_a && mydata->changements_opinions_d > mydata->changements_opinions_b
      && mydata->changements_opinions_d > mydata->changements_opinions_c) {
      transition_tetu_d();
    }
    else printf("ERREUR TRANSITION_TETU 12 \n");
    //affichage_debugage_1();
  }
}

void set_opinion_a() {
  if (!mydata->tetu) {
    mydata->opinion_a = 1;
    mydata->opinion_b = 0;
    mydata->opinion_c = 0;
    mydata->opinion_d = 0;
    tableau_nbre_agents[mydata->flag_speaker-1] --;
    mydata->flag_speaker = 1;
    tableau_nbre_agents[mydata->flag_speaker-1] ++;
    mydata->quality_of_site = quality_of_site_a;
    set_color(RGB(2,0,0));
    mydata->changements_opinions_a ++;
    mydata->changements_opinions ++;
    if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_a += poids_changements_opinions;
    //if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_a -= poids_changements_opinions;
    if (mydata->changements_opinions == iteration_critique_decision) {
      //mydata->seuil_changements_opinions = seuil_min_changements_opinions * mydata->quality_of_site;
      //printf("flag_speaker : %d => %d\n", mydata->flag_speaker, kilo_uid);
      mydata->seuil_changements_opinions = seuil_min_changements_opinions / mydata->quality_of_site;
      if (mydata->seuil_changements_opinions < mydata->changements_opinions) {
        mydata->seuil_changements_opinions = mydata->changements_opinions;
      }
    }
    transition_tetu();
    //if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_a += poids_changements_opinions;
  }
}

void set_opinion_b() {
  if (!mydata->tetu) {
    mydata->opinion_a = 0;
    mydata->opinion_b = 1;
    mydata->opinion_c = 0;
    mydata->opinion_d = 0;
    tableau_nbre_agents[mydata->flag_speaker-1] --;
    mydata->flag_speaker = 2;
    tableau_nbre_agents[mydata->flag_speaker-1] ++;
    mydata->quality_of_site = quality_of_site_b;
    set_color(RGB(0,2,0));
    mydata->changements_opinions_b ++;
    mydata->changements_opinions ++;
    if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_b += poids_changements_opinions;
    //if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_b -= poids_changements_opinions;
    if (mydata->changements_opinions == iteration_critique_decision) {
      //mydata->seuil_changements_opinions = seuil_min_changements_opinions * mydata->quality_of_site;
      //printf("flag_speaker : %d => %d\n", mydata->flag_speaker, kilo_uid);
      mydata->seuil_changements_opinions = seuil_min_changements_opinions / mydata->quality_of_site;
      if (mydata->seuil_changements_opinions < mydata->changements_opinions) {
        mydata->seuil_changements_opinions = mydata->changements_opinions;
      }
    }
    transition_tetu();
    //if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_b += poids_changements_opinions;
  }
}

void set_opinion_c() {
  if (!mydata->tetu) {
    mydata->opinion_a = 0;
    mydata->opinion_b = 0;
    mydata->opinion_c = 1;
    mydata->opinion_d = 0;
    tableau_nbre_agents[mydata->flag_speaker-1] --;
    mydata->flag_speaker = 3;
    tableau_nbre_agents[mydata->flag_speaker-1] ++;
    mydata->quality_of_site = quality_of_site_c;
    set_color(RGB(0,0,2));
    mydata->changements_opinions_c ++;
    mydata->changements_opinions ++;
    if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_c += poids_changements_opinions;
    //if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_c -= poids_changements_opinions;
    if (mydata->changements_opinions == iteration_critique_decision) {
      //mydata->seuil_changements_opinions = seuil_min_changements_opinions * mydata->quality_of_site;
      //printf("flag_speaker : %d => %d\n", mydata->flag_speaker, kilo_uid);
      mydata->seuil_changements_opinions = seuil_min_changements_opinions / mydata->quality_of_site;
      if (mydata->seuil_changements_opinions < mydata->changements_opinions) {
        mydata->seuil_changements_opinions = mydata->changements_opinions;
      }
    }
    transition_tetu();
    //if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_c += poids_changements_opinions;
  }
}

void set_opinion_d() {
  if (!mydata->tetu) {
    mydata->opinion_a = 0;
    mydata->opinion_b = 0;
    mydata->opinion_c = 0;
    mydata->opinion_d = 1;
    tableau_nbre_agents[mydata->flag_speaker-1] --;
    mydata->flag_speaker = 4;
    tableau_nbre_agents[mydata->flag_speaker-1] ++;
    mydata->quality_of_site = quality_of_site_d;
    set_color(RGB(2,2,2));
    mydata->changements_opinions_d ++;
    mydata->changements_opinions ++;
    if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_d += poids_changements_opinions;
    //if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_d -= poids_changements_opinions;
    if (mydata->changements_opinions == iteration_critique_decision) {
      //mydata->seuil_changements_opinions = seuil_min_changements_opinions * mydata->quality_of_site;
      //printf("flag_speaker : %d => %d\n", mydata->flag_speaker, kilo_uid);
      mydata->seuil_changements_opinions = seuil_min_changements_opinions / mydata->quality_of_site;
      if (mydata->seuil_changements_opinions < mydata->changements_opinions) {
        mydata->seuil_changements_opinions = mydata->changements_opinions;
      }
    }
    transition_tetu();
    //if (mydata->changements_opinions == iteration_critique_changements_opinions) mydata->changements_opinions_d += poids_changements_opinions;
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
        double proba_c;
        double proba_d;
        double tirage;
        total = mydata->opinion_a + mydata->opinion_b + mydata->opinion_c + mydata->opinion_d;
        proba_a = mydata->opinion_a / total;
        proba_b = mydata->opinion_b / total;
        proba_c = mydata->opinion_c / total;
        proba_d = mydata->opinion_d / total;
        tirage = frand_a_b(0.0,1.0);
        if (tirage <= proba_a) set_opinion_a();
        else if (tirage <= proba_a + proba_b) set_opinion_b();
        else if (tirage <= proba_a + proba_b + proba_c) set_opinion_c();
        else if (tirage <= proba_a + proba_b + proba_c + proba_d) set_opinion_d();
        else printf("ERREUR VOTER_MODEL 1 \n"); // peut-être souci de nest trop petit qui provoque des NaN
        vider_tableau_uid();
      }
      else {// agent têtu
        mydata->state = EXPLORATION;
        explore();
        if (mydata->flag_speaker == 1) mydata->quality_of_site = quality_of_site_a;
        else if (mydata->flag_speaker == 2) mydata->quality_of_site = quality_of_site_b;
        else if (mydata->flag_speaker == 3) mydata->quality_of_site = quality_of_site_c;
        else if (mydata->flag_speaker == 4) mydata->quality_of_site = quality_of_site_d;
        else printf("ERREUR VOTER_MODEL 2 \n");
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
    
    //mydata->flag_nest = 1;
    //mydata->stepTicks = kilo_ticks;
}

void loop() {

  //if(kilo_ticks % intervalle_temps_kiloticks == 0) {
  if(kilo_uid == 0 && (kilo_ticks % intervalle_temps_kiloticks == 0 || kilo_ticks % intervalle_temps_kiloticks == 1) && kilo_ticks != 1) {
    
    qualite_consensus_a[mydata->indice_temps] = tableau_nbre_agents[0];
    qualite_consensus_b[mydata->indice_temps] = tableau_nbre_agents[1];
    qualite_consensus_c[mydata->indice_temps] = tableau_nbre_agents[2];
    qualite_consensus_d[mydata->indice_temps] = tableau_nbre_agents[3];
    //qualite_consensus_global[mydata->indice_temps] = (qualite_consensus_a[mydata->indice_temps] + qualite_consensus_b[mydata->indice_temps] + qualite_consensus_c[mydata->indice_temps] + qualite_consensus_d[mydata->indice_temps]) / NBRE_OPTIONS;
    
    //printf("mydata->indice_temps = %d \n", mydata->indice_temps);
    mydata->indice_temps ++;
    dernier_indice_temps = mydata->indice_temps;

    //printf("%d\n",seuil_max_changements_opinions);
    
    printf("qualite_consensus_a = %d\n", qualite_consensus_a[mydata->indice_temps-1]);
    printf("qualite_consensus_b = %d\n", qualite_consensus_b[mydata->indice_temps-1]);
    printf("qualite_consensus_c = %d\n", qualite_consensus_c[mydata->indice_temps-1]);
    printf("qualite_consensus_d = %d\n", qualite_consensus_d[mydata->indice_temps-1]);
    //printf("qualite_consensus_global = %d\n", qualite_consensus_global[mydata->indice_temps-1]);
    uint16_t min = 0;
    printf("Kiloticks du consensus : %d \n", kilo_ticks);
    min = kilo_ticks / 31 / 60;
    printf("Temps en min du consensus : %d \n", min);
    
  }
  /*
  else if (kilo_uid==0 && kilo_ticks == 1292) {
   //printf("kilo_ticks modulo intervalle_temps_kiloticks == %d\n", kilo_ticks % intervalle_temps_kiloticks);
   printf("kiloticks : %d\n", kilo_ticks);
  }
  */

  if(mydata->state == DISSEMINATION) {
    nest();
    if (mydata->new_message) {
      mydata->new_message = 0; // On remet le flag à O
      collisions();
      //nest();
      if(mydata->flag_nest && mydata->tetu == 0) {     
        if (mydata->flag_listener == 1 && !mydata->flag_voisin_deja_rencontre) {
          mydata->opinion_a ++;
          mydata->cpt_voisins ++;
        }            
        else if (mydata->flag_listener == 2 && !mydata->flag_voisin_deja_rencontre) {
          mydata->opinion_b ++;
          mydata->cpt_voisins ++;
        }
        else if (mydata->flag_listener == 3 && !mydata->flag_voisin_deja_rencontre) {
          mydata->opinion_c ++;
          mydata->cpt_voisins ++;
        }
        else if (mydata->flag_listener == 4 && !mydata->flag_voisin_deja_rencontre) {
          mydata->opinion_d ++;
          mydata->cpt_voisins ++;
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
  else printf("ERREUR LOOP %d \n",mydata->state);
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

  mydata->message_c.type = NORMAL;
  mydata->message_c.data[0] = 3;
  mydata->message_c.data[1] = kilo_uid;
  mydata->message_c.crc = message_crc(&mydata->message_c);

  mydata->message_d.type = NORMAL;
  mydata->message_d.data[0] = 4;
  mydata->message_d.data[1] = kilo_uid;
  mydata->message_d.crc = message_crc(&mydata->message_d);

  mydata->message_exploration.type = NORMAL;
	mydata->message_exploration.data[0] = 0;
  mydata->message_exploration.data[1] = kilo_uid;
	mydata->message_exploration.crc = message_crc(&mydata->message_exploration);
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
    else if (mydata->flag_speaker == 3) return &mydata->message_c;
    else if (mydata->flag_speaker == 4) return &mydata->message_d;
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

  setup_message();
  setup_light();

  /* VOTER MODEL */

  if (kilo_uid % NBRE_OPTIONS == 0) {
    /* OPINION A*/
    set_color(RGB(3,0,0));
    mydata->opinion_a = 0;
    mydata->opinion_b = 0;
    mydata->opinion_c = 0;
    mydata->opinion_d = 0;
    mydata->flag_speaker = 1;
  }
  else if (kilo_uid % NBRE_OPTIONS == 1) {
    /* OPINION B */
    set_color(RGB(0,3,0));
    mydata->opinion_a = 0;
    mydata->opinion_b = 0;
    mydata->opinion_c = 0;
    mydata->opinion_d = 0;
    mydata->flag_speaker = 2;
  }
  else if (kilo_uid % NBRE_OPTIONS == 2) {
    /* OPINION C */
    set_color(RGB(0,0,3));
    mydata->opinion_a = 0;
    mydata->opinion_b = 0;
    mydata->opinion_c = 0;
    mydata->opinion_d = 0;
    mydata->flag_speaker = 3;
  }
  else if (kilo_uid % NBRE_OPTIONS == 3) {
    /* OPINION D */
    set_color(RGB(3,3,3));
    mydata->opinion_a = 0;
    mydata->opinion_b = 0;
    mydata->opinion_c = 0;
    mydata->opinion_d = 0;
    mydata->flag_speaker = 4;
  }
  else printf("ERREUR SETUP VOTER_MODEL\n");

  /* AGENT TÊTU */

  /* seuil du nombre de changements d'avis avant de devenir un agent têtu */
  mydata->seuil_changements_opinions = seuil_max_changements_opinions;
  mydata->changements_opinions = 0;
  mydata->changements_opinions_a = 0;
  mydata->changements_opinions_b = 0;
  mydata->changements_opinions_c = 0;
  mydata->changements_opinions_d = 0;
  mydata->tetu = 0;

  /* AGENTS TETUS DEPUIS LE DEBUT */
  if (kilo_uid < nbre_initial_agents_tetus) {
    if (kilo_uid % NBRE_OPTIONS == 0) {
      mydata->tetu = 1;
      mydata->opinion_a = 1;
      mydata->opinion_b = 0;
      mydata->opinion_c = 0;
      mydata->opinion_d = 0;
      mydata->flag_speaker = 1;
      set_color(RGB(1,0,0));
    }
    else if (kilo_uid % NBRE_OPTIONS == 1) {
      mydata->tetu = 1;
      mydata->opinion_a = 0;
      mydata->opinion_b = 1;
      mydata->opinion_c = 0;
      mydata->opinion_d = 0;
      mydata->flag_speaker = 2;
      set_color(RGB(0,1,0));
    }
    else if (kilo_uid % NBRE_OPTIONS == 2) {
      mydata->tetu = 1;
      mydata->opinion_a = 0;
      mydata->opinion_b = 0;
      mydata->opinion_c = 1;
      mydata->opinion_d = 0;
      mydata->flag_speaker = 3;
      set_color(RGB(0,0,1));
    }
    else if (kilo_uid % NBRE_OPTIONS == 3) {
      mydata->tetu = 1;
      mydata->opinion_a = 0;
      mydata->opinion_b = 0;
      mydata->opinion_c = 0;
      mydata->opinion_d = 1;
      mydata->flag_speaker = 4;
      set_color(RGB(1,1,1));
    }
  }
  mydata->flag_bug = 0;

  mydata->indice_temps = 0;
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
  double proba_c;
  double proba_d;
  total = mydata->opinion_a + mydata->opinion_b + mydata->opinion_c + mydata->opinion_d;
  proba_a = mydata->opinion_a / total;
  proba_b = mydata->opinion_b / total;
  proba_c = mydata->opinion_c / total;
  proba_d = mydata->opinion_d / total;
  
  if (mydata->state == DISSEMINATION) {
    if (!mydata->tetu) {
      p += sprintf (p, "Opinion a: %f ", proba_a);
      p += sprintf (p, "Opinion b: %f ", proba_b);
      p += sprintf (p, "Opinion c: %f ", proba_c);
    	p += sprintf (p, "Opinion d: %f \n", proba_d);
      p += sprintf (p, "cpt voisins : %d \n", mydata->cpt_voisins);
      p += sprintf (p, "Changements d'opinions : ");
      p += sprintf (p, "au total : %d, ", mydata->changements_opinions);
      p += sprintf (p, "en a : %d, ", mydata->changements_opinions_a);
      p += sprintf (p, "en b : %d, ", mydata->changements_opinions_b);
      p += sprintf (p, "en c : %d, ", mydata->changements_opinions_c);
      p += sprintf (p, "en d : %d. \n", mydata->changements_opinions_d);
    }
    else {
      p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
      p += sprintf (p, "Changements d'opinions : ");
      p += sprintf (p, "au total : %d, ", mydata->changements_opinions);
      p += sprintf (p, "en a : %d, ", mydata->changements_opinions_a);
      p += sprintf (p, "en b : %d, ", mydata->changements_opinions_b);
      p += sprintf (p, "en c : %d, ", mydata->changements_opinions_c);
      p += sprintf (p, "en d : %d. \n", mydata->changements_opinions_d);
    }
  }
  else if (mydata->state == EXPLORATION) {
    p += sprintf (p, "Opinion : %d \n", mydata->flag_speaker);
    p += sprintf (p, "Changements d'opinions : ");
    p += sprintf (p, "au total : %d, ", mydata->changements_opinions);
    p += sprintf (p, "en a : %d, ", mydata->changements_opinions_a);
    p += sprintf (p, "en b : %d, ", mydata->changements_opinions_b);
    p += sprintf (p, "en c : %d, ", mydata->changements_opinions_c);
    p += sprintf (p, "en d : %d. \n", mydata->changements_opinions_d);
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
      double light_y = 500.0;
      double dist_x = pow(light_x + x, 2);
      double dist_y = pow(light_y + y, 2);
      double dist_c = sqrt(dist_x + dist_y);
      return (int16_t)dist_c;
    }
    else if (mydata->flag_speaker == 2) { // aller vers le site b
      double light_x = -600.0;
      double light_y = 500.0;
      double dist_x = pow(light_x + x, 2);
      double dist_y = pow(light_y + y, 2);
      double dist_c = sqrt(dist_x + dist_y);
      return (int16_t)dist_c;
    }
    else if (mydata->flag_speaker == 3) { // aller vers le site c
      double light_x = 600.0;
      double light_y = -500.0;
      double dist_x = pow(light_x + x, 2);
      double dist_y = pow(light_y + y, 2);
      double dist_c = sqrt(dist_x + dist_y);
      return (int16_t)dist_c;
    }
    else if (mydata->flag_speaker == 4) { // aller vers le site d
      double light_x = -600.0;
      double light_y = -500.0;
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