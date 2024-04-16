
EXTRA_CFLAGS := -I../common -I../common/log
EXTRA_CFLAGS += -I../platform/linux -I./mbedtls/include
EXTRA_CFLAGS += -I../mqttclient

obj-y += network.o
obj-y += nettype_tcp.o
ifeq ($(MQTT_USE_TLS), no)
else
obj-y += nettype_tls.o
obj-y += mbedtls/
endif
