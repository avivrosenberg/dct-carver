
#ifndef __RENDER_H__
#define __RENDER_H__


/*  Public functions  */

void   render (	gint32				image_ID,
				GimpDrawable		*drawable,
				PlugInVals			*vals,
				PlugInImageVals		*image_vals,
				PlugInDrawableVals	*drawable_vals);
	       
void dct_energy  (	GimpDrawable     *drawable,
					GimpPreview      *preview);
                         


#endif /* __RENDER_H__ */
