# Here2Serve

This is the individual project for the Internet of Things course of "Sapienza" university of Rome, held during the accademic year 2021-2022. 
In a lot of situations, especially in the hot days, there is the need for a glass of cold water. It might require a bit of time and effort to pour water on it. So, a system that assists on pouring water when it is requested be very useful. There are a lot of systems that pour water when pressing a button. However, since this system can be used in public places and having a lot of people that press this buttonm may increase the risk. So it would be perfect to have a system that puor water and have the possibility to do that without even needing to press some buttons. That is why the system I am presenting here is created in such a way that is satisfies all the these requirements. 
It uses an ultrasonic sensor to measure the distance from the closest object/person and puors water when the distance is below a certain level.

## Architecture
In this section there are going to be shown the physical elements of the system, the sensors and the actuators. 

### Sensors
- **Ultrasonic sensor SRF05(da confermare)**
The sensor we are using here, as we said before, is an ultrasonic sensor. It measures the smallest distance of the objects from the sensor. The approach that this sensor uses to measure the smallest distance is by sending a trigger signal ans receiving an echo signal. [visit datasheet for further information]
The system considers this distance and when needed, changes the state of the system. More specifically, if this distance is larger than 15 cm, the system doesn't react since it is considered that there is no intention o using the system if the system is located that far. Instead if the distance is between 5-15 cm, there may be intention to use the system, but still we have add the possibility to notify the user that if it comes any further, the state of the system may change. Finally, if the smallest object/person is located not further than 5 cm, the system changes its state and activate an relay.

### Actuators
- **Mini semaphore**
It has three LEDs: red, yellow, and green. They are used to provide feedback on the distance measured by the ultrasonic sensor. Initialially the semaphore has the green light on. This remain like this if after a measurement, the distance measured is not less than 15 cm. If this happen and the disntace is between 5-15 cm, the semaphore turn off the green light and turn on the yellow, notifying this was the change of the system state. Instead, if the distance is not more than 5 cm, the red light is the only one which is turned on.

- **1 channel relay**
This relay has the only purpose the activate the water pump. It is connected to the 3,3v, ground and an activation pin. This pin is activated when the distance measured is not more than 5 cm. When this happen, to the pin where the water pump is connected now has a potential of 5v. 

- **Water pump**
As we said before, the water pump is activated only in the Red Zone state of the system. It is used to puor water into a glass or other possible containers. After every execution of the pump, there is also taen under consideration the amount of time is has been on, sending than this information into a dashboard. There might be than different calculations considering that the pump pours around 25 ml of liquid in a seconds. For an normal glass of 250 ml, the need time for switching the pump on is like 10 seconds. 

During the development part, we have noticed that the sensor measures an undeterministic and wrong value if the pump is on in that period. The solution proposed is to stop the activation of the pump for like 100 ml every 1 seconds(period of sampling the ultrasonic sensor). 

## What the system does

## Configuration

## Useful Links
[Demo](LINK HERE)
[Linkedin Profile](https://www.linkedin.com/public-profile/settings?lipi=urn%3Ali%3Apage%3Ad_flagship3_profile_self_edit_contact-info%3BIBuyH%2BI8S8WMSqIn7dUb8A%3D%3D)
[Blog post](LINK HERE)

## Images from the system 