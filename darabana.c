
/*	
 *	darabana.c
 *	darabana percussion synthesizer
 *	author: Vlad Dumitru
 *
 *	As long as you retain this notice, you may do whatever you want to do with
 *	this file. If we shall meet one day, and you would consider appropriate,
 *	I will gladly accept donations in beer.
*/

/*	TODO
 *	on err != PaNoError, raise GTK messagebox with error details
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>

#include <portaudio.h>
#include <sndfile.h>

#define SLIDER_COUNT 12
#define MAX_SAMPLE_LENGTH (44100)

#define CHANNEL_COUNT 1
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

//#define DEBUG

GtkWidget *window;
GtkWidget *statusbar;
GtkWidget *graph;
GdkPixmap *graph_pixmap;
GtkWidget *info_label;
cairo_t *cr;

float *main_buf = NULL;
int buf_length;
int buf_position = -1;

char length_info[256];

PaStream *stream;

char *slider_text[] = {
	"Amp.",
	"Decay",
	"Freq.",
	"Freq. decay",
	"Amp",
	"Decay",
	"Filter",
	"Ring",
	"Amp.",
	"Length",
	"Freq.",
	"Phase"
};

double adj_data[][6] = {
	{ 2, 0, 5.0, 0.05, 0.1, 3 },
	{ 0.9995, 0, 1, 0.01, 0.1, 3 },
	{ 210, 0, 2000, 1, 5 , 0 },
	{ 0.002, 0, 1, 0.01, 0.1, 3 },

	{ 1.0 , 0, 5, 0.05, 0.1, 3 },
	{ 0.9997, 0, 1, 0.01, 0.1, 3 },
	{ 0.5, 0, 1, 0.01, 0.1, 3 },
	{ 0, 0, 1, 0.01, 0.1, 3 },

	{ 5, 0, 10, 0.1, 0.5, 3 },
	{ 5, 0, 100, 1, 5, 1 },
	{ 0.5, 0, 1, 0.01, 0.1, 3 },
	{ 5, 0, 100, 1, 5, 1 }
};

float gen_data[SLIDER_COUNT];

int paCallback(	const void *inputBuffer, void *outputBuffer,
				unsigned long framesPerBuffer,
				const PaStreamCallbackTimeInfo *timeInfo,
				PaStreamCallbackFlags statusFlags,
				void *userData )
{
	(void) timeInfo;
	(void) statusFlags;
	(void) inputBuffer;
	(void) userData;

	float *out = (float*)outputBuffer;
	unsigned long i;

	for( i = 0; i < framesPerBuffer; i++ )
	{
		if( buf_position >= buf_length )
		{
			buf_position = -1;
			out[i] = 0.0f;
		}

		else if( buf_position >= 0 )
		{
			out[i] = main_buf[buf_position];
			buf_position++;
		}

		else
			out[i] = 0.0f;
	}

	return paContinue;
}

int generate_drum(	float amp, float decay, float freq, float freqdecay,
					float noise, float noisedecay, float noisefilter, float ring,
					float phaserlen, float phaserfreq, float phaseramp, float phaserphase
					)
{
	if( main_buf )
		free( main_buf );
	
	main_buf = malloc( sizeof(float) * MAX_SAMPLE_LENGTH );
	float *buf = main_buf;
	float f = 0;
	int i = 0;

	int _sampleRate = SAMPLE_RATE;

	if( decay == 0.0f )
		return 0;
	if( noisedecay == 0.0f )
		return 0;

	decay = 1.0f - pow( 10.0f, -sqrt( decay * 20.0f ) );
	freqdecay = pow( 10.0f, (freqdecay - 1.0f)*5.0f );
	noisedecay = 1.0f - pow( 10.0f, -sqrt( noisedecay * 20.0f ) );
	noisefilter = 1.0f - pow( 10.0f, -sqrt( noisefilter ) );
	(void) ring; /* TODO ring! */

#ifdef DEBUG
	printf( "decay: %f; freqdecay: %f; noisedecay: %f; noisefilter: %f\n", decay, freqdecay, noisedecay, noisefilter );
#endif

	while( 1 )
	{
		buf[i] = amp * sin( 2.0 * M_PI * freq * (double)i / (double)_sampleRate );
		f = (noisefilter) * f + (1.0f-noisefilter) * noise * ( (float)( rand()%1000 ) / 1000.0f - 0.5f );
		buf[i] += f;

		if( phaserlen > 0 )
		{
			if( i >= (int)phaserlen )
				buf[i] += phaseramp * cos( 2.0 * M_PI * phaserfreq * ((double)i+phaserphase) / (double)_sampleRate ) * buf[i-(int)phaserlen];
		}

		noise *= noisedecay;
		amp *= decay;
		freq -= freqdecay;

		if( noise < .0 ) noise = .0;
		if( amp < .0 ) amp = .0;
		if( freq < .0 ) freq = .0;
		if( buf[i] < -1.0 ) buf[i] = -1.0;
		if( buf[i] >  1.0 ) buf[i] =  1.0;

		if( ( amp < .001 ) && ( noise < .001 ) )
			break;

		if( i > MAX_SAMPLE_LENGTH )
			break;
		
		i++;
	}
	
	return i;
}

static void change_param( GtkAdjustment *a, gpointer data )
{
#ifdef DEBUG
	printf( "%i: %f\n", (int)data, a->value );
#endif

	gen_data[(int)data] = a->value;

	buf_length = generate_drum(
		gen_data[0], gen_data[1], gen_data[2], gen_data[3],
		gen_data[4], gen_data[5], gen_data[6], gen_data[7],
		gen_data[8], gen_data[9], gen_data[10], gen_data[11] );

	gdk_window_invalidate_rect( GDK_WINDOW( graph->window ), NULL, FALSE );

	snprintf( length_info, 256, "Length: %i samples (%f seconds).", buf_length, (float)buf_length/(float)SAMPLE_RATE );
	gtk_label_set_text( GTK_LABEL( info_label ), length_info );
}

static void gui_play( GtkWidget *w, gpointer data )
{
	(void) w;
	(void) data;

	buf_position = 0;

	gtk_statusbar_push( GTK_STATUSBAR( statusbar ), 0, "Sample replay finished." );
}

static void gui_export( GtkWidget *w, gpointer data )
{
	(void) w;
	(void) data;

	GtkWidget *export_dialog;
	char *fn;
	SNDFILE *sf;
	SF_INFO si;

	si.samplerate = SAMPLE_RATE;
	si.channels = CHANNEL_COUNT;
	si.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

	export_dialog = gtk_file_chooser_dialog_new( "Export Wave", GTK_WINDOW( window ),
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL );
	
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

static gboolean delete_event( GtkWidget *w, GdkEvent *e, gpointer data )
{
	(void) w;
	(void) e;
	(void) data;

	return FALSE;
}

static void destroy( GtkWidget *w, gpointer data )
{
	(void) w;
	(void) data;

	gtk_main_quit();
}

static gboolean draw_graph( GtkWidget *w, GdkEventExpose *e )
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

static void configure_graph( GtkWidget *w, gpointer data )
{
	(void) data;

	if( graph_pixmap )
		gdk_pixmap_unref( graph_pixmap );
	
	graph_pixmap = gdk_pixmap_new( w->window, w->allocation.width, w->allocation.height, -1 );

	gdk_draw_rectangle( graph_pixmap, w->style->black_gc, TRUE, 0, 0, w->allocation.width, w->allocation.height );
}

void make_gui()
{
	GtkWidget *main_box;
	GtkWidget *main_slider_box;
	GtkWidget *button_box;

	GtkObject *adj[SLIDER_COUNT];
	GtkWidget *slider[SLIDER_COUNT];
	GtkWidget *slider_box[SLIDER_COUNT];
	GtkWidget *slider_label[SLIDER_COUNT];

	GtkWidget *btn_play, *btn_export;
	GtkWidget *separator1, *separator2;

	GtkWidget *tone_frame, *noise_frame, *phaser_frame;
	GtkWidget *tone_box, *noise_box, *phaser_box;

	srand( time( 0 ) );

	window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_default_size( GTK_WINDOW( window ), 600, 320 );
	gtk_window_set_title( GTK_WINDOW( window ), "Darabana lu' Dumitru" );
	g_signal_connect( window, "delete-event", G_CALLBACK( delete_event ), NULL );
	g_signal_connect( window, "destroy", G_CALLBACK( destroy ), NULL );
	gtk_container_set_border_width( GTK_CONTAINER( window ), 2 );

	statusbar = gtk_statusbar_new();
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
	for( i = 4; i < 8; i++ )
	{
		gtk_box_pack_start( GTK_BOX( noise_box ), slider_box[i], TRUE, TRUE, 0 );
		gtk_widget_show( slider_box[i] );
	}
	gtk_container_add( GTK_CONTAINER( noise_frame ), noise_box );
	gtk_widget_show( noise_box );

	gtk_box_pack_start( GTK_BOX( main_slider_box ), noise_frame, TRUE, TRUE, 0 );
	gtk_widget_show( noise_frame );

	phaser_box = gtk_hbox_new( TRUE, 0 );
	for( i = 8; i < SLIDER_COUNT; i++ )
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
	gtk_box_pack_start( GTK_BOX( main_box ), main_slider_box, TRUE, TRUE, 0 );
	gtk_widget_show( main_slider_box );

	button_box = gtk_hbox_new( FALSE, 0 );
	gtk_box_pack_start( GTK_BOX( button_box ), btn_play, FALSE, FALSE, 0 );
	gtk_widget_show( btn_play );
	gtk_box_pack_start( GTK_BOX( button_box ), btn_export, FALSE, FALSE, 0 );
	gtk_widget_show( btn_export );

	gtk_box_pack_start( GTK_BOX( main_box ), separator1, FALSE, FALSE, 0 );
	gtk_widget_show( separator1 );
	gtk_box_pack_start( GTK_BOX( main_box ), button_box, FALSE, FALSE, 0 );
	gtk_widget_show( button_box );

	gtk_box_pack_end( GTK_BOX( main_box ), statusbar, FALSE, FALSE, 0 );
	gtk_widget_show( statusbar );

	gtk_container_add( GTK_CONTAINER( window ), main_box );
	gtk_widget_show( main_box );
	
	gtk_widget_show( window );
}

void init_pa( void )
{
	PaStreamParameters outputParameters;
	PaError err;

	err = Pa_Initialize();
	/* TODO check err != paNoError */

	outputParameters.device = Pa_GetDefaultOutputDevice();
	/* TODO check for paNoDevice */

	outputParameters.channelCount = CHANNEL_COUNT;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream( &stream, NULL, &outputParameters, SAMPLE_RATE,
		FRAMES_PER_BUFFER, paClipOff, paCallback, NULL );
	/* TODO check err != paNoError */

	err = Pa_StartStream( stream );
}

void terminate_pa( void )
{
	PaError err;

	err = Pa_StopStream( stream );
	/* TODO check err != paNoError */
	err = Pa_CloseStream( stream );
	/* TODO check err != paNoError */
	Pa_Terminate();
}

int main( int argc, char** argv )
{
	int i;
	for( i = 0; i < SLIDER_COUNT; i++ )
		gen_data[i] = adj_data[i][0];

	buf_length = generate_drum(
		gen_data[0], gen_data[1], gen_data[2], gen_data[3],
		gen_data[4], gen_data[5], gen_data[6], gen_data[7],
		gen_data[8], gen_data[9], gen_data[10], gen_data[11] );

	gtk_init( &argc, &argv );
	init_pa();
	make_gui();
	gtk_statusbar_push( GTK_STATUSBAR( statusbar ), 0, "Welcome to Darabana!" );
	gtk_main();

	terminate_pa();
	free( main_buf );

	return 0;
}
