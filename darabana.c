
/*	
 *	darabana.c
 *	darabana percussion synthesizer
 *	author: Vlad Dumitru
 *
 *	As long as you retain this notice, you may do whatever you want to do with
 *	this file. If we shall meet one day, and you would consider appropriate,
 *	I will gladly accept donations in beer.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>

#include <portaudio.h>

#include "darabana.h"

PaStream *stream;

/*	the PortAudio callback function;
 *	if a sample is playing, output the next framesPerBuffer samples, stopping
 *	when reaching the end of the sample;
 *	if a sample is not playing, output silence
*/
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

void init_pa( void )
{
	PaStreamParameters outputParameters;
	PaError err;

	err = Pa_Initialize();
	if( err != paNoError )
		raise_error( "Unable to initialize PortAudio." );

	outputParameters.device = Pa_GetDefaultOutputDevice();
	if( outputParameters.device == paNoDevice )
		raise_error( "No audio devices found." );

	outputParameters.channelCount = CHANNEL_COUNT;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream( &stream, NULL, &outputParameters, SAMPLE_RATE,
		FRAMES_PER_BUFFER, paClipOff, paCallback, NULL );
	if( err != paNoError )
		raise_error( "Unable to open PortAudio stream." );

	err = Pa_StartStream( stream );
	if( err != paNoError )
		raise_error( "Unable to start PortAudio stream." );
}

void terminate_pa( void )
{
	PaError err;

	err = Pa_StopStream( stream );
	if( err != paNoError )
		raise_error( "Unable to stop PortAudio stream." );
	
	err = Pa_CloseStream( stream );
	if( err != paNoError )
		raise_error( "Unable to close PortAudio stream." );
	
	Pa_Terminate();
}

int main( int argc, char** argv )
{
	/* init parameters with default values */
	int i;
	for( i = 0; i < SLIDER_COUNT; i++ )
		gen_data[i] = adj_data[i][0];

	/* generate initial, example instrument */
	buf_length = generate_drum(
		gen_data[0], gen_data[1], gen_data[2], gen_data[3],
		gen_data[4], gen_data[5], gen_data[6], gen_data[7],
		gen_data[8], gen_data[9], gen_data[10] );

	gtk_init( &argc, &argv );
	init_pa();
	make_gui();
	gtk_main();

	terminate_pa();
	free( main_buf );

	return 0;
}

