#include <gtk/gtk.h>
#include "gtk.h"
#include "audio.h"

GtkRange *positionslider, *speedslider, *pitchslider;

gdouble normalize(gdouble value) {if (value>1) return 1; if (value<0) return 0; return value;}



void changesliders(double position, double speed, double pitch)
    {
    if ((position>=0)&&(position<=1))
        gtk_range_set_value(positionslider, position);
    if ((speed>=0)&&(speed<=1))
        gtk_range_set_value(speedslider, speed);
    if ((pitch>=0)&&(pitch<=1))
        gtk_range_set_value(pitchslider, pitch);
    }


gboolean  on_position_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
    {
    value=normalize(value);
    seek_stream(value);
    return 0;
    }

gboolean  on_speed_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
    {
    value=normalize(value);    
    change_speed(1-value+0.5);
    return 0;
    }

gboolean  on_pitch_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
    {
    value=normalize(value);    
    change_pitch(1+value-0.5);
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
    positionslider = GTK_RANGE (gtk_builder_get_object (builder, "position"));
    speedslider = GTK_RANGE (gtk_builder_get_object (builder, "speed"));
    pitchslider = GTK_RANGE (gtk_builder_get_object (builder, "pitch"));

    gtk_range_set_value(pitchslider, 0.5);
    gtk_range_set_value(positionslider, 1);
    gtk_range_set_value(speedslider, 0.5);
    
    
    gtk_builder_connect_signals (builder, NULL);
    g_object_unref (G_OBJECT (builder));
    gtk_widget_show (window);                
    
    }

void start_gtk() {gtk_main ();}
