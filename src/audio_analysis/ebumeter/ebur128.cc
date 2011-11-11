// -------------------------------------------------------------------------
//
//    Copyright (C) 2010-2011 Fons Adriaensen <fons@linuxaudio.org>
//    
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// -------------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <math.h>
#include "ebu_r128_proc.h"
#include "ebur128.h"

extern "C" {
#include "audio_decoder/ad.h"
}

const bool prob = false;
const bool lufs = false;

static int ebur128proc (const char *fn, struct ebur128 *ebr) {
	void *sf;
	int i, k;
	struct adinfo nfo;
	Ebu_r128_proc  Proc;
	float *data[2];
	float *inpb;

	if (!(sf=ad_open(fn, &nfo))) {
		fprintf (stderr, "Can't open input file '%s'.\n", fn);
		return 1;
	}

	const int nchan = nfo.channels;
	const float fsamp = nfo.sample_rate;
	if (nchan > 2) {
		fprintf (stderr, "Input file must be mono or stereo.\n");
		ad_close(sf);
		return 1;
	}

	const int bsize = fsamp / 5;
	inpb = new float [nchan * bsize];
	data [0] = new float [bsize];
	data [1] = new float [bsize];

	Proc.init (nchan, fsamp);
	Proc.integr_start ();
	while (true) {
		k = ad_read (sf, inpb, bsize);
		if (k == 0) break;
		if (nchan > 1) {
	    float *p = inpb;
	    for (i = 0; i < k; i++) {
				data [0][i] = *p++;
				data [1][i] = *p++;
	    }
		} else {
	    float *p = inpb;
	    for (i = 0; i < k; i++) {
				data [0][i] = *p;
				data [1][i] = *p++;
	    }
		}
		Proc.process (k, data);
	}
	ad_close(sf);

	delete[] data [0];
	delete[] data [1];
	delete[] inpb;

	if (ebr) {
		if (lufs) {
			ebr->lufs = 1;
			ebr->integrated = Proc.integrated ();
			ebr->range      = Proc.range_max () - Proc.range_min ();
			ebr->integ_thr  = Proc.integ_thr ();
			ebr->range_thr  = Proc.range_thr ();
			ebr->range_min  = Proc.range_min ();
			ebr->range_max  = Proc.range_max ();
			ebr->maxloudn_M = Proc.maxloudn_M ();
			ebr->maxloudn_S = Proc.maxloudn_S ();
		} else {
			ebr->lufs = 0;
			ebr->integrated = Proc.integrated () + 23.0f;
			ebr->range      = Proc.range_max () - Proc.range_min ();
			ebr->integ_thr  = Proc.integ_thr () + 23.0f;
			ebr->range_thr  = Proc.range_thr () + 23.0f;
			ebr->range_min  = Proc.range_min () + 23.0f;
			ebr->range_max  = Proc.range_max () + 23.0f;
			ebr->maxloudn_M = Proc.maxloudn_M () + 23.0f;
			ebr->maxloudn_S = Proc.maxloudn_S () + 23.0f;
		}
	}

#if 1 /* dump to stdout */
	printf ("ebu_r128 - file: '%s'\n", fn);
	if (lufs) {
		printf ("Integrated loudness:   %6.1lf LUFS\n", Proc.integrated ());
		printf ("Loudness range:        %6.1lf LU\n", Proc.range_max () - Proc.range_min ());
		printf ("Integrated threshold:  %6.1lf LUFS\n", Proc.integ_thr ());
		printf ("Range threshold:       %6.1lf LUFS\n", Proc.range_thr ());
		printf ("Range min:             %6.1lf LUFS\n", Proc.range_min ());
		printf ("Range max:             %6.1lf LUFS\n", Proc.range_max ());
		printf ("Momentary max:         %6.1lf LUFS\n", Proc.maxloudn_M ());
		printf ("Short term max:        %6.1lf LUFS\n", Proc.maxloudn_S ());
	} else {
		printf ("Integrated loudness:   %6.1lf LU\n", Proc.integrated () + 23.0f);
		printf ("Loudness range:        %6.1lf LU\n", Proc.range_max () - Proc.range_min ());
		printf ("Integrated threshold:  %6.1lf LU\n", Proc.integ_thr () + 23.0f);
		printf ("Range threshold:       %6.1lf LU\n", Proc.range_thr () + 23.0f);
		printf ("Range min:             %6.1lf LU\n", Proc.range_min () + 23.0f);
		printf ("Range max:             %6.1lf LU\n", Proc.range_max () + 23.0f);
		printf ("Momentary max:         %6.1lf LU\n", Proc.maxloudn_M () + 23.0f);
		printf ("Short term max:        %6.1lf LU\n", Proc.maxloudn_S () + 23.0f);
	}
#endif

#if 0
	if (prob) {
		int nm, ns;
		float km, ks, v;
		const int *hm, *hs;
		km = Proc.hist_M_count ();
		ks = Proc.hist_S_count ();
		if ((km < 10) || (ks < 10)) {
			fprintf (stderr, "Insufficient data for probability file.\n");
		} else {
			FILE *F;
			F = fopen ("ebur128-prob", "w");
	    if (F) {
				hm = Proc.histogram_M ();
				hs = Proc.histogram_S ();
				nm = 0;
				ns = 0;
				for (i = 0; i <= 750; i++) {
					nm += hm [i];
					ns += hs [i];
					v = 0.1f * (i - 700);
					if (! lufs) v += 23.0f;
					fprintf (F, "%5.1lf %8.6lf %8.6lf\n", v, nm / km, ns / ks);
				}
				fclose (F);
	    } else {
				fprintf (stderr, "Can't open probability data file.\n");
	    }
		}
	}
#endif
	return 0;
}

extern "C" {
	int ebur128analyse (const char *fn, struct ebur128 *e) {
		return ebur128proc(fn, e);
	}
}