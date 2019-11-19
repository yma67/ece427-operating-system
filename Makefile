CFLAGS = -g -Wall -std=gnu99 

LDFLAGS = `pkg-config fuse --cflags --libs`

# Uncomment one of the following three lines to compile
SOURCES_1= single_layer_fuse/unorganize/disk_emu.c single_layer_fuse/unorganize/sfs_api.c single_layer_fuse/unorganize/sfs_test.c single_layer_fuse/unorganize/sfs_api.h
SOURCES_2= single_layer_fuse/unorganize/disk_emu.c single_layer_fuse/unorganize/sfs_api.c single_layer_fuse/unorganize/sfs_test2.c single_layer_fuse/unorganize/sfs_api.h
SOURCES= single_layer_fuse/unorganize/disk_emu.c single_layer_fuse/unorganize/sfs_api.c single_layer_fuse/unorganize/fuse_wrappers.c single_layer_fuse/unorganize/sfs_api.h

EXECUTABLE=yuxiang_ma_sfs

all: $(SOURCES)
	gcc $(CFLAGS) $(LDFLAGS) -o $(EXECUTABLE) $(SOURCES)
	
test1: $(SOURCES_1)
	gcc $(CFLAGS) -o $(EXECUTABLE) $(SOURCES_1)

test2: $(SOURCES_1)
	gcc $(CFLAGS) -o $(EXECUTABLE) $(SOURCES_2)

fuse: $(SOURCES)
	gcc $(CFLAGS) $(LDFLAGS) -o $(EXECUTABLE) $(SOURCES)

clean:
	rm -rf *.o *~ $(EXECUTABLE)

