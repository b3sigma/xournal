#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

// DATA STRUCTURES AND CONSTANTS

#define PIXEL_MOTION_THRESHOLD 0.3
#define MAX_AXES 12
#define EPSILON 1E-7
#define MAX_ZOOM 20
#define DEFAULT_ZOOM 1.3333333333
#define MIN_ZOOM 0.2

/* a string (+ aux data) that maintains a refcount */

typedef struct Refstring {
  int nref;
  char *s;
  gpointer aux;
} Refstring;


/* The journal is mostly a list of pages. Each page is a list of layers,
   and a background. Each layer is a list of items, from bottom to top.
*/

typedef struct Background {
  int type;
  GnomeCanvasItem *canvas_item;
  int color_no;
  guint color_rgba;
  int ruling;
  GdkPixbuf *pixbuf;
  Refstring *filename;
  int file_domain;
  int file_page_seq;
  int pixbuf_dpi;      // for PDF only - the *current* dpi value
  double pixbuf_scale; // for PIXMAP, this is the *current* zoom value
                       // for PDF, this is the *requested* zoom value
} Background;

#define BG_SOLID 0
#define BG_PIXMAP 1
#define BG_PDF 2      // not implemented yet

#define RULING_NONE 0
#define RULING_LINED 1
#define RULING_RULED 2
#define RULING_GRAPH 3

#define DOMAIN_ABSOLUTE 0
#define DOMAIN_ATTACH 1
#define DOMAIN_CLONE 2  // only while loading file

typedef struct Brush {
  int tool_type;
  int color_no;
  guint color_rgba;
  int thickness_no;
  double thickness;
  int tool_options;
} Brush;

#define COLOR_BLACK      0
#define COLOR_BLUE       1
#define COLOR_RED        2
#define COLOR_GREEN      3
#define COLOR_GRAY       4
#define COLOR_LIGHTBLUE  5
#define COLOR_LIGHTGREEN 6
#define COLOR_MAGENTA    7
#define COLOR_ORANGE     8
#define COLOR_YELLOW     9
#define COLOR_WHITE     10
#define COLOR_OTHER     -1
#define COLOR_MAX       11

extern guint predef_colors_rgba[COLOR_MAX];
extern guint predef_bgcolors_rgba[COLOR_MAX];

#define THICKNESS_VERYFINE  0
#define THICKNESS_FINE      1
#define THICKNESS_MEDIUM    2
#define THICKNESS_THICK     3
#define THICKNESS_VERYTHICK 4
#define THICKNESS_MAX       5

#define TOOL_PEN          0
#define TOOL_ERASER       1
#define TOOL_HIGHLIGHTER  2
#define TOOL_TEXT         3
#define TOOL_SELECTREGION 4
#define TOOL_SELECTRECT   5
#define TOOL_VERTSPACE    6
#define NUM_STROKE_TOOLS  3

#define TOOLOPT_ERASER_STANDARD     0
#define TOOLOPT_ERASER_WHITEOUT     1
#define TOOLOPT_ERASER_STROKES      2

#define HILITER_ALPHA_MASK 0xffffff80

extern double predef_thickness[NUM_STROKE_TOOLS][THICKNESS_MAX];

typedef struct BBox {
  double left, right, top, bottom;
} BBox;

struct UndoErasureData;

typedef struct Item {
  int type;
  struct Brush brush; // the brush to use, if ITEM_STROKE
  GnomeCanvasPoints *path;
  GnomeCanvasItem *canvas_item; // the corresponding canvas item, or NULL
  struct BBox bbox;
  struct UndoErasureData *erasure; // for temporary use during erasures
} Item;

// item type values for Item.type, UndoItem.type, ui.cur_item_type ...
// (not all are valid in all places)
#define ITEM_NONE -1
#define ITEM_STROKE 0
#define ITEM_TEMP_STROKE 1
#define ITEM_ERASURE 2
#define ITEM_SELECTRECT 3
#define ITEM_MOVESEL 4
#define ITEM_PASTE 5
#define ITEM_NEW_LAYER 6
#define ITEM_DELETE_LAYER 7
#define ITEM_NEW_BG_ONE 8
#define ITEM_NEW_BG_RESIZE 9
#define ITEM_PAPER_RESIZE 10
#define ITEM_NEW_DEFAULT_BG 11
#define ITEM_NEW_PAGE 13
#define ITEM_DELETE_PAGE 14

typedef struct Layer {
  GList *items; // the items on the layer, from bottom to top
  int nitems;
  GnomeCanvasGroup *group;
} Layer;

typedef struct Page {
  GList *layers; // the layers on the page
  int nlayers;
  double height, width;
  double hoffset, voffset; // offsets of canvas group rel. to canvas root
  struct Background *bg;
  GnomeCanvasGroup *group;
} Page;

typedef struct Journal {
  GList *pages;  // the pages in the journal
  int npages;
  int last_attach_no; // for naming of attached backgrounds
} Journal;

typedef struct Selection {
  int type;
  BBox bbox; // the rectangle bbox of the selection
  struct Layer *layer; // the layer on which the selection lives
  double anchor_x, anchor_y, last_x, last_y; // for selection motion
  GnomeCanvasItem *canvas_item; // if the selection box is on screen 
  GList *items; // the selected items (a list of struct Item)
} Selection;

typedef struct UIData {
  int pageno, layerno; // the current page and layer
  struct Page *cur_page;
  struct Layer *cur_layer;
  int toolno;  // the number of the currently selected tool
  gboolean saved; // is file saved ?
  struct Brush *cur_brush;  // the brush in use (one of brushes[...])
  struct Brush brushes[NUM_STROKE_TOOLS]; // the current pen, eraser, hiliter
  struct Brush default_brushes[NUM_STROKE_TOOLS]; // the default ones
  gboolean ruler; // whether we're in ruler mode
  struct Page default_page;  // the model for the default page
  int layerbox_length;  // the number of entries registered in the layers combo-box
  struct Item *cur_item; // the item being drawn, or NULL
  int cur_item_type;
  GnomeCanvasPoints cur_path; // the path being drawn
  int cur_path_storage_alloc;
  int which_mouse_button; // the mouse button drawing the current path
  int saved_toolno;  // while using an eraser device
  gboolean saved_ruler;
  double zoom; // zoom factor, in pixels per pt
  gboolean use_xinput; // use input devices instead of core pointer
  gboolean allow_xinput; // allow use of xinput ?
  int screen_width, screen_height; // initial screen size, for XInput events
  char *filename;
  gboolean view_continuous, fullscreen;
  gboolean in_update_page_stuff; // semaphore to avoid scrollbar retroaction
  struct Selection *selection;
  GdkCursor *cursor;
  gboolean emulate_eraser;
  gboolean antialias_bg; // bilinear interpolation on bg pixmaps
  gboolean progressive_bg; // rescale bg's one at a time
} UIData;

typedef struct UndoErasureData {
  struct Item *item; // the item that got erased
  int npos; // its position in its layer
  int nrepl; // the number of replacement items
  GList *replacement_items;
} UndoErasureData;

typedef struct UndoItem {
  int type;
  struct Item *item; // for ITEM_STROKE
  struct Layer *layer; // for ITEM_STROKE, ITEM_ERASURE, ITEM_PASTE, ITEM_NEW_LAYER, ITEM_DELETE_LAYER
  struct Layer *layer2; // for ITEM_DELETE_LAYER with val=-1
  struct Page *page;  // for ITEM_NEW_BG_ONE/RESIZE, ITEM_NEW_PAGE, ITEM_NEW_LAYER, ITEM_DELETE_LAYER, ITEM_DELETE_PAGE
  GList *erasurelist; // for ITEM_ERASURE
  GList *itemlist;  // for ITEM_MOVESEL, ITEM_PASTE
  struct Background *bg;  // for ITEM_NEW_BG_ONE/RESIZE, ITEM_NEW_DEFAULT_BG
  int val; // for ITEM_NEW_PAGE, ITEM_NEW_LAYER, ITEM_DELETE_LAYER, ITEM_DELETE_PAGE
  double val_x, val_y; // for ITEM_MOVESEL, ITEM_NEW_BG_RESIZE, ITEM_PAPER_RESIZE, ITEM_NEW_DEFAULT_BG
  struct UndoItem *next;
  int multiop;
} UndoItem;

#define MULTIOP_CONT_REDO 1 // not the last in a multiop, so keep redoing
#define MULTIOP_CONT_UNDO 2 // not the first in a multiop, so keep undoing


typedef struct BgPdfRequest {
  int pageno;
  int dpi;
  gboolean initial_request; // if so, loop over page numbers
  gboolean is_printing;     // this is for printing, not for display
} BgPdfRequest;

typedef struct BgPdfPage {
  int dpi;
  GdkPixbuf *pixbuf;
} BgPdfPage;

typedef struct BgPdf {
  int status; // the rest only makes sense if this is not STATUS_NOT_INIT
  int pid; // PID of the converter process
  Refstring *filename;
  int file_domain;
  gchar *tmpfile_copy; // the temporary work copy of the file (in tmpdir)
  int npages;
  GList *pages; // a list of BgPdfPage structures
  GList *requests; // a list of BgPdfRequest structures
  gchar *tmpdir; // where to look for pages coming from pdf converter
  gboolean create_pages; // create journal pages as we find stuff in PDF
  gboolean has_failed; // has failed in the past...
} BgPdf;

#define STATUS_NOT_INIT 0
#define STATUS_IDLE     1
#define STATUS_RUNNING  2  // currently running child process on head(requests)
#define STATUS_ABORTED  3  // child process running, but head(requests) aborted
#define STATUS_SHUTDOWN 4  // waiting for child process to shut down

// UTILITY MACROS

// getting a component of the interface by name
#define GET_COMPONENT(a)  GTK_WIDGET (g_object_get_data(G_OBJECT (winMain), a))

// the margin between consecutive pages in continuous view
#define VIEW_CONTINUOUS_SKIP 20.0


// GLOBAL VARIABLES

// the main window and the canvas

extern GtkWidget *winMain;
extern GnomeCanvas *canvas;

// the data

extern struct Journal journal;
extern struct UIData ui;
extern struct BgPdf bgpdf;
extern struct UndoItem *undo, *redo;