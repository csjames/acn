package configs

import (
	"github.com/tarm/serial"
	"gopkg.in/redis.v5"
)

var SERIAL_PORT = "/dev/ttyACM0"

var SERIAL_CONFIG = &serial.Config{
	Name: SERIAL_PORT,
	Baud: 115200,
}

var REDIS_CONFIG = &redis.Options{
	Addr:     "localhost:6379",
	Password: "", // no password set
	DB:       0,  // use default DB
}
