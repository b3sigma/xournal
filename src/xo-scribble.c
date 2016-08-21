#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "libscribble/scribble.h"

typedef struct{
 guint16 red;
 guint16 green;
 guint16 blue;
 guint16 alpha;
} color_struct;


color_struct get_pixel_pixbuf(double x, double y, GdkPixbuf *pixbuf, unsigned char *pixels){
  color_struct color;
  guchar *p;
  p = pixels + ((int)y) * gdk_pixbuf_get_rowstride (pixbuf) + ((int)x) * gdk_pixbuf_get_n_channels(pixbuf);

  color.red = p[0];
  color.green = p[1];
  color.blue = p[2];
  color.alpha = p[3];

  return color;
}


void print_pixel_at(GnomeCanvas* canvas, double x, double y) {
	GdkPixbuf *surface_pixbuf;
	GtkWidget *drawingarea;
	guchar *pixbuf_pixels;
	color_struct pixel_color;

	drawingarea = (GtkWidget*)canvas;
	surface_pixbuf = gdk_pixbuf_get_from_drawable(NULL,
			GDK_DRAWABLE(drawingarea->window), gdk_colormap_get_system(), 0,0,0,0,
			drawingarea->allocation.width, drawingarea->allocation.height);
	if(!surface_pixbuf) {
		printf("print_pixel_at: no surface_pixbuf\n");
		return;
	}
	pixbuf_pixels = gdk_pixbuf_get_pixels(surface_pixbuf);
	if(!pixbuf_pixels) {
		printf("print_pixel_at: no pixbuf_pixels\n");
		return;
	}

	pixel_color = get_pixel_pixbuf(x, y, surface_pixbuf, pixbuf_pixels);
	// if(!pixel_color) {
	// 	printf("print_pixel_at: no pixel_color\n");
	// 	return;		
	// }
	// printf("Got a color, r:%d g:%d b:%d a:%d\n",
	// 	pixel_color->red, pixel_color->blue, pixel_color->green, pixel_color->alpha); 

	printf("Got a color, r:%d g:%d b:%d a:%d\n",
			pixel_color.red, pixel_color.green, pixel_color.blue, pixel_color.alpha); 
}

void update_scribble(
    GnomeCanvas* canvas, double xEndStroke, double yEndStroke, double xStartStroke, double yStartStroke) {

	GdkPixbuf *surface_pixbuf;
	GtkWidget *drawingarea;
	guchar *pixbuf_pixels;
	color_struct pixel_color;
	double box_size = 32.0; // just supporting 32x32 pixel input recognition so far
	double x_box_min = xEndStroke - box_size;
	double x_box_max = xEndStroke;
	double y_box_min = yEndStroke - box_size;
	double y_box_max = yEndStroke;
  guchar *start_pixel;
	void* scribble_handle = NULL;
	int guessed_number = -1;

	if(x_box_min < 0.0) {
		x_box_max += (-x_box_min);
		x_box_min = 0.0;
	} // uh so what's the max?
	if(y_box_min < 0.0) {
		y_box_max += (-y_box_min);
		y_box_min = 0.0;
	} // uh so what's the max also here?
	
	x_box_min = 0;
	x_box_max = 32;
	y_box_min = 0;
	y_box_max = 32;

	drawingarea = (GtkWidget*)canvas;
	surface_pixbuf = gdk_pixbuf_get_from_drawable(NULL,
			GDK_DRAWABLE(drawingarea->window), gdk_colormap_get_system(), 0,0,0,0,
			drawingarea->allocation.width, drawingarea->allocation.height);
	if(!surface_pixbuf) {
		printf("print_pixel_at: no surface_pixbuf\n");
		return;
	}
	pixbuf_pixels = gdk_pixbuf_get_pixels(surface_pixbuf);
	if(!pixbuf_pixels) {
		printf("print_pixel_at: no pixbuf_pixels\n");
		return;
	}

 int pitch_bytes = gdk_pixbuf_get_n_channels(surface_pixbuf);
 int y_stride_bytes = gdk_pixbuf_get_rowstride(surface_pixbuf);

  start_pixel = pixbuf_pixels 
									+ ((int)y_box_min) * y_stride_bytes
									+ ((int)x_box_min) * pitch_bytes;

	if(!initialize(0 /* use default weights */, &scribble_handle)) {
		printf("failed to initialize scribble\n");
		return;
	}

	// unsigned char* test_data = (unsigned char*)


	if(memory_recognize(scribble_handle, start_pixel,
			32, 32,
			//(int)(x_box_max - x_box_min), (int)(y_box_max - y_box_min), 
			pitch_bytes, y_stride_bytes,
      &guessed_number)) {
		printf("Was the number a %d? \t(%f,%f to %f,%f)\n", guessed_number, x_box_min, x_box_max, y_box_min, y_box_max);
	} else {
		printf("Didn't see nothin\n");
	}

	cleanup(scribble_handle);
}
