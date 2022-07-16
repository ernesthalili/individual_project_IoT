import paho.mqtt.client as mqtt
from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import json
import paho.mqtt.publish as publish


TOPIC_IN = "topic_in"
TOPIC_OUT1 = "topic_out"

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe(TOPIC_OUT1)

def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))
    message = {}
    message['message'] = str(msg.payload)[2:-1]
    messageJson = json.dumps(message)
    myAWSIoTMQTTClient.publish(msg.topic, messageJson, 1)

def customCallback(client, userdata, message):
    jsonmessage = json.loads(message.payload)
    publish.single(TOPIC_IN, jsonmessage["message"], hostname="localhost", port=1886, client_id="2")

host = "al9n4keue2wuo-ats.iot.eu-west-1.amazonaws.com"
rootCAPath = "root-CA.crt"
certificatePath = "device_individual.cert.pem"
privateKeyPath = "device_individual.private.key"
clientId = "ernest"
port=8883

myAWSIoTMQTTClient = None
myAWSIoTMQTTClient = AWSIoTMQTTClient(clientId)
myAWSIoTMQTTClient.configureEndpoint(host, port)
myAWSIoTMQTTClient.configureCredentials(rootCAPath, privateKeyPath, certificatePath)
myAWSIoTMQTTClient.connect()
print("Hello\n")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

myAWSIoTMQTTClient.subscribe(TOPIC_IN, 1, customCallback)

client.connect("localhost", 1886, 60)

client.loop_forever()