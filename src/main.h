#ifndef __MAIN_H__
#define __MAIN_H__

#include <lqr.h>

/*  Constants  */

#define PLUGIN_NAME			"plug-in-dct-carver"
#define DATA_KEY_VALS    	"plug_in_dct_carver"
#define DATA_KEY_UI_VALS 	"plug_in_dct_carver_ui"

typedef struct {
    gfloat    edges;
    gfloat    textures;
    gint      blocksize;
    gint	  seams_number;
    gboolean	new_layer;
    gboolean	resize_canvas;
    gboolean	output_energy;
    gboolean	output_seams;
    gboolean	vertically;
} PlugInVals;

typedef struct {
    gint32 image_id;
    gint32 energy_image_id;
    gint32 seams_image_id;
} PlugInImageVals;

typedef struct {
    gint32 drawable_id;
    gint32 energy_layer_id;
    gint32 seams_layer_id;
} PlugInDrawableVals;

typedef struct {
    PlugInVals *vals;
    GtkObject *seams_number_spinbutton_adj;
    gint width;
    gint height;
    GimpPreview *preview;
    GimpDrawable *drawable; //previews' drawable
} PlugInUIVals;

typedef struct {
    PlugInVals *vals;
    gint old_width;
    gint old_height;
    gint x_off;
    gint y_off;
    gint image_ID;
    gint layer_ID;
    LqrCarver* carver;
} PlugInUIIVals;

typedef enum {DC_INTERACTIVE = 1, DC_BACK_TO_MAIN = 2} dc_dialog_response;

/*  Default values  */

extern	const	PlugInVals			default_vals;
extern	const	PlugInImageVals		default_image_vals;
extern	const	PlugInDrawableVals	default_drawable_vals;
extern	const	PlugInUIVals		default_ui_vals;

#endif /* __MAIN_H__ */
