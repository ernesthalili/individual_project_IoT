#ifndef MAIN_FUNCTIONS
#define MAIN_FUNCTIONS


static void *emcute_thread(void *arg);
static int pub(char* topic,char* msg);
static void on_pub(const emcute_topic_t *topic, void *data, size_t len);
static int sub(char* topic);
static int con(void);
static int add_address(char* addr);
void mqtts_init(void);
void ultrasonic_time(void* arg);
int ultrasonic_distance(void);
void set_lights(int lights_state);
void* set_relay(void* arg);
void sensor_init(void);

// color text
void red();
void yellow();
void green();
void blue();
void reset();

#endif // MAIN_FUNCTIONS