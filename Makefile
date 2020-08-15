CC?=$(CROSS_COMPILE)gcc
CFLAGS_=-I /opt/poky-atmel/2.4.2/sysroots/cortexa5hf-neon-poky-linux-gnueabii/usr/include/
CFLAGS_ += -I /opt/poky-atmel/2.4.2/sysroots/cortexa5hf-neon-poky-linux-gnueabi/usr/include/libdrm/
LDFLAGS_=-L /opt/poky-atmel/2.4.2/sysroots/cortexa5hf-neon-poky-linux-gnueabi/usr/lib/

FLAGS=-ldrm
all:
	$(CC) $(CFLAGS_) $(LDFLAGS_) $(FLAGS) modeset-plane-test.c -o modeset-plane-test
	$(CC) $(CFLAGS_) $(LDFLAGS_) $(FLAGS) modeset-plane-move.c -o modeset-plane-move
	$(CC) $(CFLAGS_) $(LDFLAGS_) $(FLAGS) modeset-plane-rotate.c -o modeset-plane-rotate
	$(CC) $(CFLAGS_) $(LDFLAGS_) $(FLAGS) modeset-plane-scale.c -o modeset-plane-scale
	$(CC) $(CFLAGS_) $(LDFLAGS_) $(FLAGS)  modeset-plane-alpha.c -o  modeset-plane-alpha
.PHONY: clean
clean:
	rm -f *.o