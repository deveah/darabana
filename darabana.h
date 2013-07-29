
/*
 *	darabana.h
 *	header file for darabana percussion synthesizer
 *	author: Vlad Dumitru
 *
 *	license information in darabana.c
*/

#ifndef _DARABANA_H_
#define _DARABANA_H_

#include <gtk/gtk.h>
#include <portaudio.h>
#include <sndfile.h>

#define SLIDER_COUNT 11
#define MAX_SAMPLE_LENGTH (44100)

#define CHANNEL_COUNT 1
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define OUTPUT_FORMAT (SF_FORMAT_WAV|SF_FORMAT_FLOAT)

/*#define DEBUG*/

/* the PortAudio callback function */
int paCallback(	const void *inputBuffer, void *outputBuffer,
				unsigned long framesPerBuffer,
				const PaStreamCallbackTimeInfo *timeInfo,
				PaStreamCallbackFlags statusFlags,
				void *userData );

/* the moneymaking function */
int generate_drum(	float amp, float decay, float freq, float freqdecay,
					float noise, float noisedecay, float noisefilter,
					float phaserlen, float phaserfreq, float phaseramp,
					float phaserphase );

/* gui-related functions */
void change_param( GtkAdjustment *a, gpointer data );
void gui_play( GtkWidget *w, gpointer data );
void gui_export( GtkWidget *w, gpointer data );
gboolean delete_event( GtkWidget *w, GdkEvent *e, gpointer data );
void destroy( GtkWidget *w, gpointer data );
gboolean draw_graph( GtkWidget *w, GdkEventExpose *e );
void configure_graph( GtkWidget *w, gpointer data );
void raise_error( char* s );
void raise_warning( char* s );
void make_gui();

/* initialization and termination functions for PortAudio */
void init_pa( void );
void terminate_pa( void );

/* main sound buffer data */
extern float *main_buf;
extern int buf_length;
extern int buf_position;

/* slider values */
extern float gen_data[SLIDER_COUNT];

extern double adj_data[SLIDER_COUNT][6];

#endif
