/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __inspector_h__
#define __inspector_h__
#include <gtk/gtk.h>
#include "typedefs.h"

GtkWidget* inspector_new                  ();
void       inspector_free                 (Inspector*);

struct _inspector
{
	GtkWidget*     widget;     //scrollwin
	gboolean       show_waveform;
	int            preferred_height;
#ifndef USE_GDL
	int            user_height;
#endif
	InspectorPriv* priv;
};

#endif
