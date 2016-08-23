#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "xournal.h"
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

typedef struct Vector2 {
  double x;
  double y;
} Vector2;

typedef struct BoundingBox {
  Vector2 min;
  Vector2 max;
} BoundingBox;

BoundingBox calculate_bounding_box_from_path(
    GnomeCanvasPoints* cur_path) {
  BoundingBox box;
  box.min.x = DBL_MAX;
  box.min.y = DBL_MAX;
  box.max.x = DBL_MIN;
  box.max.y = DBL_MIN;

	// there are actually 2*num_points coords, x and y interleaved
	for(int i = 0; i < cur_path->num_points; i++) {
		double point_x = cur_path->coords[i * 2];
		double point_y = cur_path->coords[i * 2 + 1];
		if(box.min.x > point_x) {
			box.min.x = point_x;
		}
		if(box.min.y > point_y) {
			box.min.y = point_y;
		}
		if(box.max.x < point_x) {
			box.max.x = point_x;
		}
		if(box.max.y < point_y) {
			box.max.y = point_y;
		}
	}
	
  return box;
}

void update_scribble(
    GnomeCanvas* canvas, double xEndStroke, double yEndStroke,
		double xStartStroke, double yStartStroke) {

	GdkPixbuf *surface_pixbuf;
	GtkWidget *drawingarea;
	guchar *pixbuf_pixels;
	color_struct pixel_color;
	double box_size = 320.0; // just supporting 32x32 pixel input recognition so far
	double x_box_min = xEndStroke - box_size;
	double x_box_max = xEndStroke;
	double y_box_min = yEndStroke - box_size;
	double y_box_max = yEndStroke;
  guchar *start_pixel;
	void* scribble_handle = NULL;
	int guessed_number = -1;

	BoundingBox path_bb = calculate_bounding_box_from_path(&ui.cur_path);

	if(x_box_min < 0.0) {
		x_box_max += (-x_box_min);
		x_box_min = 0.0;
	} // uh so what's the max?
	if(y_box_min < 0.0) {
		y_box_max += (-y_box_min);
		y_box_min = 0.0;
	} // uh so what's the max also here?

	double cx, cy;
	int wx, wy, sx, sy;
  gnome_canvas_get_scroll_offsets(canvas, &sx, &sy);
  gdk_window_get_geometry(GTK_WIDGET(canvas)->window, NULL, NULL, &wx, &wy, NULL);
  gnome_canvas_window_to_world(canvas, sx + wx/2, sy + wy/2, &cx, &cy);
	int x_pix_min = (int)(cx); //(x_box_min * 4.0);
	int y_pix_min = (int)(cy); //(y_box_min * 4.0);
	printf("sx:%d sy:%d wx:%d wy:%d cx:%f cy:%f x_pix:%d y_pix:%d\n", 
			sx, sy, wx, wy, cx, cy, x_pix_min, y_pix_min);

	drawingarea = (GtkWidget*)canvas;
	surface_pixbuf = gdk_pixbuf_get_from_drawable(NULL,
			GDK_DRAWABLE(drawingarea->window), gdk_colormap_get_system(),
			// x_box_min, y_box_min, 0, 0,s
			// (int)box_size, (int)box_size);
			0,0,0,0,
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
									+ ((int)y_pix_min) * y_stride_bytes
									+ ((int)x_pix_min) * pitch_bytes;
	// start_pixel = pixbuf_pixels;

	if(!initialize(0 /* use default weights */, &scribble_handle)) {
		printf("failed to initialize scribble\n");
		return;
	}

	// unsigned char* test_data = (unsigned char*)
	if(memory_recognize(scribble_handle, start_pixel,
			(int)box_size, (int)box_size,
			//(int)(x_box_max - x_box_min), (int)(y_box_max - y_box_min), 
			pitch_bytes, y_stride_bytes,
      &guessed_number)) {
		printf("Was the number a %d? \t(%f,%f to %f,%f)\n", guessed_number, x_box_min, y_box_min, x_box_max, y_box_max);
	} else {
		printf("Didn't see nothin\n");
	}

	cleanup(scribble_handle);
}
