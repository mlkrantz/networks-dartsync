# Dartsync makefile

all:
	# gcc file_monitor.c network.c -o test -g
	gcc src/server.c utils/file_monitor.c utils/network.c utils/tracker_peer_table.c -lpthread -o server -g
	gcc src/client.c utils/file_monitor.c utils/network.c utils/tracker_peer_table.c -lpthread -o client -g
	cp client gile/client
	cp client bear/client
	# scp -r /home/hw87244557/20150517 weihuang@tahoe.cs.dartmouth.edu:/net/grad/weihuang/proj

clean:
	rm -f client
	rm -f server
	rm -f gile/client
	rm -f bear/client
