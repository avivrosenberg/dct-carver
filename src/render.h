
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

struct CarverData_ {
    LqrCarver* carver;
    LqrProgress* progress;
    gint old_width;
    gint old_height;
    gint channels;
    EnergyParameters* energy_params; //needed in order to free it
};

typedef struct CarverData_ CarverData;

/*  Public functions  */

void
write_carver_to_layer(LqrCarver * r, gint32 layer_ID, gboolean progress_update);

CarverData
init_carver_from_vals(gint layer_ID, PlugInVals *vals);

void
render(gint32 layer_ID, PlugInVals *vals, PlugInImageVals *image_vals, PlugInDrawableVals *drawable_vals);

void
dct_energy_preview(GimpDrawable *drawable, GimpPreview *preview);

#endif /* __RENDER_H__ */
