#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884197169399375105820974944
#endif

typedef enum {
	STOP,
	FORWARD,
	LEFT,
	RIGHT
} motion_t;

/*
typedef enum {
	NEIGHBOR_TOOCLOSE,
	ALONE,
} collision_state_t;
*/

typedef enum {
	DISSEMINATION,
	EXPLORATION,
} state_t;

typedef struct {

	uint8_t quality_of_site;
	uint32_t dissemination_time;
	uint32_t exploration_time;

	// time
	uint32_t last_time_update;

	state_t state;
	//int tab_uid*;
	uint8_t tab_uid[100];
	uint8_t cpt_voisins;
	uint8_t flag_voisin_deja_rencontre;

	//collision_state_t collision_state; 
	uint8_t neighbor_distance;
    motion_t direction;

    uint8_t msgID;

    /* SPEAKER */
    uint8_t new_message;
  	distance_measurement_t dist;
  	//message_t transmit_msg;

  	uint8_t flag_a_speaker;
	message_t message_a;
	message_t message_b;


  	/* LISTENER */
  	uint8_t flag_a_listener;
  	message_t rcvd_message;

  	/* Majority Rule */
  	//uint8_t my_opinion;
  	uint8_t opinion_a;
  	uint8_t opinion_b;

  	uint8_t uid;

} USERDATA;

extern USERDATA *mydata;