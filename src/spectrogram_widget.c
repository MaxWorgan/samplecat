/* spectrogram_widget.c generated by valac, the Vala compiler
 * generated from spectrogram_widget.vala, do not modify */

/**/
/* Johan Dahlin 2008*/
/**/
/* A quite simple Gtk.Widget subclass which demonstrates how to subclass*/
/* and do realizing, sizing and drawing.*/
/* */
/* from http://live.gnome.org/Vala/GTKSample?highlight=%28widget%29%7C%28vala%29*/

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gdk/gdk.h>


#define TYPE_SPECTROGRAM_WIDGET (spectrogram_widget_get_type ())
#define SPECTROGRAM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidget))
#define SPECTROGRAM_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidgetClass))
#define IS_SPECTROGRAM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SPECTROGRAM_WIDGET))
#define IS_SPECTROGRAM_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SPECTROGRAM_WIDGET))
#define SPECTROGRAM_WIDGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidgetClass))

typedef struct _SpectrogramWidget SpectrogramWidget;
typedef struct _SpectrogramWidgetClass SpectrogramWidgetClass;
typedef struct _SpectrogramWidgetPrivate SpectrogramWidgetPrivate;
#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

typedef void (*PrintIntFunc) (GdkPixbuf* a, void* user_data_, void* user_data);
struct _SpectrogramWidget {
	GtkWidget parent_instance;
	SpectrogramWidgetPrivate * priv;
};

struct _SpectrogramWidgetClass {
	GtkWidgetClass parent_class;
};

struct _SpectrogramWidgetPrivate {
	char* _filename;
	GdkPixbuf* pixbuf;
};


static gpointer spectrogram_widget_parent_class = NULL;

GType spectrogram_widget_get_type (void);
void render_spectrogram (gchar* path, SpectrogramWidget* w, PrintIntFunc callback, void* callback_target, void* user_data);
void get_spectrogram (gchar* path, SpectrogramWidget* w, PrintIntFunc callback, void* callback_target);
#define SPECTROGRAM_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidgetPrivate))
enum  {
	SPECTROGRAM_WIDGET_DUMMY_PROPERTY
};
void spectrogram_widget_image_ready (SpectrogramWidget* self, GdkPixbuf* _pixbuf);
static void _lambda0_ (GdkPixbuf* a, void* b, SpectrogramWidget* self);
static void __lambda0__print_int_func (GdkPixbuf* a, void* user_data_, gpointer self);
static void _spectrogram_widget_image_ready_print_int_func (GdkPixbuf* a, void* user_data_, gpointer self);
void spectrogram_widget_set_file (SpectrogramWidget* self, gchar* filename);
static void spectrogram_widget_real_realize (GtkWidget* base);
static void spectrogram_widget_real_unrealize (GtkWidget* base);
static void spectrogram_widget_real_size_request (GtkWidget* base, GtkRequisition* requisition);
static void spectrogram_widget_real_size_allocate (GtkWidget* base, GdkRectangle* allocation);
static gboolean spectrogram_widget_real_expose_event (GtkWidget* base, GdkEventExpose* event);
SpectrogramWidget* spectrogram_widget_new (void);
SpectrogramWidget* spectrogram_widget_construct (GType object_type);
static GObject * spectrogram_widget_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void spectrogram_widget_finalize (GObject* obj);



void spectrogram_widget_image_ready (SpectrogramWidget* self, GdkPixbuf* _pixbuf) {
	g_return_if_fail (self != NULL);
	if (self->priv->pixbuf != NULL) {
		g_object_unref ((GObject*) self->priv->pixbuf);
	}
	self->priv->pixbuf = _pixbuf;
	gtk_widget_queue_draw ((GtkWidget*) self);
}


static void _lambda0_ (GdkPixbuf* a, void* b, SpectrogramWidget* self) {
	self->priv->pixbuf = a;
	fprintf (stdout, "set_file: got callback!\n");
	gtk_widget_queue_draw ((GtkWidget*) self);
}


static void __lambda0__print_int_func (GdkPixbuf* a, void* user_data_, gpointer self) {
	_lambda0_ (a, user_data_, self);
}


static void _spectrogram_widget_image_ready_print_int_func (GdkPixbuf* a, void* user_data_, gpointer self) {
	spectrogram_widget_image_ready (self, a);
}


void spectrogram_widget_set_file (SpectrogramWidget* self, gchar* filename) {
	char* _tmp0_;
	PrintIntFunc _tmp1_;
	GDestroyNotify p1_target_destroy_notify = NULL;
	void* p1_target = NULL;
	PrintIntFunc p1;
	g_return_if_fail (self != NULL);
	self->priv->_filename = (_tmp0_ = g_strdup ((const char*) filename), _g_free0 (self->priv->_filename), _tmp0_);
	p1 = (_tmp1_ = __lambda0__print_int_func, p1_target = g_object_ref (self), p1_target_destroy_notify = g_object_unref, _tmp1_);
	get_spectrogram (filename, self, _spectrogram_widget_image_ready_print_int_func, self);
	(p1_target_destroy_notify == NULL) ? NULL : p1_target_destroy_notify (p1_target);
	p1 = NULL;
	p1_target = NULL;
	p1_target_destroy_notify = NULL;
}


static void spectrogram_widget_real_realize (GtkWidget* base) {
	SpectrogramWidget * self;
	GdkWindowAttr attrs = {0};
	GdkWindow* _tmp0_;
	self = (SpectrogramWidget*) base;
	GTK_WIDGET_SET_FLAGS ((GtkWidget*) self, GTK_REALIZED);
	memset (&attrs, 0, sizeof (GdkWindowAttr));
	attrs.window_type = GDK_WINDOW_CHILD;
	attrs.width = ((GtkWidget*) self)->allocation.width;
	attrs.wclass = GDK_INPUT_OUTPUT;
	attrs.event_mask = gtk_widget_get_events ((GtkWidget*) self) | GDK_EXPOSURE_MASK;
	((GtkWidget*) self)->window = (_tmp0_ = gdk_window_new (gtk_widget_get_parent_window ((GtkWidget*) self), &attrs, 0), _g_object_unref0 (((GtkWidget*) self)->window), _tmp0_);
	gdk_window_set_user_data (((GtkWidget*) self)->window, self);
	gtk_widget_set_style ((GtkWidget*) self, gtk_style_attach (gtk_widget_get_style ((GtkWidget*) self), ((GtkWidget*) self)->window));
	gtk_style_set_background (gtk_widget_get_style ((GtkWidget*) self), ((GtkWidget*) self)->window, GTK_STATE_NORMAL);
	gdk_window_move_resize (((GtkWidget*) self)->window, ((GtkWidget*) self)->allocation.x, ((GtkWidget*) self)->allocation.y, ((GtkWidget*) self)->allocation.width, ((GtkWidget*) self)->allocation.height);
}


static void spectrogram_widget_real_unrealize (GtkWidget* base) {
	SpectrogramWidget * self;
	self = (SpectrogramWidget*) base;
	gdk_window_set_user_data (((GtkWidget*) self)->window, NULL);
}


static void spectrogram_widget_real_size_request (GtkWidget* base, GtkRequisition* requisition) {
	SpectrogramWidget * self;
	GtkWidgetFlags flags;
	self = (SpectrogramWidget*) base;
	(*requisition).width = 10;
	(*requisition).height = 80;
	flags = GTK_WIDGET_FLAGS ((GtkWidget*) self);
	if ((flags & GTK_REALIZED) != 0) {
	}
}


static void spectrogram_widget_real_size_allocate (GtkWidget* base, GdkRectangle* allocation) {
	SpectrogramWidget * self;
	self = (SpectrogramWidget*) base;
	((GtkWidget*) self)->allocation = (GtkAllocation) (*allocation);
	if ((GTK_WIDGET_FLAGS ((GtkWidget*) self) & GTK_REALIZED) == 0) {
		return;
	}
	gdk_window_move_resize (((GtkWidget*) self)->window, ((GtkWidget*) self)->allocation.x, ((GtkWidget*) self)->allocation.y, ((GtkWidget*) self)->allocation.width, ((GtkWidget*) self)->allocation.height);
}


static gboolean spectrogram_widget_real_expose_event (GtkWidget* base, GdkEventExpose* event) {
	SpectrogramWidget * self;
	gboolean result = FALSE;
	gint width;
	gint height;
	GdkPixbuf* scaled;
	guchar* image;
	self = (SpectrogramWidget*) base;
	if (self->priv->pixbuf == NULL) {
		result = TRUE;
		return result;
	}
	width = ((GtkWidget*) self)->allocation.width;
	height = ((GtkWidget*) self)->allocation.height;
	scaled = gdk_pixbuf_scale_simple (self->priv->pixbuf, ((GtkWidget*) self)->allocation.width, ((GtkWidget*) self)->allocation.height, GDK_INTERP_BILINEAR);
	image = gdk_pixbuf_get_pixels (scaled);
	gdk_draw_rgb_image ((GdkDrawable*) ((GtkWidget*) self)->window, gtk_widget_get_style ((GtkWidget*) self)->fg_gc[GTK_STATE_NORMAL], 0, 0, width, height, GDK_RGB_DITHER_MAX, (guchar*) image, gdk_pixbuf_get_rowstride (scaled));
	result = TRUE;
	return result;
}


SpectrogramWidget* spectrogram_widget_construct (GType object_type) {
	SpectrogramWidget * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


SpectrogramWidget* spectrogram_widget_new (void) {
	return spectrogram_widget_construct (TYPE_SPECTROGRAM_WIDGET);
}


static GObject * spectrogram_widget_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	SpectrogramWidget * self;
	parent_class = G_OBJECT_CLASS (spectrogram_widget_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = SPECTROGRAM_WIDGET (obj);
	{
	}
	return obj;
}


static void spectrogram_widget_class_init (SpectrogramWidgetClass * klass) {
	spectrogram_widget_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (SpectrogramWidgetPrivate));
	GTK_WIDGET_CLASS (klass)->realize = spectrogram_widget_real_realize;
	GTK_WIDGET_CLASS (klass)->unrealize = spectrogram_widget_real_unrealize;
	GTK_WIDGET_CLASS (klass)->size_request = spectrogram_widget_real_size_request;
	GTK_WIDGET_CLASS (klass)->size_allocate = spectrogram_widget_real_size_allocate;
	GTK_WIDGET_CLASS (klass)->expose_event = spectrogram_widget_real_expose_event;
	G_OBJECT_CLASS (klass)->constructor = spectrogram_widget_constructor;
	G_OBJECT_CLASS (klass)->finalize = spectrogram_widget_finalize;
}


static void spectrogram_widget_instance_init (SpectrogramWidget * self) {
	self->priv = SPECTROGRAM_WIDGET_GET_PRIVATE (self);
	self->priv->pixbuf = NULL;
}


static void spectrogram_widget_finalize (GObject* obj) {
	SpectrogramWidget * self;
	self = SPECTROGRAM_WIDGET (obj);
	_g_free0 (self->priv->_filename);
	G_OBJECT_CLASS (spectrogram_widget_parent_class)->finalize (obj);
}


GType spectrogram_widget_get_type (void) {
	static volatile gsize spectrogram_widget_type_id__volatile = 0;
	if (g_once_init_enter (&spectrogram_widget_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (SpectrogramWidgetClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) spectrogram_widget_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SpectrogramWidget), 0, (GInstanceInitFunc) spectrogram_widget_instance_init, NULL };
		GType spectrogram_widget_type_id;
		spectrogram_widget_type_id = g_type_register_static (GTK_TYPE_WIDGET, "SpectrogramWidget", &g_define_type_info, 0);
		g_once_init_leave (&spectrogram_widget_type_id__volatile, spectrogram_widget_type_id);
	}
	return spectrogram_widget_type_id__volatile;
}




