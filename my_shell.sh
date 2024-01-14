home/irza/Desktop/CEW OEL/cod.c
gcc -o cod cod.c $(pkg-config --cflags --libs glib-2.0 gdk-pixbuf-2.0) -lcurl -ljansson -lnotify -lm
./cod
