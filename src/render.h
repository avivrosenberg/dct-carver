
#ifndef __RENDER_H__
#define __RENDER_H__

#define RGB2LUMINANCE(r,g,b) ((guchar)(16.0 + (r)*0.2568 + (g)*0.5041 + (b)*0.0979))
#define DOUBLE2GUCHAR(d,min,max) ((guchar)(ROUND(255*(((d) - (min))/((max)-(min))))))
#define GUCHAR2DOUBLE(g) (((gdouble)g)/255.0)

struct EnergyParameters_ {
    gfloat  edges;
    gfloat  textures;
    gint	blocksize;
    int*    ip;
    double* w;
    double** data;
};

typedef struct EnergyParameters_ EnergyParameters;

/*  Public functions  */

void
render(gint32 layer_ID, PlugInVals *vals, PlugInImageVals *image_vals, PlugInDrawableVals *drawable_vals);

void
dct_energy_preview(GimpDrawable *drawable, GimpPreview *preview);

#endif /* __RENDER_H__ */
