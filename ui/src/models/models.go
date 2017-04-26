package models

const (
	ACK = 'A'
)

type Node string

type Message struct {
	Sender      string
	Destination string
	Origin      string
	Payload     []byte
	Type        byte
	UID         uint16
	Direction   byte
}
