# Dartsync makefile

all:
	gcc src/server.c utils/file_monitor.c utils/network.c utils/tracker_peer_table.c utils/peer2peer.c -lpthread -o server -g
	gcc src/client.c utils/file_monitor.c utils/network.c utils/tracker_peer_table.c utils/peer2peer.c -lpthread -o client -g
	cp client gile/client
	cp client bear/client
	cp client spruce/client
	cp client green/client
	cp client wildcat/client

clean:
	rm -f client
	rm -f server
	rm -f gile/client
	rm -f bear/client
	rm -f spruce/client
	rm -f green/client
