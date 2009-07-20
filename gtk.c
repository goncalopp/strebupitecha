#include <gtk/gtk.h>
#include "audio.h"

gboolean  on_position_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
    {
    if (value>1) value=1; if (value<0) value=0;
    seek_stream(value);
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
