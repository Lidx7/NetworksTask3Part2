CC = gcc
FLAGS = -Wall -g

all:RUDP_Receiver RUDP_Sender

RUDP_Receiver: RUDP_Receiver.c
	$(CC) $(CFLAGS) RUDP_Receiver.c -o RUDP_Receiver

RUDP_Sender: RUDP_Sender.c
	$(CC) $(CFLAGS) RUDP_Sender.c -o RUDP_Sender

clean:
	rm RUDP_Receiver RUDP_Sender

.PHONY: clean all