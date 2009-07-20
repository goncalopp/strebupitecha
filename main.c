#include "audio.h"
#include "gtk.h"

int main (int argc, char *argv[])
    {
    int time= 10; 			//buffer time, in seconds;
    int channels= 1;			//number of channels
    if (init_audio(time, channels))
    	return 1;
    init_gtk(argc, argv);
    return 0;
    }

