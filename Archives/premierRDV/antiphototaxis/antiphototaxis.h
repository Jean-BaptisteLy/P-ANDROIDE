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
	BETTER,
	WORSE,
	NONE,
	SUCCESS,
} search_state_t;

typedef struct {
    uint32_t intensity;
    uint32_t lastIntensity;
    search_state_t search_state;
    uint32_t lastTicks;
	uint32_t stepTicks;
} USERDATA;

extern USERDATA *mydata;