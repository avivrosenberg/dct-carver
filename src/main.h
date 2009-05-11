#ifndef __MAIN_H__
#define __MAIN_H__

/*  Constants  */

#define PLUGIN_NAME			"plug-in-dct-carver"
#define DATA_KEY_VALS    	"plug_in_dct_carver"
#define DATA_KEY_UI_VALS 	"plug_in_dct_carver_ui"

typedef enum direction_type_t {
	vertically = 0,
	horizontally
} direction_type;

typedef struct {
    gfloat    edges;
    gfloat    textures;
    gint      blocksize;
    gint	  seams_number;
    gboolean	preview;
    gboolean	new_layer;
    gboolean	resize_canvas;
    direction_type	direction;
} PlugInVals;

typedef struct {
    gint32    image_id;
} PlugInImageVals;

typedef struct {
    gint32    drawable_id;
} PlugInDrawableVals;

typedef struct {
    gboolean  chain_active;
} PlugInUIVals;


/*  Default values  */

extern	const	PlugInVals			default_vals;
extern	const	PlugInImageVals		default_image_vals;
extern	const	PlugInDrawableVals	default_drawable_vals;
extern	const	PlugInUIVals		default_ui_vals;
extern 			DCTAtomDB			dctAtomDB; /* Global database of dct atoms */
extern 			PlugInVals			vals;

#endif /* __MAIN_H__ */
