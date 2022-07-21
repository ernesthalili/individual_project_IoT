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


//to add addresses to board interface
extern int _gnrc_netif_config(int argc, char **argv);

//MQTTS
static char stack[THREAD_STACKSIZE_DEFAULT];
static msg_t queue[8];

static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];

// water pump and relay
gpio_t pin_relay = GPIO_PIN(PORT_B, 5); //D4
uint32_t turn_on_time; 
uint32_t total_time_on;


//traffic light
gpio_t red_pin = GPIO_PIN(PORT_A, 10); //D2
gpio_t yellow_pin = GPIO_PIN(PORT_B, 5); //D4 
gpio_t green_pin = GPIO_PIN(PORT_A, 6); //D12 

//HY-SRF05 ultrasonic
gpio_t trigger_pin = GPIO_PIN(PORT_A, 9); //D8
gpio_t echo_pin = GPIO_PIN(PORT_A, 8); //D7
uint32_t echo_time_start;
uint32_t echo_time;


//MQTTS *****************************
static void *emcute_thread(void *arg){
    (void)arg;
    emcute_run(BROKER_PORT, "board");
    return NULL;
}


static int pub(char* topic,char* msg){
    emcute_topic_t t;
    unsigned flags = EMCUTE_QOS_0;

    printf("pub with topic: %s and name %s and flags 0x%02x\n", topic, msg, (int)flags);

    /* step 1: get topic id */
    t.name = topic;
    if (emcute_reg(&t) != EMCUTE_OK) {
        puts("error: unable to obtain topic ID");
        return 1;
    }

    /* step 2: publish data */
    if (emcute_pub(&t, msg, strlen(msg), flags) != EMCUTE_OK) {
        printf("error: unable to publish data to topic '%s [%i]'\n",t.name, (int)t.id);
        return 1;
    }

    printf("Published %i bytes to topic '%s [%i]'\n", (int)strlen(msg), t.name, t.id);

    return 0;
}

static void on_pub(const emcute_topic_t *topic, void *data, size_t len){
    (void)topic;

    char *in = (char *)data;
    printf("### got publication for topic '%s' [%i] ###\n", topic->name, (int)topic->id);
    // print the message received
    for (size_t i = 0; i < len; i++) {
        printf("%c", in[i]);
    }
    puts("");
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

    printf("Now subscribed to %s\n", topic);
    

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
    printf("Successfully connected ");

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
        dist = echo_time/58; //from datasheet
    }
    return dist;
}


void set_relay(int relay_state){

    if(relay_state == RELAY_ON){
        printf("Turn-on water pump\n");
        gpio_clear(pin_relay);

        turn_on_time = xtimer_now_usec();

        // turn on the pump and start the time counter
    }
    else if (relay_state == RELAY_OFF){
        printf("Turn-off water pump\n");
        gpio_set(pin_relay);

        turn_off_time = xtimer_now_usec();
        total_time_on = turn_off_time-turn_on_time;

        sprintf(msg, "%d", total_time_on);

        //pub(TOPIC_OUT, msg);
        
        // turn off the pump and deliver the time it was on
    }

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

int main(void){    
    
    //mqtts_init();
    sensor_init();
    int state = 0; 
    int distance = 0; 
	
	while(true){
        distance = ultrasonic_distance();

        if (distance <= RED_ZONE){
            set_relay(RELAY_ON);
            set_lights(RED_COLOR);
        }
        else if (distance <= YELLOW_ZONE){
            set_relay(RELAY_OFF);
            set_lights(YELLOW_COLOR);
        }
        else{
            set_relay(RELAY_OFF);
            set_lights(GREEN_COLOR);
        }

        xtimer_sleep(5);
	}
    return 0;
}

