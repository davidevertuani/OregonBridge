/* This file is part of MqttBridge.ino */

void MQTT_setup() {
  // initiate Mqtt connection
  // you may need to change the port depending on your broker settings
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(callback);
}

void MQTT_loop() {
  if (mqttClient.connected())
    mqttClient.loop();
  else {
    long now = millis();
    if (now - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
      lastReconnectAttempt = now;

      // Attempt to reconnect
      if (reconnectClient()) lastReconnectAttempt = 0;
    }
  }
}

bool reconnectClient() {
  // if auth needed: mqttClient.connect("OsBridge", "user", "pwd")
  if (mqttClient.connect("OsBridge")) {
    // mqttClient.subscribe("topic");
  }
  return mqttClient.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
  // MQTT callback for subscriptions
  // no-op
}