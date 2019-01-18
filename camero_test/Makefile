CC?=$(CROSS_COMPILE)gcc
CFLAGS_=-I /opt/poky-atmel/2.4.2/sysroots/cortexa5hf-vfp-poky-linux-gnueabi/usr/include/
CFLAGS_ += -I /opt/poky-atmel/2.4.2/sysroots/cortexa5hf-vfp-poky-linux-gnueabi/usr/include/libdrm/
LDFLAGS_=-L /opt/poky-atmel/2.4.2/sysroots/cortexa5hf-vfp-poky-linux-gnueabi/usr/lib/

FLAGS=-ldrm
all:
	$(CC) $(CFLAGS_) $(LDFLAGS_) $(FLAGS) camero_plane.c v4l2_test.c -o cemero_test


.PHONY: clean
clean:
	rm -f *.o