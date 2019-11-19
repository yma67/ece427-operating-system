CFLAGS = -g -Wall -std=gnu99 

LDFLAGS = `pkg-config fuse --cflags --libs`

# Uncomment one of the following three lines to compile
SOURCES_1= disk_emu.c sfs_api.c sfs_test.c sfs_api.h
SOURCES_2= disk_emu.c sfs_api.c sfs_test2.c sfs_api.h
SOURCES= disk_emu.c sfs_api.c fuse_wrappers.c sfs_api.h

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

