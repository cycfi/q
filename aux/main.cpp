#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	<sndfile.h>

#ifndef		M_PI
#define		M_PI		3.14159265358979323846264338
#endif

#define		SAMPLE_RATE			44100
#define		SAMPLE_COUNT		(SAMPLE_RATE * 4)	/* 4 seconds */
#define		AMPLITUDE			(1.0 * 0x7F000000)
#define		LEFT_FREQ			(344.0 / SAMPLE_RATE)
#define		RIGHT_FREQ			(466.0 / SAMPLE_RATE)

int
main (void)
{	SNDFILE	*file ;
	SF_INFO	sfinfo ;
	int		k ;
	int	*buffer ;

	if (! (buffer = (int*) malloc (2 * SAMPLE_COUNT * sizeof (int))))
	{	printf ("Error : Malloc failed.\n") ;
		return 1 ;
		} ;

	memset (&sfinfo, 0, sizeof (sfinfo)) ;

	sfinfo.samplerate	= SAMPLE_RATE ;
	sfinfo.frames		= SAMPLE_COUNT ;
	sfinfo.channels		= 2 ;
	sfinfo.format		= (SF_FORMAT_WAV | SF_FORMAT_PCM_24) ;

	if (! (file = sf_open ("sine.wav", SFM_WRITE, &sfinfo)))
	{	printf ("Error : Not able to open output file.\n") ;
		free (buffer) ;
		return 1 ;
		} ;

	if (sfinfo.channels == 1)
	{	for (k = 0 ; k < SAMPLE_COUNT ; k++)
			buffer [k] = AMPLITUDE * sin (LEFT_FREQ * 2 * k * M_PI) ;
		}
	else if (sfinfo.channels == 2)
	{	for (k = 0 ; k < SAMPLE_COUNT ; k++)
		{	buffer [2 * k] = AMPLITUDE * sin (LEFT_FREQ * 2 * k * M_PI) ;
			buffer [2 * k + 1] = AMPLITUDE * sin (RIGHT_FREQ * 2 * k * M_PI) ;
			} ;
		}
	else
	{	printf ("Error : make_sine can only generate mono or stereo files.\n") ;
		sf_close (file) ;
		free (buffer) ;
		return 1 ;
		} ;

	if (sf_write_int (file, buffer, sfinfo.channels * SAMPLE_COUNT) !=
											sfinfo.channels * SAMPLE_COUNT)
		puts (sf_strerror (file)) ;

	sf_close (file) ;
	free (buffer) ;
	return 0 ;
} /* main */