/* spectrogram_widget.c generated by valac 0.12.1, the Vala compiler
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

typedef void (*RenderDoneFunc) (gchar* filename, GdkPixbuf* a, void* user_data_, void* user_data);
struct _SpectrogramWidget {
	GtkWidget parent_instance;
	SpectrogramWidgetPrivate * priv;
};

struct _SpectrogramWidgetClass {
	GtkWidgetClass parent_class;
};

struct _SpectrogramWidgetPrivate {
	gchar* _filename;
	GdkPixbuf* pixbuf;
};


static gpointer spectrogram_widget_parent_class = NULL;

void get_spectrogram_with_target (gchar* path, RenderDoneFunc on_ready, void* on_ready_target, void* user_data);
void cancel_spectrogram (gchar* path);
GType spectrogram_widget_get_type (void) G_GNUC_CONST;
#define SPECTROGRAM_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_SPECTROGRAM_WIDGET, SpectrogramWidgetPrivate))
enum  {
	SPECTROGRAM_WIDGET_DUMMY_PROPERTY
};
void spectrogram_widget_image_ready (SpectrogramWidget* self, gchar* filename, GdkPixbuf* _pixbuf, void* user_data);
void spectrogram_widget_set_file (SpectrogramWidget* self, gchar* filename);
static void _lambda0_ (gchar* filename, GdkPixbuf* a, void* b, SpectrogramWidget* self);
static void __lambda0__render_done_func (gchar* filename, GdkPixbuf* a, void* user_data_, gpointer self);
static void _spectrogram_widget_image_ready_render_done_func (gchar* filename, GdkPixbuf* a, void* user_data_, gpointer self);
static void spectrogram_widget_real_realize (GtkWidget* base);
static void spectrogram_widget_real_unrealize (GtkWidget* base);
static void spectrogram_widget_real_size_request (GtkWidget* base, GtkRequisition* requisition);
static void spectrogram_widget_real_size_allocate (GtkWidget* base, GdkRectangle* allocation);
static gboolean spectrogram_widget_real_expose_event (GtkWidget* base, GdkEventExpose* event);
SpectrogramWidget* spectrogram_widget_new (void);
SpectrogramWidget* spectrogram_widget_construct (GType object_type);
static GObject * spectrogram_widget_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void spectrogram_widget_finalize (GObject* obj);


void spectrogram_widget_image_ready (SpectrogramWidget* self, gchar* filename, GdkPixbuf* _pixbuf, void* user_data) {
	g_return_if_fail (self != NULL);
	if ((gboolean) self->priv->pixbuf) {
		g_object_unref ((GObject*) self->priv->pixbuf);
	}
	self->priv->pixbuf = _pixbuf;
	gtk_widget_queue_draw ((GtkWidget*) self);
}


static void _lambda0_ (gchar* filename, GdkPixbuf* a, void* b, SpectrogramWidget* self) {
	self->priv->pixbuf = a;
	g_print ("%s: got callback!\n", "SpectrogramWidget.set_file._lambda0_");
	gtk_widget_queue_draw ((GtkWidget*) self);
}


static void __lambda0__render_done_func (gchar* filename, GdkPixbuf* a, void* user_data_, gpointer self) {
	_lambda0_ (filename, a, user_data_, self);
}


static void _spectrogram_widget_image_ready_render_done_func (gchar* filename, GdkPixbuf* a, void* user_data_, gpointer self) {
	spectrogram_widget_image_ready (self, filename, a, user_data_);
}


void spectrogram_widget_set_file (SpectrogramWidget* self, gchar* filename) {
	gchar* _tmp0_ = NULL;
	RenderDoneFunc p1;
	void* p1_target = NULL;
	GDestroyNotify p1_target_destroy_notify = NULL;
	g_return_if_fail (self != NULL);
	_tmp0_ = g_strdup ((const gchar*) filename);
	_g_free0 (self->priv->_filename);
	self->priv->_filename = _tmp0_;
	if ((gboolean) self->priv->pixbuf) {
		gdk_pixbuf_fill (self->priv->pixbuf, (guint32) 0x0);
	}
	cancel_spectrogram (NULL);
	p1 = __lambda0__render_done_func;
	p1_target = g_object_ref (self);
	p1_target_destroy_notify = g_object_unref;
	get_spectrogram_with_target (filename, _spectrogram_widget_image_ready_render_done_func, self, NULL);
	(p1_target_destroy_notify == NULL) ? NULL : (p1_target_destroy_notify (p1_target), NULL);
	p1 = NULL;
	p1_target = NULL;
	p1_target_destroy_notify = NULL;
}


static void spectrogram_widget_real_realize (GtkWidget* base) {
	SpectrogramWidget * self;
	GdkWindowAttr attrs = {0};
	gint _tmp0_;
	GdkWindow* _tmp1_ = NULL;
	GdkWindow* _tmp2_ = NULL;
	GtkStyle* _tmp3_ = NULL;
	GtkStyle* _tmp4_ = NULL;
	GtkStyle* _tmp5_ = NULL;
	self = (SpectrogramWidget*) base;
	GTK_WIDGET_SET_FLAGS ((GtkWidget*) self, GTK_REALIZED);
	memset (&attrs, 0, sizeof (GdkWindowAttr));
	attrs.window_type = GDK_WINDOW_CHILD;
	attrs.width = ((GtkWidget*) self)->allocation.width;
	attrs.wclass = GDK_INPUT_OUTPUT;
	_tmp0_ = gtk_widget_get_events ((GtkWidget*) self);
	attrs.event_mask = _tmp0_ | GDK_EXPOSURE_MASK;
	_tmp1_ = gtk_widget_get_parent_window ((GtkWidget*) self);
	_tmp2_ = gdk_window_new (_tmp1_, &attrs, 0);
	_g_object_unref0 (((GtkWidget*) self)->window);
	((GtkWidget*) self)->window = _tmp2_;
	gdk_window_set_user_data (((GtkWidget*) self)->window, self);
	_tmp3_ = gtk_widget_get_style ((GtkWidget*) self);
	_tmp4_ = gtk_style_attach (_tmp3_, ((GtkWidget*) self)->window);
	gtk_widget_set_style ((GtkWidget*) self, _tmp4_);
	_tmp5_ = gtk_widget_get_style ((GtkWidget*) self);
	gtk_style_set_background (_tmp5_, ((GtkWidget*) self)->window, GTK_STATE_NORMAL);
	gdk_window_move_resize (((GtkWidget*) self)->window, ((GtkWidget*) self)->allocation.x, ((GtkWidget*) self)->allocation.y, ((GtkWidget*) self)->allocation.width, ((GtkWidget*) self)->allocation.height);
}


static void spectrogram_widget_real_unrealize (GtkWidget* base) {
	SpectrogramWidget * self;
	self = (SpectrogramWidget*) base;
	gdk_window_set_user_data (((GtkWidget*) self)->window, NULL);
}


static void spectrogram_widget_real_size_request (GtkWidget* base, GtkRequisition* requisition) {
	SpectrogramWidget * self;
	GtkRequisition _requisition = {0};
	GtkWidgetFlags _tmp0_;
	GtkWidgetFlags flags;
	self = (SpectrogramWidget*) base;
	_requisition.width = 10;
	_requisition.height = 80;
	_tmp0_ = GTK_WIDGET_FLAGS ((GtkWidget*) self);
	flags = _tmp0_;
	if ((flags & GTK_REALIZED) != 0) {
	}
	if (requisition) {
		*requisition = _requisition;
	}
}


static void spectrogram_widget_real_size_allocate (GtkWidget* base, GdkRectangle* allocation) {
	SpectrogramWidget * self;
	GtkWidgetFlags _tmp0_;
	self = (SpectrogramWidget*) base;
	((GtkWidget*) self)->allocation = (GtkAllocation) (*allocation);
	_tmp0_ = GTK_WIDGET_FLAGS ((GtkWidget*) self);
	if ((_tmp0_ & GTK_REALIZED) == 0) {
		return;
	}
	gdk_window_move_resize (((GtkWidget*) self)->window, ((GtkWidget*) self)->allocation.x, ((GtkWidget*) self)->allocation.y, ((GtkWidget*) self)->allocation.width, ((GtkWidget*) self)->allocation.height);
}


static gboolean spectrogram_widget_real_expose_event (GtkWidget* base, GdkEventExpose* event) {
	SpectrogramWidget * self;
	gboolean result = FALSE;
	gint width;
	gint height;
	GdkPixbuf* _tmp0_ = NULL;
	GdkPixbuf* scaled;
	guint8* _tmp1_ = NULL;
	guchar* image;
	GtkStyle* _tmp2_ = NULL;
	gint _tmp3_;
	self = (SpectrogramWidget*) base;
	if (self->priv->pixbuf == NULL) {
		result = TRUE;
		return result;
	}
	width = ((GtkWidget*) self)->allocation.width;
	height = ((GtkWidget*) self)->allocation.height;
	_tmp0_ = gdk_pixbuf_scale_simple (self->priv->pixbuf, ((GtkWidget*) self)->allocation.width, ((GtkWidget*) self)->allocation.height, GDK_INTERP_BILINEAR);
	scaled = _tmp0_;
	_tmp1_ = gdk_pixbuf_get_pixels (scaled);
	image = _tmp1_;
	_tmp2_ = gtk_widget_get_style ((GtkWidget*) self);
	_tmp3_ = gdk_pixbuf_get_rowstride (scaled);
	gdk_draw_rgb_image ((GdkDrawable*) ((GtkWidget*) self)->window, _tmp2_->fg_gc[GTK_STATE_NORMAL], 0, 0, width, height, GDK_RGB_DITHER_MAX, (guchar*) image, _tmp3_);
	result = TRUE;
	return result;
}


SpectrogramWidget* spectrogram_widget_construct (GType object_type) {
	SpectrogramWidget * self = NULL;
	self = (SpectrogramWidget*) gtk_widget_new (object_type, NULL);
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



