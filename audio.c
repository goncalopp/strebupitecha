#include <stdlib.h>
#include <stdio.h>
#include <jack/jack.h>
#include <rubberband/rubberband-c.h>
#include "circularbuffers.h"
#include "audio.h"


jack_client_t *jack_client;
jack_port_t *ports[2];
unsigned long int sample_rate;
float *wastebuffer;
struct circularbuffers *input_cbs;
struct circularbuffers *output_cbs;
struct RubberBandState *stretcher;

double speed, pitch, seek;
int speed_semaphore, pitch_semaphore, seek_semaphore;


void dump()
    {
    printf("input_cbs  - begin: %lu   end: %lu   position %lu  \n", input_cbs->bufferbegin, input_cbs->bufferend, input_cbs->readposition);
    }

void dump2()
    {
    printf("output_cbs - begin: %lu   end: %lu   position %lu  \n", output_cbs->bufferbegin, output_cbs->bufferend, output_cbs->readposition);
    }

void seek_stream(double vector) {seek=vector; seek_semaphore=1;}
void change_speed(double vector) {speed=vector; speed_semaphore=1;}
void change_pitch(double vector) {pitch=vector; pitch_semaphore=1;}

void execute_control_messages()
    {
    if (speed_semaphore)
    	{
	rubberband_set_time_ratio(stretcher, speed);
	speed_semaphore=0;
	}
    if (pitch_semaphore)
    	{
	rubberband_set_pitch_scale(stretcher, pitch);
	pitch_semaphore=0;
	}
    if (seek_semaphore)
    	{
	circular_seek_percentage(input_cbs, seek);
    	//rubberband_reset(stretcher);
	circular_reset(output_cbs);
	printf("		SEEKED\n");
	seek_semaphore=0;
	}
    }

void process_with_rubberband()
    {
    jack_default_audio_sample_t *pointertopointer[1];
    pointertopointer[0]=circular_reading_data_pointer(input_cbs,0);
    unsigned long int copying_number;
    dump();
    printf("circular readable conti %lu, required: %lu \n", circular_readable_continuous(input_cbs), rubberband_get_samples_required(stretcher));
	
    pointertopointer[0]=circular_reading_data_pointer(input_cbs,0);
    while (0<(copying_number = min(circular_readable_continuous(input_cbs), rubberband_get_samples_required(stretcher))))
	{
	printf("stretching %lu samples (%i needed)\n", copying_number, rubberband_get_samples_required(stretcher));
	rubberband_process(stretcher, pointertopointer, copying_number, 0);
	circular_seek_relative(input_cbs, copying_number);
	pointertopointer[0]=circular_reading_data_pointer(input_cbs,0);
	}
    
    pointertopointer[0]=circular_writing_data_pointer(output_cbs,0);
    printf("available: %lu\n", rubberband_available(stretcher));
    while (0< (copying_number = min(rubberband_available(stretcher), circular_writable_continuous(output_cbs))))
	{
	printf("buffering %lu stretched samples\n", copying_number);
	rubberband_retrieve(stretcher, pointertopointer, copying_number);
	circular_write_seek_relative(output_cbs, copying_number);
	pointertopointer[0]=circular_writing_data_pointer(output_cbs,0);
    	}
    }

void process_not(jack_nframes_t n)
    {
    int tmp= circular_read(input_cbs, circular_writing_data_pointer(output_cbs,0), 0, min(n, circular_writable_continuous(output_cbs)));
    if (tmp<n)
    	circular_read(input_cbs, circular_writing_data_pointer(output_cbs,0), 0, n-tmp);
    circular_write_seek_relative(output_cbs, n);
    }

int process(jack_nframes_t nframes, void *notused)
    {

    int	i;
    for (i=0; i<2; i++)
        if (ports[i]==NULL)
	    return 0;

    execute_control_messages();

    jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer (ports[0], nframes);
    jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (ports[1], nframes);

    unsigned long int tmp;
    tmp= circular_write(input_cbs, in, 0, nframes, 1);
    if (tmp<nframes)
    	printf("error writing input buffer!");

    process_with_rubberband();
    //process_not(nframes);

    if (circular_used_space(output_cbs)-circular_get_position_offset(output_cbs)>= nframes)
	{
	tmp= circular_read(output_cbs, out, 0, nframes);
	if (tmp<nframes)
    		printf("error writing output buffer!");
    	}
    else
    	printf("NOT OUTPUTTING!\n");
    //printf("	END CYCLE\n");
    
    return 0;
    }
    

int init_audio(int time, int channels)
    {
    speed=1; pitch=1; speed_semaphore=0, pitch_semaphore=0, seek_semaphore=0;
    
    if ((jack_client = jack_client_open("strebupitecha", JackNullOption, NULL)) == 0)
	    return 1;
    jack_set_process_callback (jack_client, process, 0);
    sample_rate= jack_get_sample_rate (jack_client);
    ports[0] = jack_port_register(jack_client, "inleft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    ports[1] = jack_port_register(jack_client, "outleft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    input_cbs=circular_new(1, time*sample_rate);
    output_cbs=circular_new(1, 1*sample_rate);
    stretcher= rubberband_new(sample_rate, 1+0, RubberBandOptionProcessRealTime, 1.0,1.0);
    wastebuffer=malloc(sizeof(float)*sample_rate);
    printf("latency: %i\n", rubberband_get_latency(stretcher));
    
    if (jack_activate (jack_client))
    	return 2;
    return 0;
    }
