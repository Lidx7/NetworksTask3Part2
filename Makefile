CC = gcc
FLAGS = -Wall -g

all:RUDP_Receiver RUDP_Sender

RUDP_Receiver: RUDP_Receiver.c
	$(CC) $(FLAGS) RUDP_Receiver.c -o RUDP_Receiver

RUDP_Sender: RUDP_Sender.c
	$(CC) $(FLAGS) RUDP_Sender.c -o RUDP_Sender

clean:
	rm RUDP_Receiver RUDP_Sender

.PHONY: clean all