#include <stdlib.h>
#include <stdio.h>
#include <jack/jack.h>
#include <rubberband/rubberband-c.h>
#include "circularbuffers.h"
#include "audio.h"
#include "gtk.h"


jack_client_t *jack_client;
jack_port_t *ports[2];
unsigned long int sample_rate;
struct circularbuffers *input_cbs;
struct circularbuffers *output_cbs;
struct RubberBandState *stretcher;

double speed, pitch, seek;
int speed_semaphore, pitch_semaphore, seek_semaphore;
unsigned long int counter;


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

void execute_control_messages(jack_nframes_t nframes)
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
    	rubberband_reset(stretcher);
	circular_reset(output_cbs);
	printf("SEEKED\n");
	seek_semaphore=0;
	}

    if (((input_cbs->bufferlength -circular_get_position_offset(input_cbs) ) < 2*nframes )&&(speed<1.0f) )
    	{
	change_speed(1);
	changesliders(-1.0f, 1-speed+0.5, -1.0f);
    	}
	
    if ((circular_get_position_offset(input_cbs)  < nframes )&&(speed>1.0f) )
    	{
	change_speed(1);
	changesliders(-1.0f, 1-speed+0.5, -1.0f);
    	}
	
    if ((counter= ++counter % 20)==0 )
    	changesliders(circular_get_position_percentage(input_cbs), -1.0f, -1.0f);
    }

void process_with_rubberband(jack_nframes_t nframes)
    {
    jack_default_audio_sample_t *pointerarray[1];
    while (((circular_used_space(output_cbs)-circular_get_position_offset(output_cbs))< nframes)&& (circular_readable_continuous(input_cbs)>0))
	{
	unsigned long int copying= min(circular_readable_continuous(input_cbs), rubberband_get_samples_required(stretcher));
	pointerarray[0]=circular_reading_data_pointer(input_cbs, 0);
	rubberband_process(stretcher, pointerarray, copying, 0);
	circular_seek_relative(input_cbs, copying); 

    	copying= min(circular_writable_continuous(output_cbs), rubberband_available(stretcher));
	pointerarray[0]=circular_writing_data_pointer(output_cbs, 0);
	rubberband_retrieve(stretcher, pointerarray, copying);
	circular_write_seek_relative(output_cbs, copying);
    	}
    }

void process_not(jack_nframes_t nframes)
    {
    int tmp= circular_read(input_cbs, circular_writing_data_pointer(output_cbs,0), 0, min(nframes, circular_writable_continuous(output_cbs)));
    circular_write_seek_relative(output_cbs, nframes);
    if (tmp<nframes)
    	{
	circular_read(input_cbs, circular_writing_data_pointer(output_cbs,0), 0, nframes-tmp);
	circular_write_seek_relative(output_cbs, nframes-tmp);
    	}
    }

int process(jack_nframes_t nframes, void *notused)
    {
    int	i;
    for (i=0; i<2; i++)
        if (ports[i]==NULL)
	    return 0;

    execute_control_messages(nframes);

    jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer (ports[0], nframes);
    jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (ports[1], nframes);

    circular_write(input_cbs, in, 0, nframes, 1);

    process_with_rubberband(nframes);
    //process_not(nframes);

    if (circular_used_space(output_cbs)-circular_get_position_offset(output_cbs)>= nframes)
	circular_read(output_cbs, out, 0, nframes);

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
    
    if (jack_activate (jack_client))
    	return 2;
    return 0;
    }
