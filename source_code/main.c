#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"

#include "periph/gpio.h"
#include "xtimer.h"
#include "thread.h"

#include "cpu.h"
#include "board.h"
#include "main.h"


//MQTTS
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 1)
#define _IPV6_DEFAULT_PREFIX_LEN        (64U)
#define BROKER_PORT         1885
#define BROKER_ADDRESS      "fec0:affe::1"
#define NUMOFSUBS           (16U)
#define TOPIC_MAXLEN        (64U)
#define TOPIC_IN            "topic_1"
#define TOPIC_OUT           "topic_2"

// Controlling macros 
#define RED_ZONE 5
#define YELLOW_ZONE 15
#define RELAY_ON 1
#define RELAY_OFF 0
#define RED_COLOR 0
#define YELLOW_COLOR 1
#define GREEN_COLOR 2


// Threads
static char relay_stack[THREAD_STACKSIZE_DEFAULT];

//to add addresses to board interface
extern int _gnrc_netif_config(int argc, char **argv);

//MQTTS
static char stack[THREAD_STACKSIZE_DEFAULT];
static msg_t queue[8];

static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];

// variables to handle message sending
char msg[10];
bool message_to_send = false;

// messages to handle the message receiving
bool system_on = true;
char my_com ;


// water pump and relay
gpio_t pin_relay = GPIO_PIN(PORT_B, 5); //D4
int relay_state = 1;
uint32_t turn_on_time; 
uint32_t total_time_on;


//traffic light
gpio_t red_pin = GPIO_PIN(PORT_A, 10); //D2
gpio_t yellow_pin = GPIO_PIN(PORT_B, 3); //D3
gpio_t green_pin = GPIO_PIN(PORT_A, 6); //D12 

//HY-SRF05 ultrasonic
gpio_t trigger_pin = GPIO_PIN(PORT_A, 9); //D8
gpio_t echo_pin = GPIO_PIN(PORT_A, 8); //D7
uint32_t echo_time_start;
uint32_t echo_time;
bool sensor_on = false;


//MQTTS *****************************
static void *emcute_thread(void *arg){
    (void)arg;
    emcute_run(BROKER_PORT, "board");
    return NULL;
}

static int pub(char* topic,char* msg){
    emcute_topic_t t;
    unsigned flags = EMCUTE_QOS_0;

    t.name = topic;
    if (emcute_reg(&t) != EMCUTE_OK) {
        puts("error: unable to obtain topic ID");
        return 1;
    }

    if (emcute_pub(&t, msg, strlen(msg), flags) != EMCUTE_OK) {
        printf("error: unable to publish data to topic '%s [%i]'\n",t.name, (int)t.id);
        return 1;
    }

    return 0;
}

static void on_pub(const emcute_topic_t *topic, void *data, size_t len){
    (void)topic;

    char *in = (char *)data;
    if (strcmp(in,"Start")){
        system_on = true;
    }
    else if (strcmp(in,"Stop")){
        system_on = false;
    }
    else {
        printf("Received command not correct\n");
    }
    printf("\n");

}

static int sub(char* topic){
    unsigned flags = EMCUTE_QOS_0;

    if (strlen(topic) > TOPIC_MAXLEN) {
        puts("error: topic name exceeds maximum possible size");
        return 1;
    }

    /* find empty subscription slot */
    unsigned i = 0;
    for (; (i < NUMOFSUBS) && (subscriptions[i].topic.id != 0); i++) {}
    if (i == NUMOFSUBS) {
        puts("error: no memory to store new subscriptions");
        return 1;
    }

    subscriptions[i].cb = on_pub;
    strcpy(topics[i], topic);
    subscriptions[i].topic.name = topics[i];
    if (emcute_sub(&subscriptions[i], flags) != EMCUTE_OK) {
        printf("error: unable to subscribe to %s\n", topic);
        return 1;
    }

    return 0;
}

static int con(void){
    sock_udp_ep_t gw = { .family = AF_INET6, .port = BROKER_PORT };
    char *topic = NULL;
    char *message = NULL;
    size_t len = 0;

    ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, BROKER_ADDRESS);

    if (emcute_con(&gw, true, topic, message, len, 0) != EMCUTE_OK) {
        printf("error: unable to connect to [%s]:%i\n", BROKER_ADDRESS, (int)gw.port);
        return 1;
    }
    return 0;
}

static int add_address(char* addr){
    char * arg[] = {"ifconfig", "4", "add", addr};
    return _gnrc_netif_config(4, arg);
}

//initializes the connection with the MQTT-SN broker
void mqtts_init(void){

    /* the main thread needs a msg queue to be able to run `ping`*/
    msg_init_queue(queue, ARRAY_SIZE(queue));

    /* initialize our subscription buffers */
    memset(subscriptions, 0, (NUMOFSUBS * sizeof(emcute_sub_t)));

    /* start the emcute thread */+
    thread_create(stack, sizeof(stack), EMCUTE_PRIO, 0, emcute_thread, NULL, "emcute");

    char * addr1 = "fec0:affe::99";
    add_address(addr1);

    con();
    sub(TOPIC_IN); 
}
//***********************

//Sensors & Actuators *********************

// ultrasonic echo time
void ultrasonic_time(void* arg){
    (void) arg;
    int val = gpio_read(echo_pin);
	uint32_t echo_time_stop;

    if(val){
		echo_time_start = xtimer_now_usec();
	}
    else{
		echo_time_stop = xtimer_now_usec();
		echo_time = echo_time_stop - echo_time_start;
        
	}
}

// ultrasonic distance measure
int ultrasonic_distance(void){ 

	uint32_t dist;
    dist=0;
    echo_time = 0;

    gpio_clear(trigger_pin);
    xtimer_usleep(20); 
    gpio_set(trigger_pin);

    xtimer_msleep(30); 

    if(echo_time > 0){
        dist = echo_time/58; 
    }
    return dist;
}

void set_lights(int lights_state){

    if (lights_state == GREEN_COLOR){

        gpio_clear(red_pin);
        gpio_clear(yellow_pin);
        gpio_set(green_pin);
    }
    else if (lights_state == YELLOW_COLOR){
        gpio_clear(red_pin);
        gpio_clear(green_pin);
        gpio_set(yellow_pin);
    }
    else if (lights_state == RED_COLOR){
        gpio_clear(green_pin);
        gpio_clear(yellow_pin);
        gpio_set(red_pin);
    }
    else {print("Lights command not found\n");}

    return; 
}

void* set_relay(void* arg){
    (void) arg;
    
    while(true){
        if(relay_state == RELAY_ON  && sensor_on == false){
            gpio_clear(pin_relay);
        }
        else if (relay_state == RELAY_OFF || sensor_on == true){
            gpio_set(pin_relay);
        }
        xtimer_msleep(50);
    }
    return NULL;
}

void sensor_init(void){

    // water pump and relay
    gpio_init(pin_relay, GPIO_OUT);

    //traffic light
    gpio_init(red_pin, GPIO_OUT);
    gpio_init(yellow_pin, GPIO_OUT);
    gpio_init(green_pin, GPIO_OUT);

    //ultrasonic
    gpio_init(trigger_pin, GPIO_OUT);
    gpio_init_int(echo_pin, GPIO_IN, GPIO_BOTH, &ultrasonic_time, NULL);
    ultrasonic_distance(); //first read returns always 0

}
//***********************

//******** coloured text **********
void red(){
    printf("\033[1;31m");
}
void yellow(){
    printf("\033[1;33m");
}
void green(){
    printf("\033[1;32m");
}
void blue(){
    printf("\033[1;34m");
}
void reset(){
    printf("\033[0m");
}
//***********

int main(void){    
    
    mqtts_init();
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Here2Serve\n\n\n\n");

    sensor_init();

    thread_create(relay_stack,sizeof(relay_stack),THREAD_PRIORITY_MAIN -1, 0,set_relay, NULL, "relay_logic");
    
    int distance = 100;
    xtimer_msleep(100);

	while(true){

        if (system_on == false){
            printf("SYSTEM OFF\n");
            relay_state = RELAY_OFF;
            set_lights(GREEN_COLOR);

            if (message_to_send == true){
                uint32_t turn_off_time = xtimer_now_usec();
                total_time_on = turn_off_time - turn_on_time;
                uint32_t seconds = total_time_on / 1000000;
                sprintf(msg, "%ld", total_time_on);

                blue();
                printf("Usage time: %ld sec\n",seconds);
                reset();

                pub(TOPIC_OUT, msg);
                message_to_send = false;
            }
        }
        else {
            sensor_on = true;
            xtimer_msleep(60);
            distance = ultrasonic_distance();

            blue();
            printf("Distance: %ld\n",distance);
            reset();

            if (distance <= RED_ZONE){
                red();
                printf("Red zone\n");
                reset();
                relay_state = RELAY_ON;
                set_lights(RED_COLOR);
                if (message_to_send == false){
                    turn_on_time = xtimer_now_usec();
                    message_to_send = true;
                }
            
            }
            else if (distance <= YELLOW_ZONE){
                yellow();
                printf("Yellow zone\n");
                reset();
                relay_state = RELAY_OFF;
                set_lights(YELLOW_COLOR);

                if (message_to_send == true){
                    uint32_t turn_off_time = xtimer_now_usec();
                    total_time_on = turn_off_time - turn_on_time;
                    uint32_t seconds = total_time_on / 1000000;
                    sprintf(msg, "%ld", total_time_on);

                    blue();
                    printf("Usage time: %ld sec\n",seconds);
                    reset();

                    pub(TOPIC_OUT, msg);
                    message_to_send = false;
                }
            
            }
            else {
                green();
                printf("Green zone\n");
                reset();
                relay_state = RELAY_OFF;
                set_lights(GREEN_COLOR);

                if (message_to_send == true){
                    uint32_t turn_off_time = xtimer_now_usec();
                    total_time_on = turn_off_time - turn_on_time;
                    uint32_t seconds = total_time_on / 1000000;
                    sprintf(msg, "%ld", total_time_on);

                    blue();
                    printf("Usage time: %ld sec\n",seconds);
                    reset();

                    pub(TOPIC_OUT, msg);
                    message_to_send = false;
                }
            
            }
            sensor_on = false;
        }
        xtimer_msleep(1000);
	}
    return 0;
}

