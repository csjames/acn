package configs

import (
	MQTT "github.com/eclipse/paho.mqtt.golang"
	"github.com/tarm/serial"
	"gopkg.in/redis.v5"
)

var SERIAL_PORT = "/dev/ttyGateway"

var SERIAL_CONFIG = &serial.Config{
	Name: SERIAL_PORT,
	Baud: 115200,
}

var REDIS_CONFIG = &redis.Options{
	Addr:     "localhost:6379",
	Password: "", // no password set
	DB:       0,  // use default DB
}

var MQTT_CONFIG = MQTT.NewClientOptions().AddBroker("ws://127.0.0.1:9900")

func Init(f MQTT.MessageHandler) {
	MQTT_CONFIG.SetClientID("gwprog")
	MQTT_CONFIG.SetDefaultPublishHandler(f)
}
