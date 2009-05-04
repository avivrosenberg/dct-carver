#ifndef __RENDER_H__
#define __RENDER_H__

struct EnergyParameters_ {
	gfloat  edges;
	gfloat  textures;
	gint	blocksize;
} extra_pars;

typedef struct EnergyParameters_ EnergyParameters;

/*  Public functions  */

void   render(		  gint32		layer_ID,
			  PlugInVals		*vals,
			  PlugInImageVals	*image_vals,
			  PlugInDrawableVals	*drawable_vals);

void dct_energy(GimpDrawable     *drawable,
				GimpPreview      *preview);



#endif /* __RENDER_H__ */
