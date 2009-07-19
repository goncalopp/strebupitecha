#include <stdlib.h>
#include <gtk/gtk.h>
#include <jack/jack.h>
#include <rubberband/rubberband-c.h>
#include "circularbuffers.h"

extern int min(int a, int b) ;



jack_port_t *ports[2];
unsigned long int sample_rate; 
struct circularbuffers *cbs;
struct circularbuffers *stretcher_cbs;
double speed, pitch;
struct RubberBandState *stretcher;


void dump()
    {
    printf("cbs - begin: %lu   end: %lu   position %lu  \n", cbs->bufferbegin, cbs->bufferend, cbs->readposition);
    }

void dump2()
    {
    printf("stretcher_cbs - begin: %lu   end: %lu   position %lu  \n", stretcher_cbs->bufferbegin, stretcher_cbs->bufferend, stretcher_cbs->readposition);
    }

gboolean  on_position_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
    {
    if (value>1) value=1; if (value<0) value=0;
    circular_seek_percentage(cbs, value);
    return 0;
    }

gboolean  on_speed_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
    {
    if (value>0.90) value=0.90; if (value<-0.90) value=-0.90;    
    pitch=1+value;
    return 0;
    }

void on_window_destroy (GtkObject *object, gpointer user_data)
    {
    gtk_main_quit ();
    }

void init_gtk(int argc, char *argv[])
    {
    GtkBuilder      *builder; 
    GtkWidget       *window;
    gtk_init (&argc, &argv);
    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "strebupitecha.glade", NULL);
    window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
    GtkAdjustment *pos = GTK_ADJUSTMENT (gtk_builder_get_object (builder, "positionadjustment"));
    gtk_adjustment_set_value(pos, 1);
    
    gtk_builder_connect_signals (builder, NULL);
    g_object_unref (G_OBJECT (builder));
    gtk_widget_show (window);                
    gtk_main ();
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
    jack_client_t *client;
    if ((client = jack_client_open("strebupitecha", JackNullOption, NULL)) == 0)
	return 1;
    jack_set_process_callback (client, process, 0);
    sample_rate= jack_get_sample_rate (client);
    
    cbs=circular_new(1, time*sample_rate);
    stretcher_cbs=circular_new(1, 1*sample_rate);
    
    speed=1; pitch=1;
    stretcher= rubberband_new(sample_rate, 1+0, RubberBandOptionProcessRealTime, 1.0,1.0);
    printf("latency: %i\n", rubberband_get_latency(stretcher));

    ports[0] = jack_port_register(client, "inleft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    ports[1] = jack_port_register(client, "outleft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    
    if (jack_activate (client))
    	return 2;
    return 0;
    }

int main (int argc, char *argv[])
    {
    int time= 1; 			//buffer time, in seconds;
    int channels= 1;			//number of channels
    if (init_audio(time, channels)) return 1;

    init_gtk(argc, argv);
    return 0;
    }

