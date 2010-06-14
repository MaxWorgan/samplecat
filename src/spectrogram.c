/*
** Copyright (C) 2007-2009 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 or version 3 of the
** License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	Generate a spectrogram as a PNG file from a given sound file.
*/

/*
**	Todo:
**      - Decouple height of image from FFT length. FFT length should be
*         greater than height and then interpolated to height.
**      - Make magnitude to colour mapper allow abitrary scaling (ie cmdline
**        arg).
**      - Better cmdline arg parsing and flexibility.
**      - Add option to do log frequency scale.
*/

#define _ISOC99_SOURCE
#define _XOPEN_SOURCE

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sndfile.h>
#include <cairo.h>
#include <fftw3.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "typedefs.h"
#include "support.h"
#include "sndfile_common.h"
#include "sndfile_window.h"

#include "main.h"

extern unsigned debug;

#ifdef USE_OPENGL
typedef struct _GLSpectrogram GLSpectrogram;
extern void gl_spectrogram_image_ready (GLSpectrogram* self, GdkPixbuf* _pixbuf);
#else
typedef struct _SpectrogramWidget SpectrogramWidget;
extern void spectrogram_widget_image_ready (SpectrogramWidget* self, GdkPixbuf* _pixbuf);
#endif

typedef void (*SpectrogramReady)(const char* filename, GdkPixbuf*, gpointer);


#define TIMER_STOP FALSE

#define MIN_WIDTH 640
#define MIN_HEIGHT 480
#define MAX_WIDTH 8192
#define MAX_HEIGHT 4096

#define TICK_LEN 6
#define BORDER_LINE_WIDTH 1.8

#define TITLE_FONT_SIZE 20.0
#define NORMAL_FONT_SIZE 12.0

#define	LEFT_BORDER			65.0
#define	TOP_BORDER			30.0
#define	RIGHT_BORDER		75.0
#define	BOTTOM_BORDER		40.0

//#define	SPEC_FLOOR_DB		-180.0
#define	SPEC_FLOOR_DB		-90.0


typedef struct
{	const char*        sndfilepath;
	const char*        filename;
	int                width, height;
	bool               border, log_freq;
	double             spec_floor_db;
	GdkPixbuf*         pixbuf;
#ifdef USE_OPENGL
	GLSpectrogram*     w; //hopefully temp
#else
	SpectrogramWidget* w; //hopefully temp
#endif
	SpectrogramReady   callback;
	gpointer           user_data;
} RENDER;

typedef struct 
{
	int left, top, width, height;
} RECT;

static const char font_family [] = "Terminus";

static void
get_colour_map_value (float value, double spec_floor_db, unsigned char colour [3])
{	static unsigned char map [][3] =
	{	/* These values were originally calculated for a dynamic range of 180dB. */
		{	255, 255, 255 },  /* -0dB */
		{	240, 254, 216 },  /* -10dB */
		{	242, 251, 185 },  /* -20dB */
		{	253, 245, 143 },  /* -30dB */
		{	253, 200, 102 },  /* -40dB */
		{	252, 144,  66 },  /* -50dB */
		{	252,  75,  32 },  /* -60dB */
		{	237,  28,  41 },  /* -70dB */
		{	214,   3,  64 },  /* -80dB */
		{	183,   3, 101 },  /* -90dB */
		{	157,   3, 122 },  /* -100dB */
		{	122,   3, 126 },  /* -110dB */
		{	 80,   2, 110 },  /* -120dB */
		{	 45,   2,  89 },  /* -130dB */
		{	 19,   2,  70 },  /* -140dB */
		{	  1,   3,  53 },  /* -150dB */
		{	  1,   3,  37 },  /* -160dB */
		{	  1,   2,  19 },  /* -170dB */
		{	  0,   0,   0 },  /* -180dB */
	} ;

	float rem ;
	int indx ;

	if (value >= 0.0)
	{	colour = map [0] ;
		return;
	};

	value = fabs (value * (-180.0 / spec_floor_db) * 0.1);

	indx = lrintf (floor (value)) ;

	if (indx < 0) {
		printf ("\nError : colour map array index is %d\n\n", indx);
		exit (1) ;
	};

	if (indx >= G_N_ELEMENTS (map) - 1) {
		colour = map [G_N_ELEMENTS (map) - 1] ;
		return ;
	};

	rem = fmod (value, 1.0) ;

	colour [0] = lrintf ((1.0 - rem) * map [indx][0] + rem * map [indx + 1][0]) ;
	colour [1] = lrintf ((1.0 - rem) * map [indx][1] + rem * map [indx + 1][1]) ;
	colour [2] = lrintf ((1.0 - rem) * map [indx][2] + rem * map [indx + 1][2]) ;

	return ;
} /* get_colour_map_value */


static void
read_mono_audio (SNDFILE * file, sf_count_t filelen, double * data, int datalen, int indx, int total)
{
	sf_count_t start ;

	memset (data, 0, datalen * sizeof (data [0])) ;

	start = (indx * filelen) / total - datalen / 2 ;

	if (start >= 0)
		sf_seek (file, start, SEEK_SET) ;
	else
	{	start = -start ;
		sf_seek (file, 0, SEEK_SET) ;
		data += start ;
		datalen -= start ;
		} ;

	sfx_mix_mono_read_double (file, data, datalen) ;

	return ;
} /* read_mono_audio */

static void
apply_window (double * data, int datalen)
{
	static double window [10 * MAX_HEIGHT] ;
	static int window_len = 0 ;
	int k ;

	if (window_len != datalen)
	{
		window_len = datalen ;
		if (datalen > G_N_ELEMENTS (window))
		{
			printf ("%s : datalen >  MAX_HEIGHT\n", __func__) ;
			exit (1) ;
		} ;

		calc_kaiser_window (window, datalen, 20.0) ;
	} ;

	for (k = 0 ; k < datalen ; k++)
		data [k] *= window [k] ;

	return ;
} /* apply_window */

static double
calc_magnitude (const double * freq, int freqlen, double * magnitude)
{
	int k ;
	double max = 0.0 ;

	for (k = 1 ; k < freqlen / 2 ; k++)
	{	magnitude [k] = sqrt (freq [k] * freq [k] + freq [freqlen - k - 1] * freq [freqlen - k - 1]) ;
		max = MAX (max, magnitude [k]) ;
		} ;
	magnitude [0] = 0.0 ;

	return max ;
} /* calc_magnitude */

#ifdef NEVER
static void
_render_spectrogram (cairo_surface_t * surface, double spec_floor_db, float mag2d [MAX_WIDTH][MAX_HEIGHT], double maxval, double left, double top, double width, double height)
{
	dbg(0, "...");
	unsigned char colour [3] = { 0, 0, 0 } ;
	double linear_spec_floor ;
	int w, h;

	int stride = cairo_image_surface_get_stride (surface);

	unsigned char* data = cairo_image_surface_get_data (surface);
	memset (data, 0, stride * cairo_image_surface_get_height (surface)) ;
	dbg(0, "height=%i stride=%i", cairo_image_surface_get_height (surface), stride);

	linear_spec_floor = pow (10.0, spec_floor_db / 20.0) ;

	for (w = 0 ; w < width ; w ++) {
		for (h = 0 ; h < height ; h++) {

			mag2d [w][h] = mag2d [w][h] / maxval ;
			mag2d [w][h] = (mag2d [w][h] < linear_spec_floor) ? spec_floor_db : 20.0 * log10 (mag2d [w][h]) ;

			get_colour_map_value (mag2d [w][h], spec_floor_db, colour);

			int y = height + top - 1 - h;
			int x = (w + left) * 4;
			data [y * stride + x + 0] = colour [2];
			data [y * stride + x + 1] = colour [1];
			data [y * stride + x + 2] = colour [0];
			data [y * stride + x + 3] = 0;
		};
	}
	cairo_surface_mark_dirty (surface);
}
#endif

static void
_render_spectrogram_to_pixbuf (GdkPixbuf* pixbuf, double spec_floor_db, float mag2d [MAX_WIDTH][MAX_HEIGHT], double maxval, double left, double top, double width, double height)
{
	unsigned char colour [3] = { 0, 0, 0 } ;
	double linear_spec_floor ;

	int stride = gdk_pixbuf_get_rowstride (pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels(pixbuf);

	unsigned char* data = gdk_pixbuf_get_pixels(pixbuf);

	linear_spec_floor = pow (10.0, spec_floor_db / 20.0) ;

	int w, h;
	for (w = 0 ; w < width ; w ++) {
		for (h = 0 ; h < height ; h++) {

			mag2d [w][h] = mag2d [w][h] / maxval ;
			mag2d [w][h] = (mag2d [w][h] < linear_spec_floor) ? spec_floor_db : 20.0 * log10 (mag2d [w][h]) ;

			get_colour_map_value (mag2d [w][h], spec_floor_db, colour);

			int y = height + top - 1 - h;
			int x = (w + left) * n_channels;
			data [y * stride + x + 0] = colour [2];
			data [y * stride + x + 1] = colour [1];
			data [y * stride + x + 2] = colour [0];
			//data [y * stride + x + 3] = 0;
		};
	}
}

#ifdef NEVER
static void
render_heat_map (cairo_surface_t * surface, double magfloor, const RECT *r)
{
	unsigned char colour [3], *data ;
	int w, h, stride ;

	stride = cairo_image_surface_get_stride (surface) ;
	data = cairo_image_surface_get_data (surface) ;

	for (h = 0 ; h < r->height ; h++)
	{	get_colour_map_value (magfloor * (r->height - h) / (r->height + 1), magfloor, colour) ;

		for (w = 0 ; w < r->width ; w ++)
		{	int x, y ;

			x = (w + r->left) * 4 ;
			y = r->height + r->top - 1 - h ;

			data [y * stride + x + 0] = colour [2] ;
			data [y * stride + x + 1] = colour [1] ;
			data [y * stride + x + 2] = colour [0] ;
			data [y * stride + x + 3] = 0 ;
			} ;
		} ;

	cairo_surface_mark_dirty (surface) ;
}
#endif

static inline void
x_line (cairo_t * cr, double x, double y, double len)
{	cairo_move_to (cr, x, y) ;
	cairo_rel_line_to (cr, len, 0.0) ;
	cairo_stroke (cr) ;
} /* x_line */

static inline void
y_line (cairo_t * cr, double x, double y, double len)
{	cairo_move_to (cr, x, y) ;
	cairo_rel_line_to (cr, 0.0, len) ;
	cairo_stroke (cr) ;
} /* y_line */

typedef struct
{	double value [15] ;
	double distance [15] ;
} TICKS ;

static inline int
calculate_ticks (double max, double distance, TICKS * ticks)
{	const int div_array [] =
	{	10, 10, 8, 6, 8, 10, 6, 7, 8, 9, 10, 11, 12, 12, 7, 14, 8, 8, 9
		} ;

	double scale = 1.0, scale_max ;
	int k, leading, divisions ;

	if (max < 0)
	{	printf ("\nError in %s : max < 0\n\n", __func__) ;
		exit (1) ;
		} ;

	while (scale * max >= G_N_ELEMENTS (div_array))
		scale *= 0.1 ;

	while (scale * max < 1.0)
		scale *= 10.0 ;

	leading = lround (scale * max) ;
	divisions = div_array [leading % G_N_ELEMENTS (div_array)] ;

	/* Scale max down. */
	scale_max = leading / scale ;
	scale = scale_max / divisions ;

	if (divisions > G_N_ELEMENTS (ticks->value) - 1)
	{	printf ("Error : divisions (%d) > G_N_ELEMENTS (ticks->value) (%d)\n", divisions, G_N_ELEMENTS (ticks->value)) ;
		exit (1) ;
		} ;

	for (k = 0 ; k <= divisions ; k++)
	{	ticks->value [k] = k * scale ;
		ticks->distance [k] = distance * ticks->value [k] / max ;
		} ;

	return divisions + 1 ;
} /* calculate_ticks */

#ifdef NEVER
static void
str_print_value (char * text, int text_len, double value)
{
	if (fabs (value) < 1e-10)
		snprintf (text, text_len, "0") ;
	else if (fabs (value) >= 10.0)
		snprintf (text, text_len, "%1.0f", value) ;
	else if (fabs (value) >= 1.0)
		snprintf (text, text_len, "%3.1f", value) ;
	else
		snprintf (text, text_len, "%4.2f", value) ;

	return ;
}


static void
render_spect_border (cairo_surface_t* surface, const char* filename, double left, double width, double seconds, double top, double height, double max_freq)
{
	char text [512];
	cairo_text_extents_t extents;
	cairo_matrix_t matrix;

	TICKS ticks;
	int k, tick_count;

	cairo_t* cr = cairo_create (surface);

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_set_line_width (cr, BORDER_LINE_WIDTH);

	/* Print title. */
	cairo_select_font_face (cr, font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 1.0 * TITLE_FONT_SIZE);

	snprintf (text, sizeof (text), "Spectrogram: %s", filename);
	cairo_text_extents (cr, text, &extents);
	cairo_move_to (cr, left + 2, top - extents.height / 2);
	cairo_show_text (cr, text);

	/* Print labels. */
	cairo_select_font_face (cr, font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL) ;
	cairo_set_font_size (cr, 1.0 * NORMAL_FONT_SIZE) ;

	/* Border around actual spectrogram. */
	cairo_rectangle (cr, left, top, width, height) ;

	tick_count = calculate_ticks (seconds, width, &ticks) ;
	for (k = 0 ; k < tick_count ; k++)
	{	y_line (cr, left + ticks.distance [k], top + height, TICK_LEN) ;
		if (k % 2 == 1)
			continue ;
		str_print_value (text, sizeof (text), ticks.value [k]) ;
		cairo_text_extents (cr, text, &extents) ;
		cairo_move_to (cr, left + ticks.distance [k] - extents.width / 2, top + height + 8 + extents.height) ;
		cairo_show_text (cr, text) ;
		} ;

	tick_count = calculate_ticks (max_freq, height, &ticks) ;
	for (k = 0 ; k < tick_count ; k++)
	{	x_line (cr, left + width, top + height - ticks.distance [k], TICK_LEN) ;
		if (k % 2 == 1)
			continue ;
		str_print_value (text, sizeof (text), ticks.value [k]) ;
		cairo_text_extents (cr, text, &extents) ;
		cairo_move_to (cr, left + width + 12, top + height - ticks.distance [k] + extents.height / 4.5) ;
		cairo_show_text (cr, text) ;
		} ;

	cairo_set_font_size (cr, 1.0 * NORMAL_FONT_SIZE) ;

	/* Label X axis. */
	snprintf (text, sizeof (text), "Time (secs)") ;
	cairo_text_extents (cr, text, &extents) ;
	cairo_move_to (cr, left + (width - extents.width) / 2, cairo_image_surface_get_height (surface) - 8) ;
	cairo_show_text (cr, text) ;

	/* Label Y axis (rotated). */
	snprintf (text, sizeof (text), "Frequency (Hz)") ;
	cairo_text_extents (cr, text, &extents) ;

	cairo_get_font_matrix (cr, &matrix) ;
	cairo_matrix_rotate (&matrix, -0.5 * M_PI) ;
	cairo_set_font_matrix (cr, &matrix) ;

	cairo_move_to (cr, cairo_image_surface_get_width (surface) - 12, top + (height + extents.width) / 2) ;
	cairo_show_text (cr, text) ;

	cairo_destroy (cr) ;
}


static void
render_heat_border (cairo_surface_t * surface, double magfloor, const RECT *r)
{
	const char *decibels = "dB" ;
	char text [512] ;
	cairo_t * cr ;
	cairo_text_extents_t extents ;
	TICKS ticks ;
	int k, tick_count ;

	cr = cairo_create (surface) ;

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0) ;
	cairo_set_line_width (cr, BORDER_LINE_WIDTH) ;

	/* Border around actual spectrogram. */
	cairo_rectangle (cr, r->left, r->top, r->width, r->height) ;
	cairo_stroke (cr) ;

	cairo_select_font_face (cr, font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL) ;
	cairo_set_font_size (cr, 1.0 * NORMAL_FONT_SIZE) ;

	cairo_text_extents (cr, decibels, &extents) ;
	cairo_move_to (cr, r->left + (r->width - extents.width) / 2, r->top - 5) ;
	cairo_show_text (cr, decibels) ;

	tick_count = calculate_ticks (fabs (magfloor), r->height, &ticks) ;
	for (k = 0 ; k < tick_count ; k++)
	{	x_line (cr, r->left + r->width, r->top + ticks.distance [k], TICK_LEN) ;
		if (k % 2 == 1)
			continue ;

		str_print_value (text, sizeof (text), -1.0 * ticks.value [k]) ;
		cairo_text_extents (cr, text, &extents) ;
		cairo_move_to (cr, r->left + r->width + 2 * TICK_LEN, r->top + ticks.distance [k] + extents.height / 4.5) ;
		cairo_show_text (cr, text) ;
		} ;

	cairo_destroy (cr) ;
}
#endif

static void
interp_spec (float * mag, int maglen, const double *spec, int speclen)
{
	int k, lastspec = 0 ;

	mag [0] = spec [0] ;

	for (k = 1 ; k < maglen ; k++)
	{	double sum = 0.0 ;
		int count = 0 ;

		do
		{	sum += spec [lastspec] ;
			lastspec ++ ;
			count ++ ;
			}
		while (lastspec <= ceil ((k * speclen) / maglen)) ;

		mag [k] = sum / count ;
		} ;

	return ;
} /* interp_spec */

#ifdef NEVER
static void
render_to_surface (const RENDER * render, SNDFILE *infile, int samplerate, sf_count_t filelen, cairo_surface_t * surface)
{
	static double time_domain [10 * MAX_HEIGHT];
	static double freq_domain [10 * MAX_HEIGHT];
	static double single_mag_spec [5 * MAX_HEIGHT];
	static float mag_spec [MAX_WIDTH][MAX_HEIGHT];

	fftw_plan plan;
	double max_mag = 0.0;
	int width, height, w, speclen;

	if (render->border) {
		width = lrint (cairo_image_surface_get_width (surface) - LEFT_BORDER - RIGHT_BORDER) ;
		height = lrint (cairo_image_surface_get_height (surface) - TOP_BORDER - BOTTOM_BORDER) ;
	}
	else
	{	width = render->width;
		height = render->height;
	}

	/*
	**	Choose a speclen value that is long enough to represent frequencies down
	**	to 20Hz, and then increase it slightly so it is a multiple of 0x40 so that
	**	FFTW calculations will be quicker.
	*/
	speclen = height * (samplerate / 20 / height + 1) ;
	speclen += 0x40 - (speclen & 0x3f) ;

	if (2 * speclen > G_N_ELEMENTS (time_domain)) {
		printf ("%s : 2 * speclen > G_N_ELEMENTS (time_domain)\n", __func__);
		exit (1);
	};

	plan = fftw_plan_r2r_1d (2 * speclen, time_domain, freq_domain, FFTW_R2HC, FFTW_MEASURE | FFTW_PRESERVE_INPUT) ;
	if (plan == NULL) {
		printf ("%s : line %d : create plan failed.\n", __FILE__, __LINE__);
		exit (1) ;
	};

	for (w = 0 ; w < width ; w++) {
		double single_max;

		read_mono_audio (infile, filelen, time_domain, 2 * speclen, w, width) ;

		apply_window (time_domain, 2 * speclen) ;

		fftw_execute (plan) ;

		single_max = calc_magnitude (freq_domain, 2 * speclen, single_mag_spec) ;
		max_mag = MAX (max_mag, single_max) ;

		interp_spec (mag_spec [w], height, single_mag_spec, speclen) ;
	};

	fftw_destroy_plan (plan);

	if (render->border) {
		dbg(0, "with border...");
		RECT heat_rect;

		heat_rect.left = 12 ;
		heat_rect.top = TOP_BORDER + TOP_BORDER / 2 ;
		heat_rect.width = 12 ;
		heat_rect.height = height - TOP_BORDER / 2 ;

		_render_spectrogram (surface, render->spec_floor_db, mag_spec, max_mag, LEFT_BORDER, TOP_BORDER, width, height) ;

		render_heat_map (surface, render->spec_floor_db, &heat_rect) ;

		render_spect_border (surface, render->filename, LEFT_BORDER, width, filelen / (1.0 * samplerate), TOP_BORDER, height, 0.5 * samplerate) ;
		render_heat_border (surface, render->spec_floor_db, &heat_rect) ;
	}
	else
		_render_spectrogram (surface, render->spec_floor_db, mag_spec, max_mag, 0, 0, width, height);

	return;
}
#endif

static void
render_to_pixbuf (const RENDER * render, SNDFILE *infile, int samplerate, sf_count_t filelen, GdkPixbuf* pixbuf)
{
	static double time_domain [10 * MAX_HEIGHT];
	static double freq_domain [10 * MAX_HEIGHT];
	static double single_mag_spec [5 * MAX_HEIGHT];
	static float mag_spec [MAX_WIDTH][MAX_HEIGHT];

	fftw_plan plan;
	double max_mag = 0.0;
	int width, height, w, speclen;

	if (render->border) {
		width = lrint (gdk_pixbuf_get_width (pixbuf) - LEFT_BORDER - RIGHT_BORDER) ;
		height = lrint (gdk_pixbuf_get_height (pixbuf) - TOP_BORDER - BOTTOM_BORDER) ;
	}
	else
	{	width = render->width;
		height = render->height;
	}

	/*
	**	Choose a speclen value that is long enough to represent frequencies down
	**	to 20Hz, and then increase it slightly so it is a multiple of 0x40 so that
	**	FFTW calculations will be quicker.
	*/
	speclen = height * (samplerate / 20 / height + 1) ;
	speclen += 0x40 - (speclen & 0x3f) ;

	if (2 * speclen > G_N_ELEMENTS (time_domain)) {
		printf ("%s : 2 * speclen > G_N_ELEMENTS (time_domain)\n", __func__);
		exit (1);
	};

	plan = fftw_plan_r2r_1d (2 * speclen, time_domain, freq_domain, FFTW_R2HC, FFTW_MEASURE | FFTW_PRESERVE_INPUT) ;
	if (plan == NULL) {
		printf ("%s : line %d : create plan failed.\n", __FILE__, __LINE__);
		exit (1) ;
	};

	for (w = 0 ; w < width ; w++) {
		double single_max;

		read_mono_audio (infile, filelen, time_domain, 2 * speclen, w, width) ;

		apply_window (time_domain, 2 * speclen) ;

		fftw_execute (plan) ;

		single_max = calc_magnitude (freq_domain, 2 * speclen, single_mag_spec) ;
		max_mag = MAX (max_mag, single_max) ;

		interp_spec (mag_spec [w], height, single_mag_spec, speclen) ;
	};

	fftw_destroy_plan (plan);

	_render_spectrogram_to_pixbuf (pixbuf, render->spec_floor_db, mag_spec, max_mag, 0, 0, width, height);

	return;
}


#ifdef NEVER
static void
render_cairo_surface (const RENDER * render, SNDFILE *infile, int samplerate, sf_count_t filelen, cairo_surface_t* Xsurface, guchar** out)
{
	dbg(0, "...");

	/*
	**	CAIRO_FORMAT_RGB24 	 each pixel is a 32-bit quantity, with the upper 8 bits
	**	unused. Red, Green, and Blue are stored in the remaining 24 bits in that order.
	*/

	dbg(0, "creating 32 bit image surface...");
	cairo_surface_t* surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, render->width, render->height);
	if (surface == NULL || cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
		cairo_status_t status = cairo_surface_status (surface);
		printf ("Error while creating surface : %s\n", cairo_status_to_string (status));
		return;
	};

	cairo_surface_flush (surface);

	render_to_surface (render, infile, samplerate, filelen, surface);

	dbg(0, "done");
//#if 0
	cairo_status_t status;
	status = cairo_surface_write_to_png (surface, "output.png");
	if (status != CAIRO_STATUS_SUCCESS)
		printf ("Error while creating PNG file : %s\n", cairo_status_to_string (status));

//	cairo_surface_destroy (surface);
//#else
	//TODO we cannot destroy the surface until we have finished with this buffer.
	*out = cairo_image_surface_get_data(surface);
//#endif

	return;
}
#endif


static GdkPixbuf*
render_pixbuf (const RENDER * render, SNDFILE *infile, int samplerate, sf_count_t filelen)
{
	PF;

	/*
	**	CAIRO_FORMAT_RGB24 	 each pixel is a 32-bit quantity, with the upper 8 bits
	**	unused. Red, Green, and Blue are stored in the remaining 24 bits in that order.
	*/

	#define NO_ALPHA FALSE
	GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, NO_ALPHA, 8, render->width, render->height);

	render_to_pixbuf (render, infile, samplerate, filelen, pixbuf);

	return pixbuf;
}


static GdkPixbuf*
render_sndfile (const RENDER* render)
{
	SF_INFO info;

	if (render->log_freq) {
		printf ("Error : --log-freq option not working yet.\n\n");
		return NULL;
	};

	memset(&info, 0, sizeof (info));

	SNDFILE* infile = sf_open (render->sndfilepath, SFM_READ, &info);
	if (infile == NULL) {
		if(debug) printf ("Error : failed to open file '%s' : \n%s\n", render->sndfilepath, sf_strerror (NULL));
		return NULL;
	};

#if 0
	render_cairo_surface (render, infile, info.samplerate, info.frames, surface, out);
#else
	GdkPixbuf* pixbuf = render_pixbuf (render, infile, info.samplerate, info.frames);
#endif

	sf_close(infile);

	return pixbuf;
}


static void
check_int_range (const char * name, int value, int lower, int upper)
{
	if (value < lower || value > upper)
	{	printf ("Error : '%s' parameter must be in range [%d, %d]\n", name, lower, upper) ;
		exit (1) ;
		} ;
} /* check_int_range */

#if 0
static void
usage_exit (const char * argv0, int error)
{
	const char * progname ;

	progname = strrchr (argv0, '/') ;
	progname = (progname == NULL) ? argv0 : progname + 1 ;

	printf ("\nUsage :\n\n    %s [options] <sound file> <img width> <img height> <png name>\n\n", progname) ;

	puts (
		"    Create a spectrogram as a PNG file from a given sound file. The\n"
		"    spectrogram image will be of the given width and height.\n"
		) ;

	puts (
		"    Options:\n"
		"        --dyn-range=<number>   : Dynamic range (ie 100 for 100dB range)\n"
		"        --no-border            : Drop the border, scales, heat map and title\n" 
		/*-"        --log-freq             : Use a logarithmic frquency scale\n" -*/
		) ;

	exit (error) ;
}
#endif


RENDER*
render_new()
{
	RENDER* r = g_new0(RENDER, 1);
	r->spec_floor_db = SPEC_FLOOR_DB;
	return r;
}


void
render_free(RENDER* render)
{
	if(render->sndfilepath) g_free((char*)render->sndfilepath);
	g_free(render);
}


void
#ifdef USE_OPENGL
render_spectrogram(const char* path, GLSpectrogram* w, SpectrogramReady callback, gpointer user_data)
#else
render_spectrogram(const char* path, SpectrogramWidget* w, SpectrogramReady callback, gpointer user_data)
#endif
{
	g_return_if_fail(path);
	g_return_if_fail(callback);

	#define no_border false

	RENDER* render = render_new();
	render->sndfilepath = g_strdup(path);
	render->w = w;
	render->callback = callback;
	render->user_data = user_data;

	//other rendering options:
	//render->spec_floor_db = -1.0 * fabs (fval); //dynamic range
	//render->log_freq = true;                    //log freq

	static GAsyncQueue* msg_queue = NULL;

	gpointer
	fft_thread(gpointer data)
	{
		PF;
		g_async_queue_ref(msg_queue);

		GList* msg_list = NULL;
		while(1){
			//any new work ?
			while(g_async_queue_length(msg_queue)){
				gpointer message = g_async_queue_pop(msg_queue);
				dbg(1, "new message");
				msg_list = g_list_append(msg_list, message);
			}

			while(msg_list){
				gboolean do_callback(gpointer _render)
				{
					RENDER* render = _render;
					if(debug && !render->pixbuf) gwarn("no pixbuf.");

					render->callback(render->sndfilepath, render->pixbuf, render->user_data);

					//temporary!
#ifdef USE_OPENGL
					gl_spectrogram_image_ready(render->w, render->pixbuf);
#else
					spectrogram_widget_image_ready(render->w, render->pixbuf);
#endif

					render_free(render);
					return TIMER_STOP;
				}

				RENDER* render = g_list_first(msg_list)->data;
				dbg(1, "filename=%s.", render->filename);
				render->pixbuf = render_sndfile(render);
				g_idle_add(do_callback, render);
				msg_list = g_list_remove(msg_list, render);
			}

			sleep(1); //FIXME this is a bit primitive - maybe the thread should have its own GMainLoop
						 //-even better is to use a blocking call on the async queue, waiting for messages.
		}
	}

	if(!msg_queue){
		dbg(2, "creating fft thread...");
		GError* error = NULL;
		msg_queue = g_async_queue_new();
		if(!g_thread_create(fft_thread, NULL, false, &error)){
			perr("error creating thread: %s\n", error->message);
			g_error_free(error);
		}
	}

	render->width = 640;
	render->height = 640;

	check_int_range ("width", render->width, MIN_WIDTH, MAX_WIDTH);
	check_int_range ("height", render->height, MIN_HEIGHT, MAX_HEIGHT);

	render->filename = strrchr (render->sndfilepath, '/'); 
	render->filename = (render->filename != NULL) ? render->filename + 1 : render->sndfilepath;

	dbg(1, "%s", render->filename);
	g_async_queue_push(msg_queue, render);
}


void
#ifdef USE_OPENGL
get_spectrogram(const char* path, GLSpectrogram* w, SpectrogramReady callback)
#else
get_spectrogram(const char* path, SpectrogramWidget* w, SpectrogramReady callback)
#endif
{
	gchar* cache_dir = g_build_filename(app.cache_dir, "spectrogram", NULL);
	g_mkdir_with_parents(cache_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP);

	gchar* make_cache_name(const char* path)
	{
		gchar* name = str_replace(path, "/", "___");
		gchar* name2 = g_strdup_printf("%s.png", name);
		gchar* cache_path = g_build_filename(app.cache_dir, "spectrogram", name2, NULL);
		g_free(name);
		g_free(name2);
		return cache_path;
	}

	gchar* cache_path = make_cache_name(path);
	if(g_file_test(cache_path, G_FILE_TEST_IS_REGULAR)){
		dbg(1, "cache: hit");
		GError** error = NULL;
		GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(cache_path, error);
		//TODO unref?
		if(pixbuf){
			dbg(1, "cache: pixbuf loaded");
			callback(pixbuf, path, w);
		}
	}else{
		dbg(1, "cache: not found: %s", cache_path);
		typedef struct __closure
		{
			SpectrogramReady callback;

		} _closure;
		_closure* closure = g_new0(_closure, 1);
		closure->callback = callback;

		void spectrogram_ready(const char* filename, GdkPixbuf* pixbuf, gpointer user_data)
		{
			void maintain_cache(const char* cache_dir)
			{
				void delete_oldest_file(const char* cache_dir)
				{
					GError** error = NULL;
					GDir* dir = g_dir_open(cache_dir, 0, error);
					struct stat s;
					const char* f;
					struct _o {time_t time; char* name;} oldest = {0, NULL};
					while((f = g_dir_read_name(dir))){
						char* p = g_build_filename(cache_dir, f, NULL);
						if(g_stat(p, &s) == -1){
							dbg(0, "stat error! %s", f);
						}else{
							dbg(2, "time=%lu %s", s.st_mtime, f);
							if(!oldest.time || s.st_mtime < oldest.time){
								oldest.time = s.st_mtime;
								oldest.name = g_strdup(p);
							}
						}
						g_free(p);
					}
					g_dir_close(dir);

					if(oldest.name){
						dbg(2, "deleting: %s", oldest.name);
						g_unlink(oldest.name);
						g_free(oldest.name);
					}
				}

				dbg(2, "...");
				GError** error = NULL;
				GDir* dir = g_dir_open(cache_dir, 0, error);
				#define MAX_CACHE_ITEMS 20
				int n = 0;
				while(g_dir_read_name(dir)) n++;
				g_dir_close(dir);

				if(n >= MAX_CACHE_ITEMS){
					dbg(2, "cache full. removing oldest cache item...");
					delete_oldest_file(cache_dir);
				}
				else dbg(2, "cache_size=%i", n);
			}

			if(pixbuf){
				gchar* cache_dir = g_build_filename(app.cache_dir, "spectrogram", NULL);

				maintain_cache(cache_dir);

				gchar* filepath = make_cache_name(filename);
				dbg(2, "saving: %s", filepath);
				GError* error = NULL;
				if(!gdk_pixbuf_save(pixbuf, filepath, "png", &error, NULL)){
					P_GERR;
				}
				g_free(filepath);

				g_free(cache_dir);
			}

			_closure* closure = user_data;

			if(false && closure && closure->callback) closure->callback(filename, pixbuf, NULL);

			g_free(closure);
		}

		render_spectrogram(path, w, spectrogram_ready, closure);
	}
	g_free(cache_dir);
	g_free(cache_path);
}

