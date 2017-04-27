package main

import (
	"github.com/tarm/serial"
	"gopkg.in/redis.v5"

	"bufio"
	"configs"
	"encoding/json"
	"github.com/gorilla/websocket"
	"io/ioutil"
	"log"
	"models"
	"net/http"
	"os"
	"strconv"
	"strings"
	"sync"
)

var SERIAL_PORT = "/dev/ttyGateway"

const (
	NODE_LIST    = "nodes"
	MESSAGE_RING = "msgRing"
	RING_SIZE    = 8

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

var connections map[*websocket.Conn]bool
var connLock sync.Mutex

func main() {
	connections = make(map[*websocket.Conn]bool)

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

	stat := Status{}
	status = &stat

	fs := http.FileServer(http.Dir("static"))
	http.Handle("/", fs)
	http.Handle("/current", http.HandlerFunc(Current))
	http.Handle("/clearNodes", http.HandlerFunc(ClearNodes))
	http.Handle("/clearMessages", http.HandlerFunc(ClearMessages))
	http.Handle("/sub", http.HandlerFunc(WSHandler))
	log.Println("Listening...")
	http.ListenAndServe(":3000", nil)
}

func WSHandler(w http.ResponseWriter, r *http.Request) {
	if r.Header.Get("Origin") != "http://"+r.Host {
		http.Error(w, "Origin not allowed", 403)
		return
	}
	conn, err := websocket.Upgrade(w, r, w.Header(), 1024, 1024)
	if err != nil {
		http.Error(w, "Could not open websocket connection", http.StatusBadRequest)
	}

	connections[conn] = true

	go echo(conn)
}

func echo(conn *websocket.Conn) {
	for {
		m := models.Message{}

		err := conn.ReadJSON(&m)
		if err != nil {
			log.Println("WS Closing...")
			connLock.Lock()
			delete(connections, conn)
			connLock.Unlock()
			return
		}

		log.Printf("Got message: %#v\n", m)
		// echo message out here :)
	}
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

	var msg = make([]byte, 29, 29)
	dest, err := strconv.ParseUint(message.Destination, 10, 64)
	if err != nil {
		log.Println("Destination in bad form")
		return
	}

	msg[0] = byte(dest)
	msg[1] = message.Type

	for i := 0; i < len(message.Payload); i++ {
		msg[i+2] = message.Payload[i]
	}

	port.Write(msg)
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
	client.SAdd(NODE_LIST, "13")
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
	log.Println("handling message")

	if message.Sender != "0" {
		client.SAdd(NODE_LIST, message.Sender)
	}
	client.SAdd(NODE_LIST, message.Destination)
	if message.Origin != "0" {
		client.SAdd(NODE_LIST, message.Origin)
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

	for c, _ := range connections {
		c.WriteJSON(message)
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
		log.Print("received: ")
		log.Println(msg)

		// i ingoing
		// r repeat
		// o outgoing
		if msg[0] != 'i' && msg[0] != 'r' && msg[0] != 'o' {
			log.Println("Aint for us.")
			continue
		}

		msgPkt := models.Message{}
		msgPkt.Direction = msg[0]

		parts := strings.Split(msg[3:], "^^")

		if len(parts) != 6 {
			log.Println("Not enough fields :(")
			continue
		}

		msgPkt.Origin = parts[0]
		msgPkt.Sender = parts[1]
		msgPkt.Destination = parts[2]

		uid, err := strconv.Atoi(parts[3])

		if err != nil {
			log.Println("Error parsing the UID")
			continue
		}

		msgPkt.UID = uint16(uid)

		msgPkt.Type = parts[4][0]
		msgPkt.Payload = []byte(parts[5])

		handleNewMessage(&msgPkt)
	}

	if err := scanner.Err(); err != nil {
		log.Println("error reading standard serial:", err)
	}
}
