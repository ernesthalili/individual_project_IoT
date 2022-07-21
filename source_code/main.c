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
#define TOPIC_OUT1          "topic_2"




//to add addresses to board interface
extern int _gnrc_netif_config(int argc, char **argv);

//MQTTS
static char stack[THREAD_STACKSIZE_DEFAULT];
static msg_t queue[8];

static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];

//traffic light
gpio_t red_pin = GPIO_PIN(PORT_A, 10); //D2
gpio_t yellow_pin = GPIO_PIN(PORT_B, 5); //D4 
gpio_t green_pin = GPIO_PIN(PORT_A, 6); //D12 



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
void sensor_init(void){

    //traffic light
    gpio_init(red_pin, GPIO_OUT);
    gpio_init(yellow_pin, GPIO_OUT);
    gpio_init(green_pin, GPIO_OUT);





}


//***********************

int main(void){    
    
    //mqtts_init();
	
	while(true){
       
        xtimer_sleep(5);
	}
    return 0;
}

