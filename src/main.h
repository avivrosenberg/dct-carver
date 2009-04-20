#ifndef __MAIN_H__
#define __MAIN_H__

/*  Constants  */

#define PLUGIN_NAME			"plug-in-dct-carver"
#define DATA_KEY_VALS    	"plug_in_dct_carver"
#define DATA_KEY_UI_VALS 	"plug_in_dct_carver_ui"
//#define PARASITE_KEY     	"plug-in-template-options"

typedef struct 
{
  gint      edges;
  gint      textures;
  gint      blocksize;
  gint		radius;
  gboolean	preview;
} PlugInVals;

typedef struct 
{
  gint32    image_id;
} PlugInImageVals;

typedef struct
{
  gint32    drawable_id;
} PlugInDrawableVals;

typedef struct 
{
  gboolean  chain_active;
} PlugInUIVals;


/*  Default values  */

extern	const	PlugInVals			default_vals;
extern	const	PlugInImageVals		default_image_vals;
extern	const	PlugInDrawableVals	default_drawable_vals;
extern	const	PlugInUIVals		default_ui_vals;
extern 			DCTAtomDB			dctAtomDB; /* Global database of dct atoms */


#endif /* __MAIN_H__ */
