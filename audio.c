#include <stdlib.h>
#include <stdio.h>
#include <jack/jack.h>
#include <rubberband/rubberband-c.h>
#include "circularbuffers.h"
#include "audio.h"


jack_client_t *jack_client;
jack_port_t *ports[2];
unsigned long int sample_rate;
struct circularbuffers *cbs;
struct circularbuffers *stretcher_cbs;
struct RubberBandState *stretcher;
double speed, pitch;


void dump()
    {
    printf("cbs - begin: %lu   end: %lu   position %lu  \n", cbs->bufferbegin, cbs->bufferend, cbs->readposition);
    }

void dump2()
    {
    printf("stretcher_cbs - begin: %lu   end: %lu   position %lu  \n", stretcher_cbs->bufferbegin, stretcher_cbs->bufferend, stretcher_cbs->readposition);
    }

void seek_stream(double vector)
    {
    circular_seek_percentage(cbs, vector);
    }


int process(jack_nframes_t nframes, void *notused)
    {

    int	i;
    for (i=0; i<2; i++)
        if (ports[i]==NULL)
	    return 0;

    jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer (ports[0], nframes);
    jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (ports[1], nframes);

    rubberband_set_time_ratio(stretcher, speed);
    rubberband_set_pitch_scale(stretcher, pitch);

    dump();
    unsigned long int tmp= circular_write(cbs, in, 0, nframes, 1);
    printf("wrote %lu samples from in to cbs\n", tmp);
    dump();

    jack_default_audio_sample_t *pointertopointer[1];
    pointertopointer[0]=circular_reading_data_pointer(cbs,0);
    unsigned long int copying_number;
    while (0<(copying_number = min(circular_readable_continuous(cbs), rubberband_get_samples_required(stretcher))))
	{
	printf("stretching %lu samples (%i needed)\n", copying_number, rubberband_get_samples_required(stretcher));
	rubberband_process(stretcher, pointertopointer, copying_number, 0);
	circular_seek_relative(cbs, copying_number);
	dump();
	}
    
    dump2();
    pointertopointer[0]=circular_writing_data_pointer(stretcher_cbs,0);
    while (0< (copying_number = min(rubberband_available(stretcher), circular_writable_continuous(stretcher_cbs))))
	{
	printf("buffering %lu stretched samples\n", copying_number);
	rubberband_retrieve(stretcher, pointertopointer, copying_number);
	circular_write_seek_relative(stretcher_cbs, copying_number);
	dump2();
    	}

    unsigned long int done=0;
    if (circular_used_space(stretcher_cbs)-circular_get_position_offset(stretcher_cbs)>= nframes)
	while (done<nframes)
	{
	copying_number=min(nframes-done, circular_readable_continuous(stretcher_cbs));
	printf("outputting %lu stretched samples\n", copying_number);
	circular_read(stretcher_cbs, out, 0, copying_number);
	done+=copying_number;
	dump2();
    	}
    printf("	END CYCLE\n");
    
    return 0;
    }
    

int init_audio(int time, int channels)
    {
    speed=1; pitch=1;
    
    if ((jack_client = jack_client_open("strebupitecha", JackNullOption, NULL)) == 0)
	    return 1;
    jack_set_process_callback (jack_client, process, 0);
    sample_rate= jack_get_sample_rate (jack_client);
    ports[0] = jack_port_register(jack_client, "inleft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    ports[1] = jack_port_register(jack_client, "outleft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    cbs=circular_new(1, time*sample_rate);
    stretcher_cbs=circular_new(1, 1*sample_rate);
    stretcher= rubberband_new(sample_rate, 1+0, RubberBandOptionProcessRealTime, 1.0,1.0);
    printf("latency: %i\n", rubberband_get_latency(stretcher));
    
    if (jack_activate (jack_client))
    	return 2;
    return 0;
    }
