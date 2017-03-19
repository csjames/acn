package main

import (
	MQTT "github.com/eclipse/paho.mqtt.golang"
	"github.com/tarm/serial"
	"gopkg.in/redis.v5"

	"bufio"
	"configs"
	"encoding/json"
	"io/ioutil"
	"log"
	"models"
	"net/http"
	"os"
	"strconv"
)

var SERIAL_PORT = "/dev/ttyGateway"

const (
	NODE_LIST    = "nodes"
	MESSAGE_RING = "msgRing"
	RING_SIZE    = 32

	OUTBOUND = "messages/outbound"
	INBOUND  = "messages/inbound"
)

type Status struct {
	Nodes    []models.Node
	Messages []*models.Message
}

var status *Status
var client *redis.Client
var port *serial.Port
var mqttClient MQTT.Client

//define a function for the default message handler
func f(client MQTT.Client, msg MQTT.Message) {
	log.Printf("TOPIC: %s\n", msg.Topic())
	log.Printf("MSG: %s\n", msg.Payload())

	pl := msg.Payload()

	m := models.Message{}
	err := json.Unmarshal(pl, &m)

	if err != nil {
		log.Println(err)
		return
	}

	header := make([]byte, 4, 4)

	header[0] = m.Sender[0]
	header[1] = m.Destination[0]
	header[2] = m.Type[0]

	outbound := append(header, m.Payload...)

	log.Println(outbound)
}

func main() {
	configs.Init(f)

	var err error
	s, err := serial.OpenPort(configs.SERIAL_CONFIG)
	if err != nil {
		log.Println(err)
	}

	port = s
	go scanFromSerial()

	client = redis.NewClient(configs.REDIS_CONFIG)

	_, err = client.Ping().Result()

	if err != nil {
		log.Println(err)
		os.Exit(-1)
	}

	log.Println("Connected to redis")

	//create and start a client using the above ClientOptions
	mqttClient = MQTT.NewClient(configs.MQTT_CONFIG)
	if token := mqttClient.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	if token := mqttClient.Subscribe(OUTBOUND, 0, nil); token.Wait() && token.Error() != nil {
	 	log.Println(token.Error())
	 	os.Exit(1)
	}

	stat := Status{}
	status = &stat

	fs := http.FileServer(http.Dir("static"))
	http.Handle("/", fs)
	http.Handle("/current", http.HandlerFunc(Current))
	http.Handle("/clearNodes", http.HandlerFunc(ClearNodes))
	http.Handle("/clearMessages", http.HandlerFunc(ClearMessages))

	log.Println("Listening...")
	http.ListenAndServe(":3000", nil)
}

func SendMessage(w http.ResponseWriter, r *http.Request) {
	log.Println("Sending Message")

	defer r.Body.Close()
	body, err := ioutil.ReadAll(r.Body)

	message := models.Message{}
	err = json.Unmarshal(body, &message)

	if err != nil {
		log.Println(err)
		w.WriteHeader(http.StatusBadRequest)
		return
	}

}

func Current(w http.ResponseWriter, r *http.Request) {
	log.Println("Serving Current State")

	status.Nodes = getNodesFromRedis()
	status.Messages = getMessagesFromRedis()

	json.NewEncoder(w).Encode(status)
}

func ClearNodes(w http.ResponseWriter, r *http.Request) {
	log.Println("Clearing Nodes")

	client.Del(NODE_LIST)
	client.SAdd(NODE_LIST, "1")
}

func ClearMessages(w http.ResponseWriter, r *http.Request) {
	log.Println("Clearing Messages")

	client.Del(MESSAGE_RING)
}

func getNodesFromRedis() (nodes []models.Node) {
	resp := client.SMembers(NODE_LIST)

	nodes = make([]models.Node, len(resp.Val()), len(resp.Val()))
	for i, s := range resp.Val() {
		nodes[i] = models.Node(s)
	}
	return nodes
}

func getMessagesFromRedis() (messages []*models.Message) {
	resp := client.LRange(MESSAGE_RING, 0, -1)

	messages = make([]*models.Message, len(resp.Val()), len(resp.Val()))
	for i, s := range resp.Val() {
		var message models.Message
		err := json.Unmarshal([]byte(s), &message)

		if err != nil {
			log.Println(err)
		}
		messages[i] = &message
	}
	return messages
}

func handleNewMessage(message *models.Message) {
	client.SAdd(NODE_LIST, message.Sender)
	client.SAdd(NODE_LIST, message.Destination)

	if message.Type == models.ACK {
		for _, c := range message.Payload {

			nId := strconv.FormatUint(uint64(c), 10)

			if c != 0 {
				client.SAdd(nId)
			}
		}
	}

	nodes := getNodesFromRedis()
	status.Nodes = nodes

	msgString, _ := json.Marshal(message)
	if client.LLen(MESSAGE_RING).Val() < 32 {
		client.RPush(MESSAGE_RING, msgString)
	} else {
		client.LPop(MESSAGE_RING)
		client.RPush(MESSAGE_RING, msgString)
	}
}

func scanFromSerial() {
	scanner := bufio.NewScanner(port)
	defer func() {
		log.Println("Recovered from fatal serial error -- program should be restarted (we are probably debugging web stuff)")
		recover()
	}()

	for scanner.Scan() {
		msg := scanner.Text()
		log.Println(msg)
		//TO:FROM:TYPE:NODETYPE:PAYLOAD
		// we have a true pcket
		if len(msg) >= 4 {
			message := models.Message{}

			source := uint8(msg[0])
			sourceNice := strconv.FormatUint(uint64(source), 10)

			destination := uint8(msg[1])
			destinationNice := strconv.FormatUint(uint64(destination), 10)

			message.Sender = sourceNice
			message.Destination = destinationNice

			message.Type = string(msg[2])

			message.Payload = []byte(msg[4:])

			messageJSON, err := json.Marshal(message)

			if err != nil {
				log.Println(err)
				break
			}

			token := mqttClient.Publish(INBOUND, 0, false, messageJSON)
			token.Wait()
		}
	}

	if err := scanner.Err(); err != nil {
		log.Println("error reading standard serial:", err)
	}
}
