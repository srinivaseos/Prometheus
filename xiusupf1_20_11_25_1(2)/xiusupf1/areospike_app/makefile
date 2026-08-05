all:
	gcc -g3 app_aero.c aero_tcp.c aero_message.c app_jet_message.c \
	../dplane/app_endpoint.c ../dplane/app_stack.c \
	-o app_aero -I../dplane/ -I../jansson-2.13/src/ -lpthread -ljansson /usr/lib/libaerospike.so -lssl -lz -lm
	gcc -g3 app_aero_client.c aero_tcp.c \
	../dplane/app_endpoint.c ../dplane/app_stack.c app_jet_message.c \
	-o app_aero_client -I../dplane/ -I../jansson-2.13/src/ -lpthread -ljansson /usr/lib/libaerospike.so -lssl -lz -lm