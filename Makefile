all:
	#gcc file_monitor.c network.c -o test -g
	gcc server.c utils/file_monitor.c utils/network.c utils/tracker_peer_table.c -lpthread -o server -g
	gcc client.c utils/file_monitor.c utils/network.c utils/tracker_peer_table.c -lpthread -o client -g
	cp client gile/client
	cp client bear/client
	scp -r /home/hw87244557/20150517 weihuang@tahoe.cs.dartmouth.edu:/net/grad/weihuang/proj