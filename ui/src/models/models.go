package models

const (
	ACK = "A"
)

type Node string

type Message struct {
	Sender      string
	Destination string
	Payload     []byte
	Type        string
}
