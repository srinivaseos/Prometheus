all:
	gcc -g3 amfas.c ../dplane/app_stack.c ../areospike_app/aero_tcp.c \
		../../xiusamf/xamf/ecc.c \
		../../xiusamf/xamf/kasumi.c \
		../../xiusamf/xamf/ogs-aes.c \
		../../xiusamf/xamf/milenage.c \
		../../xiusamf/xamf/ogs-aes-cmac.c \
		../../xiusamf/xamf/ogs-base64.c \
		../../xiusamf/xamf/ogs-sha1.c \
		../../xiusamf/xamf/ogs-sha1-hmac.c \
		../../xiusamf/xamf/ogs-sha2.c \
		../../xiusamf/xamf/ogs-sha2-hmac.c \
		../../xiusamf/xamf/snow-3g.c \
		../../xiusamf/xamf/zuc.c \
		../../xiusamf/xamf/snow_core.c \
		../../xiusamf/xamf/ogs_fwk.c \
		../../xiusamf/xamf/ogs-file.c \
		../dplane/app_endpoint.c -o amfas \
		-I../dplane/ -I../../xiusamf/xamf/  -I../areospike_app/ -I../jansson-2.13/src/ -lpthread -ljansson /usr/lib/libaerospike.so -lssl -lz -lm	