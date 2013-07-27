
#include <stdlib.h>

#include <gtk/gtk.h>
#include <sndfile.h>

#include "darabana.h"

GtkWidget *window;
GtkWidget *statusbar;
GtkWidget *graph;
GdkPixmap *graph_pixmap;
GtkWidget *info_label;
cairo_t *cr;

GtkObject *adj[SLIDER_COUNT];

char length_info[256];

char *slider_text[] = {
	"Amp.",
	"Decay",
	"Freq.",
	"Freq. decay",
	"Amp",
	"Decay",
	"Color",
	"Amp.",
	"Length",
	"Freq.",
	"Phase"
};

float gen_data[SLIDER_COUNT];

/* this array holds the sliders' configuration, along with their default values
*/
double adj_data[SLIDER_COUNT][6] = {
	{ 2, 0, 5.0, 0.05, 0.1, 3 },
	{ 0.9995, 0, 1, 0.01, 0.1, 3 },
	{ 210, 0, 2000, 1, 5 , 0 },
	{ 0.002, 0, 1, 0.01, 0.1, 3 },

	{ 1.0 , 0, 5, 0.05, 0.1, 3 },
	{ 0.9997, 0, 1, 0.01, 0.1, 3 },
	{ 0.5, 0, 1, 0.01, 0.1, 3 },

	{ 5, 0, 10, 0.1, 0.5, 3 },
	{ 5, 0, 100, 1, 5, 1 },
	{ 0.5, 0, 1, 0.01, 0.1, 3 },
	{ 5, 0, 100, 1, 5, 1 }
};

/* GTK+ callback for changing any of the sound parameters;
 * it also triggers a drum sample regeneration
*/
void change_param( GtkAdjustment *a, gpointer data )
{
#ifdef DEBUG
	printf( "%i: %f\n", (int)data, a->value );
#endif

	gen_data[(int)data] = a->value;

	buf_length = generate_drum(
		gen_data[0], gen_data[1], gen_data[2], gen_data[3],
		gen_data[4], gen_data[5], gen_data[6], gen_data[7],
		gen_data[8], gen_data[9], gen_data[10] );

	/* graph update needed */
	gdk_window_invalidate_rect( GDK_WINDOW( graph->window ), NULL, FALSE );

	/* update length text */
	snprintf( length_info, 256, "Length: %i samples (%f seconds).",
		buf_length, (float)buf_length/(float)SAMPLE_RATE );
	gtk_label_set_text( GTK_LABEL( info_label ), length_info );
}

/* GTK+ callback for pushing the 'Play' button */
void gui_play( GtkWidget *w, gpointer data )
{
	(void) w;
	(void) data;

	buf_position = 0;
}

/* GTK+ callback for exporting the sample */
void gui_export( GtkWidget *w, gpointer data )
{
	(void) w;
	(void) data;

	GtkWidget *export_dialog;
	char *fn;
	SNDFILE *sf;
	SF_INFO si;

	si.samplerate = SAMPLE_RATE;
	si.channels = CHANNEL_COUNT;
	si.format = OUTPUT_FORMAT;

	export_dialog = gtk_file_chooser_dialog_new( "Export Wave",
		GTK_WINDOW( window ), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL );
	
	if( gtk_dialog_run( GTK_DIALOG( export_dialog ) ) == GTK_RESPONSE_ACCEPT )
	{
		fn = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( export_dialog ) );

#ifdef DEBUG
		printf( "export: %s\n", fn );
#endif

		sf = sf_open( fn, SFM_WRITE, &si );
		sf_write_float( sf, main_buf, buf_length );
		sf_close( sf );
		g_free( fn );
	}

	gtk_widget_destroy( export_dialog );
}

void gui_save_preset( GtkWidget *w, gpointer data )
{
	(void) w;
	(void) data;

	GtkWidget *save_dialog;
	char *fn;
	FILE *f;

	save_dialog = gtk_file_chooser_dialog_new( "Save Preset",
		GTK_WINDOW( window ), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL );
	
	if( gtk_dialog_run( GTK_DIALOG( save_dialog ) ) == GTK_RESPONSE_ACCEPT )
	{
		fn = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( save_dialog ) );
#ifdef DEBUG
		printf( "save preset: %s\n", fn );
#endif

		f = fopen( fn, "w" );
		if( !f )
		{
			gtk_widget_destroy( save_dialog );
			raise_warning( "Unable to save preset." );
			return;
		}

		int i;
		for( i = 0; i < SLIDER_COUNT; i++ )
		{
			fprintf( f, "%f ", gen_data[i] );
		}

		fclose( f );
	}

	gtk_widget_destroy( save_dialog );
}

void gui_load_preset( GtkWidget *w, gpointer data )
{
	(void) w;
	(void) data;

	GtkWidget *load_dialog;
	char *fn;
	FILE *f;

	load_dialog = gtk_file_chooser_dialog_new( "Load Preset",
		GTK_WINDOW( window ), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL );
	
	if( gtk_dialog_run( GTK_DIALOG( load_dialog ) ) == GTK_RESPONSE_ACCEPT )
	{
		fn = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( load_dialog ) );
#ifdef DEBUG
		printf( "load preset: %s\n", fn );
#endif

		f = fopen( fn, "r" );
		if( !f )
		{
			gtk_widget_destroy( load_dialog );
			raise_warning( "Unable to load preset." );
			return;
		}

		int i;
		for( i = 0; i < SLIDER_COUNT; i++ )
		{
			float v = 0.0f;
			fscanf( f, "%f ", &v );

			if( ( v >= adj_data[i][1] ) &&
				( v <= adj_data[i][2] ) )
			{
				gtk_adjustment_set_value( GTK_ADJUSTMENT( adj[i] ), v );
			}
			else
			{
				gtk_widget_destroy( load_dialog );
				raise_warning( "Preset value out of range. Cancelling load." );
				return;
			}
		}
	}

	gtk_widget_destroy( load_dialog );
}

gboolean delete_event( GtkWidget *w, GdkEvent *e, gpointer data )
{
	(void) w;
	(void) e;
	(void) data;

	return FALSE;
}

void destroy( GtkWidget *w, gpointer data )
{
	(void) w;
	(void) data;

	gtk_main_quit();
}

/* GTK+ callback for drawing the graph;
 * amplitude/time graph, with the amplitude ranging from -1 to 1
*/
gboolean draw_graph( GtkWidget *w, GdkEventExpose *e )
{
	(void) e;

	cr = gdk_cairo_create( gtk_widget_get_window( w ) );

	cairo_set_source_rgb( cr, 0, 0, 0 );
	cairo_paint( cr );

	cairo_set_source_rgb( cr, 0.3, 0.3, 0.3 );
	cairo_set_line_width( cr, 1 );
	cairo_move_to( cr, 0, 50 );
	cairo_line_to( cr, w->allocation.width, 50 );
	cairo_stroke( cr );

	cairo_set_line_width( cr, 1 );
	cairo_set_source_rgb( cr, 0.0, 0.7, 0.0 );

	int i;
	int l = 0;
	float f = 0.0f;

	for( i = 0; i < w->allocation.width-1; i++ )
	{
		f = ( (float)i / (float)w->allocation.width ) * (float)buf_length;
		cairo_set_source_rgb( cr, 0.9, 0.9, 0.9 );

		cairo_move_to( cr, i, l );
		l = (float)main_buf[ (int)f ] * (float)(w->allocation.height/2) + (w->allocation.height/2);
		cairo_line_to( cr, i+1, l );
		cairo_stroke( cr );
	}

	cairo_destroy( cr );

	return TRUE;
}

void configure_graph( GtkWidget *w, gpointer data )
{
	(void) data;

	if( graph_pixmap )
		gdk_pixmap_unref( graph_pixmap );
	
	graph_pixmap = gdk_pixmap_new( w->window, w->allocation.width, w->allocation.height, -1 );

	gdk_draw_rectangle( graph_pixmap, w->style->black_gc, TRUE, 0, 0, w->allocation.width, w->allocation.height );
}

void raise_error( char* s )
{
	GtkWidget *dialog = gtk_message_dialog_new( GTK_WINDOW( window ),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		s );
	gtk_dialog_run( GTK_DIALOG( dialog ) );
	gtk_widget_destroy( dialog );

	terminate_pa();
	free( main_buf );
	exit( 0 );
}

void raise_warning( char* s )
{
	GtkWidget *dialog = gtk_message_dialog_new( GTK_WINDOW( window ),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
		s );
	gtk_dialog_run( GTK_DIALOG( dialog ) );
	gtk_widget_destroy( dialog );
}

/* the GUI-making function, coded by hand;
 * lesson learned: use a designer.
*/
void make_gui()
{
	GtkWidget *main_box;
	GtkWidget *main_slider_box;
	GtkWidget *button_box;

	GtkWidget *slider[SLIDER_COUNT];
	GtkWidget *slider_box[SLIDER_COUNT];
	GtkWidget *slider_label[SLIDER_COUNT];

	GtkWidget *btn_play, *btn_export, *btn_save, *btn_load;
	GtkWidget *separator1, *separator2;

	GtkWidget *tone_frame, *noise_frame, *phaser_frame;
	GtkWidget *tone_box, *noise_box, *phaser_box;

	srand( time( 0 ) );

	window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_default_size( GTK_WINDOW( window ), 600, 400 );
	gtk_window_set_title( GTK_WINDOW( window ), "Darabana lu' Dumitru" );
	g_signal_connect( window, "delete-event", G_CALLBACK( delete_event ), NULL );
	g_signal_connect( window, "destroy", G_CALLBACK( destroy ), NULL );
	gtk_container_set_border_width( GTK_CONTAINER( window ), 2 );

	statusbar = gtk_label_new( "Darabana lu' Dumitru - http://github.com/deveah/darabana" );
	graph = gtk_drawing_area_new();
	gtk_drawing_area_size( GTK_DRAWING_AREA( graph ), 580, 100 );
	g_signal_connect( graph, "configure_event", G_CALLBACK( configure_graph ), NULL );
	g_signal_connect( graph, "expose_event", G_CALLBACK( draw_graph ), NULL );

	int i;
	for( i = 0; i < SLIDER_COUNT; i++ )
	{
		adj[i] = gtk_adjustment_new( adj_data[i][0], adj_data[i][1], adj_data[i][2], adj_data[i][3], adj_data[i][4], 0.0f );
		g_signal_connect( adj[i], "value-changed", G_CALLBACK( change_param ), (void*)i );
		slider[i] = gtk_vscale_new( GTK_ADJUSTMENT( adj[i] ) );
		gtk_scale_set_digits( GTK_SCALE( slider[i] ), adj_data[i][5] );
		gtk_range_set_update_policy( GTK_RANGE( slider[i] ), GTK_UPDATE_DELAYED );
		gtk_range_set_inverted( GTK_RANGE( slider[i] ), GTK_UPDATE_DELAYED );

		slider_label[i] = gtk_label_new( slider_text[i] );
		gtk_label_set_line_wrap( GTK_LABEL( slider_label[i] ), TRUE );

		slider_box[i] = gtk_vbox_new( FALSE, 5 );
		gtk_box_pack_start( GTK_BOX( slider_box[i] ), slider_label[i], FALSE, FALSE, 0 );
		gtk_widget_show( slider_label[i] );
		gtk_box_pack_start( GTK_BOX( slider_box[i] ), slider[i], TRUE, TRUE, 5 );
		gtk_widget_show( slider[i] );
	}

	main_slider_box = gtk_hbox_new( TRUE, 0 );

	tone_frame = gtk_frame_new( "Tone" );
	noise_frame = gtk_frame_new( "Noise" );
	phaser_frame = gtk_frame_new( "Phaser" );

	tone_box = gtk_hbox_new( TRUE, 0 );
	for( i = 0; i < 4; i++ )
	{
		gtk_box_pack_start( GTK_BOX( tone_box ), slider_box[i], TRUE, TRUE, 0 );
		gtk_widget_show( slider_box[i] );
	}
	gtk_container_add( GTK_CONTAINER( tone_frame ), tone_box );
	gtk_widget_show( tone_box );

	gtk_box_pack_start( GTK_BOX( main_slider_box ), tone_frame, TRUE, TRUE, 0 );
	gtk_widget_show( tone_frame );

	noise_box = gtk_hbox_new( TRUE, 0 );
	for( i = 4; i < 7; i++ )
	{
		gtk_box_pack_start( GTK_BOX( noise_box ), slider_box[i], TRUE, TRUE, 0 );
		gtk_widget_show( slider_box[i] );
	}
	gtk_container_add( GTK_CONTAINER( noise_frame ), noise_box );
	gtk_widget_show( noise_box );

	gtk_box_pack_start( GTK_BOX( main_slider_box ), noise_frame, TRUE, TRUE, 0 );
	gtk_widget_show( noise_frame );

	phaser_box = gtk_hbox_new( TRUE, 0 );
	for( i = 7; i < SLIDER_COUNT; i++ )
	{
		gtk_box_pack_start( GTK_BOX( phaser_box ), slider_box[i], TRUE, TRUE, 0 );
		gtk_widget_show( slider_box[i] );
	}
	gtk_container_add( GTK_CONTAINER( phaser_frame ), phaser_box );
	gtk_widget_show( phaser_box );

	gtk_box_pack_start( GTK_BOX( main_slider_box ), phaser_frame, TRUE, TRUE, 0 );
	gtk_widget_show( phaser_frame );

	btn_play = gtk_button_new_with_label( "Play" );
	g_signal_connect( btn_play, "clicked", G_CALLBACK( gui_play ), NULL );
	btn_export = gtk_button_new_with_label( "Export" );
	g_signal_connect( btn_export, "clicked", G_CALLBACK( gui_export ), NULL );
	btn_save = gtk_button_new_with_label( "Save Preset" );
	g_signal_connect( btn_save, "clicked", G_CALLBACK( gui_save_preset ), NULL );
	btn_load = gtk_button_new_with_label( "Load Preset" );
	g_signal_connect( btn_load, "clicked", G_CALLBACK( gui_load_preset ), NULL );
	
	info_label = gtk_label_new( "Welcome to Darabana!" );

	separator1 = gtk_hseparator_new();
	separator2 = gtk_hseparator_new();

	main_box = gtk_vbox_new( FALSE, 0 );
	gtk_box_pack_start( GTK_BOX( main_box ), graph, FALSE, FALSE, 0 );
	gtk_widget_show( graph );
	gtk_box_pack_start( GTK_BOX( main_box ), info_label, FALSE, FALSE, 5 );
	gtk_widget_show( info_label );
	gtk_box_pack_start( GTK_BOX( main_box ), separator2, FALSE, FALSE, 0 );
	gtk_widget_show( separator2 );
	gtk_box_pack_start( GTK_BOX( main_box ), main_slider_box, TRUE, TRUE, 2 );
	gtk_widget_show( main_slider_box );

	button_box = gtk_hbox_new( FALSE, 5 );
	gtk_box_pack_start( GTK_BOX( button_box ), btn_play, FALSE, FALSE, 0 );
	gtk_widget_show( btn_play );
	gtk_box_pack_end( GTK_BOX( button_box ), btn_export, FALSE, FALSE, 0 );
	gtk_widget_show( btn_export );
	gtk_box_pack_end( GTK_BOX( button_box ), btn_save, FALSE, FALSE, 0 );
	gtk_widget_show( btn_save );
	gtk_box_pack_end( GTK_BOX( button_box ), btn_load, FALSE, FALSE, 0 );
	gtk_widget_show( btn_load );

	gtk_box_pack_start( GTK_BOX( main_box ), button_box, FALSE, FALSE, 2 );
	gtk_widget_show( button_box );
	gtk_box_pack_start( GTK_BOX( main_box ), separator1, FALSE, FALSE, 5 );
	gtk_widget_show( separator1 );
	
	gtk_box_pack_end( GTK_BOX( main_box ), statusbar, FALSE, FALSE, 2 );
	gtk_widget_show( statusbar );

	gtk_container_add( GTK_CONTAINER( window ), main_box );
	gtk_widget_show( main_box );
	
	gtk_widget_show( window );
}

