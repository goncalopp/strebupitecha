#include <stdlib.h>
#include <gtk/gtk.h>
#include <jack/jack.h>

jack_port_t *ports[2];
unsigned long int sample_rate;
unsigned long int bufferlength;
jack_default_audio_sample_t *buffer;
jack_default_audio_sample_t *streambegin, *streamend;
double readposition, speed=0;

gboolean  on_position_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
    {
    if (value>1) value=1; if (value<0) value=0;    
    long int duration;
    if (streamend >= streambegin)
    	duration= streamend-streambegin;
    else
    	duration= bufferlength - (streamend-streambegin);
    readposition= (streambegin + (long) (duration*value)) - buffer;

    if ((readposition)>= bufferlength)
	readposition-=bufferlength;
    return 0;
    }

gboolean  on_speed_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
    {
    if (value>0.90) value=0.90; if (value<-0.90) value=-0.90;    
    speed=1+value;
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

    for (i=0; i<nframes; i++)
        {
	//printf("b: %i e: %i p: %i\n", streambegin-buffer, streamend-buffer, readposition-buffer);
		
	*streamend=*in;

	//*out=*(buffer+(int)readposition);			//no interpolation
	
	double floating=readposition- (int)readposition;		//linear interpolation
	*out=	*(buffer+(int)readposition)*(1-floating) +		//linear interpolation
		*(buffer+(int)readposition+1)*floating;			//linear interpolation
	
	in++; out++; streamend++; readposition+=speed;

	if ((streamend-buffer)>= bufferlength)
		streamend=buffer;
	if ((readposition)>= bufferlength)
		readposition=0;
	if (streambegin==streamend)
		streambegin++;
	
	}

    return 0;
    }
    

int init_jack(int time)
    {
    jack_client_t *client;
    if ((client = jack_client_open("strebupitecha", JackNullOption, NULL)) == 0)
	return 1;
    jack_set_process_callback (client, process, 0);
    sample_rate= jack_get_sample_rate (client);
    bufferlength= time*sample_rate;

    bufferlength=time*sample_rate;
    buffer=malloc(bufferlength*sizeof(jack_default_audio_sample_t));
    readposition= 0; streambegin= buffer; streamend=buffer; speed=1;
    
    ports[0] = jack_port_register (client, "inleft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    ports[1] = jack_port_register (client, "outleft", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (jack_activate (client))
    	return 2;
    return 0;
    }

int main (int argc, char *argv[])
    {
    int time= 60; 			//buffer time, in seconds;
    if (init_jack(time)) return 1;

    init_gtk(argc, argv);
    return 0;
    }

