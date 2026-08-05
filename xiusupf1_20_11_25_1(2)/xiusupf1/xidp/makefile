all:
	gcc -g3 main.c ../dplane/app_stack.c ../areospike_app/aero_tcp.c ../areospike_app/app_jet_message.c ../dplane/app_endpoint.c -o xidp \
		-I../dplane/ -I../areospike_app/ -I../jansson-2.13/src/ -lpthread -ljansson /usr/lib/libaerospike.so -lssl -lz -lm
