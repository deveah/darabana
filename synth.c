
#include <stdlib.h>
#include <math.h>

#include "darabana.h"

float *main_buf = NULL;
int buf_length = 0;
int buf_position = -1;

/* the heart of darabana - the actual drum generating function */
int generate_drum(	float amp, float decay, float freq, float freqdecay,
					float noise, float noisedecay, float noisefilter,
					float phaserlen, float phaserfreq, float phaseramp,
					float phaserphase )
{
	/* if a sample already exists, destroy it */
	if( main_buf )
		free( main_buf );
	
	/* allocate space for the new sample */
	main_buf = malloc( sizeof(float) * MAX_SAMPLE_LENGTH );
	float *buf = main_buf;
	float f = 0;
	int i = 0;

	/* TODO: needed? */
	int _sampleRate = SAMPLE_RATE;

	/* sanity checks */
	if( decay == 0.0f )
		return 0;
	if( noisedecay == 0.0f )
		return 0;

	/* conversion from linear scale, in order to make sensitive areas of the
	 * scale more accessible */
	decay = 1.0f - pow( 10.0f, -sqrt( decay * 20.0f ) );
	freqdecay = pow( 10.0f, (freqdecay - 1.0f)*5.0f );
	noisedecay = 1.0f - pow( 10.0f, -sqrt( noisedecay * 20.0f ) );
	noisefilter = 1.0f - pow( 10.0f, -sqrt( noisefilter ) );

#ifdef DEBUG
	printf( "decay: %f; freqdecay: %f; noisedecay: %f; noisefilter: %f\n",
		decay, freqdecay, noisedecay, noisefilter );
#endif

	while( 1 )
	{
		/* the main oscillators - sine and noise */
		buf[i] = amp * sin( 2.0 * M_PI * freq * (double)i / (double)_sampleRate );
		/* white noise with a one-pole low pass filter */
		f = (noisefilter) * f + (1.0f-noisefilter) * noise *
			( (float)( rand()%1000 ) / 1000.0f - 0.5f );
		buf[i] += f;

		/* the phaser is actually a comb filter; it simulates the acoustic
		 * chamber of the percussion instrument by overlapping phase-offset
		 * waves of the same sound */
		if( phaserlen > 0 )
		{
			if( i >= (int)phaserlen )
				buf[i] += phaseramp * cos( 2.0 * M_PI * phaserfreq *
					((double)i+phaserphase) / (double)_sampleRate ) *
					buf[i-(int)phaserlen];
		}

		/* update amplitudes and sine frequency */
		noise *= noisedecay;
		amp *= decay;
		freq -= freqdecay;

		/* these values cannot dive below 0 */
		if( noise < .0 ) noise = .0;
		if( amp < .0 ) amp = .0;
		if( freq < .0 ) freq = .0;
		/* clipping */
		if( buf[i] < -1.0 ) buf[i] = -1.0;
		if( buf[i] >  1.0 ) buf[i] =  1.0;

		/* if sine or noise amplitude reach values that no longer produce
		 * hearable sounds, the sample is considered finished */
		if( ( amp < .001 ) && ( noise < .001 ) )
			break;

		i++;

		if( i >= MAX_SAMPLE_LENGTH )
			break;
	}
	
	return i;
}

