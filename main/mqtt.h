#ifndef MQTT_H
#define MQTT_H

// Initializes the MQTT connection
void mqtt_start(void);

// Sends a message to the MQTT broker
// message: Pointer to the string containing the message
void mqtt(const char *message);

#endif
