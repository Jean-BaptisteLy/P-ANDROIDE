#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884197169399375105820974944
#endif

typedef enum {
	STOP,
	FORWARD,
	LEFT,
	RIGHT
} motion_t;

typedef enum {
	NEIGHBOR_TOOCLOSE,
	ALONE,
} collision_state_t;

typedef struct {

	collision_state_t collision_state; 
	uint8_t neighbor_distance;
    motion_t direction;

    uint8_t msgID;

    uint8_t new_message;
  	distance_measurement_t dist;
  	message_t transmit_msg;

} USERDATA;

extern USERDATA *mydata;