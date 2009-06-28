#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <lqr.h>

#include "dct.h"
#include "main.h"
#include "interface.h"
#include "render.h"

/* 	Local variables */

/* 	Private functions */

void new_image_from_layer_with_filename(gint* new_image_ID, gint* new_layer_ID, gint layer_ID, gchar* filename) {
    gint width = gimp_drawable_width(layer_ID);
    gint height = gimp_drawable_height(layer_ID);

    *new_image_ID = gimp_image_new(width, height, GIMP_RGB);
    *new_layer_ID = gimp_layer_new_from_drawable(layer_ID, *new_image_ID);
    gimp_image_add_layer(*new_image_ID, *new_layer_ID, -1);
    gimp_drawable_set_name(*new_layer_ID, filename);
    gimp_image_set_filename(*new_image_ID, filename);
    return;
}

static void
dct_energy_preview_rows(guchar **current_rows, gdouble *energy_image, gint row_number, gint width) {
    gint j;
    gint blocksize = vals.blocksize;
    gint ii, jj;
    gdouble max_in_pixel;
    double** data = alloc_2d_double(blocksize,blocksize);
    int* ip = alloc_1d_int(2 + (int) sqrt(blocksize/2 + 0.5));
    double* w = alloc_1d_double(blocksize*3/2);

    ip[0] = 0;
    for (j = 0; j < width; j++) {
        gint left = j - (CENTER_COL(blocksize) - 1);
        gint right = j + blocksize - CENTER_COL(blocksize);
        max_in_pixel = 0;
        
        for (ii = 0; ii < blocksize; ii++) {
            for (jj = left; jj <= right; jj++) {
                data[ii][jj-left] = current_rows[ii][CLAMP(jj, 0, width - 1)];
            }
        }
        dctNxN(blocksize, data, ip, w);
        max_in_pixel = weighted_max_dct_correlation(blocksize, data, vals.edges, vals.textures); 

        energy_image[row_number*width + j] = max_in_pixel;
    } //j
    free_2d_double(data);
    free_1d_int(ip);
    free_1d_double(w);
}

static void
convert_row_to_luminance(guchar* inrow, guchar* outrow, gint channels, gint width) {
    gint j;

    if (channels == 1) {
        for (j = 0; j < width; j++) {
            outrow[j] = inrow[j];
        }
        return;
    } else if (channels < 3) {
        error("Number of channels not 1 or 3\n");
        return;
    }

    for (j = 0; j < width; j++) {
        outrow[j] = RGB2LUMINANCE(inrow[j*channels+0], inrow[j*channels+1], inrow[j*channels+2]);
    }
}

static void
normalize_image(gdouble* energy_image, guchar* output_image, gint height, gint width, gint channels) {
    gint i, j, k;
    gdouble maxval, minval, currval;
    guchar outval;

    maxval = energy_image[0]; minval = maxval;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            currval = energy_image[i * width + j];
            if (currval > maxval)
                maxval = currval;
            if (currval < minval)
                minval = currval;
        }
    }

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            outval = DOUBLE2GUCHAR(energy_image[i * width + j], minval, maxval);
            for (k = 0; k < channels; k++) {
                output_image[i*width*channels + j*channels + k] = outval;
            }
        }
    }

    return;
}


LqrProgress* progress_init() {
    LqrProgress * progress = lqr_progress_new();
    lqr_progress_set_init(progress, (LqrProgressFuncInit) gimp_progress_init);
    lqr_progress_set_update(progress, (LqrProgressFuncUpdate) gimp_progress_update);
    lqr_progress_set_end(progress, (LqrProgressFuncEnd) gimp_progress_end);
    lqr_progress_set_init_width_message(progress, ("Resizing width..."));
    lqr_progress_set_init_height_message(progress, ("Resizing height..."));
    return progress;
}

gint clamp_offset_to_border(gint base, gint offset, gint lower_border, gint upper_border) {
    if (((base + offset) - lower_border) < 0) {
        return (offset - ((base + offset) - lower_border));
    }

    if (((base + offset) - upper_border) > 0) {
        return (offset - ((base + offset) - upper_border));
    }

    return offset;
}

gfloat dct_pixel_energy(gint x, gint y, gint w, gint h, LqrReadingWindow *rw, gpointer extra_data) {
    /* read parameters */
    EnergyParameters * params = (EnergyParameters *) extra_data;
    gint blocksize = params->blocksize;
    gfloat edges = params->edges;
    gfloat textures = params->textures;
    double** data = params->data;

    gfloat max;
    gint i,j,ii,jj;
    gint radius = lqr_rwindow_get_radius(rw);

    for (i = -radius + 1; i <= radius; i++) {
        for (j = -radius + 1; j <= radius; j++) {
            ii = clamp_offset_to_border(x, i, 0, w - 1);
            jj = clamp_offset_to_border(y, j, 0, h - 1);
            data[i+radius-1][j+radius-1] = lqr_rwindow_read(rw, ii, jj, 0);
        }
    }
   
    dctNxN(blocksize, data, params->ip, params->w);
    max = weighted_max_dct_correlation(blocksize, data, edges, textures);
    return max; 
}

guchar* rgb_buffer_from_layer(gint layer_ID) {
    guchar* rgb_buffer;
    gint w = gimp_drawable_width(layer_ID);
    gint h = gimp_drawable_height(layer_ID);
    gint bpp = gimp_drawable_bpp(layer_ID);
    rgb_buffer = g_try_new(guchar, bpp * w * h);
    GimpDrawable* drawable = gimp_drawable_get(layer_ID);
    GimpPixelRgn rgn_in;

    gimp_pixel_rgn_init(&rgn_in, drawable, 0, 0, w, h, FALSE, FALSE);
    gimp_pixel_rgn_get_rect(&rgn_in, rgb_buffer, 0, 0, w, h);

    return rgb_buffer;

}

void display_carver_energy(GimpDrawable *drawable, gint w, gint h, gint channels, LqrCarver *carver) {

    guchar *output_image; 
    GimpPixelRgn rgn_out;

    gimp_pixel_rgn_init(&rgn_out, drawable, 0, 0, w, h, TRUE, FALSE);

    output_image = g_new(guchar, w * h * channels);

    lqr_carver_get_energy_image(carver, output_image, 0, lqr_carver_get_col_depth(carver) ,lqr_carver_get_image_type(carver)); //0 = horizontal orientation; should not matter
    gimp_pixel_rgn_set_rect(&rgn_out, output_image, 0, 0, w, h); 

    gimp_drawable_update(drawable->drawable_id, 0, 0, w, h);
    gimp_drawable_detach(drawable);

    g_free(output_image);
}

void display_carver_seams(GimpDrawable *drawable, LqrCarver *carver) {

    LqrVMap *vmap;
    LqrVMapList *list;
    gint* buffer;
    gint w, h, depth, vis, x, y;
    GimpPixelRgn rgn;
    guchar pixel_value[] = {0, 255, 0};

    list = lqr_vmap_list_start(carver);
    vmap = lqr_vmap_list_current(list); //Should only be one vmap in the list
    buffer = lqr_vmap_get_data(vmap);
    w = lqr_vmap_get_width(vmap);
    h = lqr_vmap_get_height(vmap);
    depth = lqr_vmap_get_depth(vmap);
    gimp_pixel_rgn_init(&rgn, drawable, 0, 0, w, h, TRUE, FALSE);
    
    for(x = 0; x < w-1; x++) {
        for(y = 0; y < h-1; y++) {
            vis = buffer[y * w + x];
            if (vis == 0) {
                continue;
            } else {
                pixel_value[1] = (guchar)(255.0 * ((gdouble)vis)/((gdouble)depth));
                gimp_pixel_rgn_set_pixel(&rgn, pixel_value, x, y); 
            }
        }
    }

    gimp_drawable_update(drawable->drawable_id, 0, 0, w, h);
    gimp_drawable_detach(drawable);
}

/*  Public functions  */

LqrRetVal write_carver_to_layer(LqrCarver * r, gint32 layer_ID) {
    GimpDrawable * drawable;
    gint y;
    gint w, h;
    GimpPixelRgn rgn_out;
    guchar *out_line;
    gint update_step;

    gimp_progress_init(("Applying changes..."));
    update_step = MAX((lqr_carver_get_height(r) - 1) / 20, 1);

    drawable = gimp_drawable_get(layer_ID);

    w = gimp_drawable_width(layer_ID);
    h = gimp_drawable_height(layer_ID);

    gimp_pixel_rgn_init(&rgn_out, drawable, 0, 0, w, h, TRUE, FALSE);

    while (lqr_carver_scan_line(r, &y, &out_line)) {
        if (lqr_carver_scan_by_row(r)) {
            gimp_pixel_rgn_set_row(&rgn_out, out_line, 0, y, w);
        } else {
            gimp_pixel_rgn_set_col(&rgn_out, out_line, y, 0, h);
        }

        if (y % update_step == 0) {
            gimp_progress_update((gdouble) y / (lqr_carver_get_height(r) - 1));
        }
    }

    gimp_drawable_flush(drawable);
    //gimp_drawable_merge_shadow(layer_ID, FALSE);
    gimp_drawable_update(layer_ID, 0, 0, w, h);
    gimp_drawable_detach(drawable);
    gimp_displays_flush();
    gimp_progress_end();
    return LQR_OK;
}

LqrCarver* init_carver_from_vals(gint layer_ID, PlugInVals *vals, EnergyParameters *params) {
    LqrCarver *carver;
    LqrProgress* progress;
    gint bpp;
    gint old_width, old_height;
    gint seams_number;
    guchar* rgb_buffer;

    progress = progress_init();
    seams_number = vals->seams_number;
    params->edges = vals->edges;
    params->textures = vals->textures;
    params->blocksize = vals->blocksize;
    params->ip = alloc_1d_int(2 + (int) sqrt(vals->blocksize/2 + 0.5));
    params->w = alloc_1d_double(vals->blocksize*3/2);
    params->data = alloc_2d_double(vals->blocksize, vals->blocksize);
    params->ip[0] = 0; 

    old_width = gimp_drawable_width(layer_ID);
    old_height = gimp_drawable_height(layer_ID);
    bpp = gimp_drawable_bpp(layer_ID);
    rgb_buffer = rgb_buffer_from_layer(layer_ID);

    carver = lqr_carver_new(rgb_buffer, old_width, old_height, bpp);
    lqr_carver_init(carver, 1, 0); //numbers are delta_x and rigidity
    lqr_carver_set_energy_function(carver, dct_pixel_energy, vals->blocksize / 2, LQR_ER_LUMA, (void*) params);
    lqr_carver_set_progress(carver, progress);

    return carver;
}

void render(gint32 image_ID, PlugInVals *vals, PlugInImageVals *image_vals, PlugInDrawableVals *drawable_vals) {

    EnergyParameters params;
    LqrCarver *carver;
    LqrProgress* progress = progress_init();
    LqrResizeOrder res_order = LQR_RES_ORDER_HOR;
    gint bpp;
    gint old_width, old_height;
    gint new_width, new_height;
    gint seams_number = vals->seams_number;
    gint layer_ID = gimp_image_get_active_layer(image_ID);
    gint32 energy_image_ID, energy_layer_ID = -1;
    gint32 seams_image_ID, seams_layer_ID = -1;
    guchar* rgb_buffer;
    gint delta_x = 1;
    gint rigidity = 0;
    gint x_off,y_off;
    gchar new_layer_name[LQR_MAX_NAME_LENGTH];

    params.edges = vals->edges;
    params.textures = vals->textures;
    params.blocksize = vals->blocksize;
    params.ip = alloc_1d_int(2 + (int) sqrt(vals->blocksize/2 + 0.5));
    params.w = alloc_1d_double(vals->blocksize*3/2);
    params.data = alloc_2d_double(vals->blocksize, vals->blocksize);
    params.ip[0] = 0; 

    if (vals->new_layer) {
      g_snprintf(new_layer_name, LQR_MAX_NAME_LENGTH, "%s (copy)", gimp_drawable_get_name(layer_ID));
      layer_ID = gimp_layer_copy(layer_ID);
      gimp_image_add_layer(image_ID, layer_ID, -1);
      gimp_drawable_set_name(layer_ID, new_layer_name);
      gimp_drawable_set_visible(layer_ID, FALSE);
    }

    old_width = gimp_drawable_width(layer_ID);
    old_height = gimp_drawable_height(layer_ID);
    bpp = gimp_drawable_bpp(layer_ID);
    gimp_drawable_offsets(layer_ID, &x_off, &y_off);
    rgb_buffer = rgb_buffer_from_layer(layer_ID);

    if (vals->vertically) { //check resize direction
    	new_width = old_width;
        new_height = old_height + seams_number;
        res_order = LQR_RES_ORDER_VERT; 
    } else {
        new_width = old_width + seams_number;
        new_height = old_height;
    }

    carver = lqr_carver_new(rgb_buffer, old_width, old_height, bpp);
    lqr_carver_init(carver, delta_x, rigidity);
    lqr_carver_set_energy_function(carver, dct_pixel_energy, vals->blocksize / 2, LQR_ER_LUMA, (void*) &params);

    if (vals->output_energy == TRUE) {
        g_snprintf(new_layer_name, LQR_MAX_NAME_LENGTH, "Energy (b=%d, e=%.2f, t=%.2f)",
                   vals->blocksize, vals->edges, vals->textures);
        new_image_from_layer_with_filename(&energy_image_ID, &energy_layer_ID, layer_ID, new_layer_name);
        display_carver_energy(gimp_drawable_get(energy_layer_ID), old_width, old_height, bpp, carver);
    }

    if (vals->output_seams == TRUE) {
        lqr_carver_set_dump_vmaps(carver);
        g_snprintf(new_layer_name, LQR_MAX_NAME_LENGTH, "Seams (b=%d, e=%.2f, t=%.2f)",
                   vals->blocksize, vals->edges, vals->textures);
        new_image_from_layer_with_filename(&seams_image_ID, &seams_layer_ID, layer_ID, new_layer_name);
    } 

    lqr_carver_set_progress(carver, progress);
    lqr_carver_set_resize_order (carver, res_order);
    lqr_carver_set_use_cache(carver, TRUE);
    lqr_carver_resize(carver, new_width, new_height);

    if (vals->resize_canvas == TRUE) {
        gimp_image_resize (image_ID, new_width, new_height, -x_off, -y_off);
        gimp_layer_resize_to_image_size (layer_ID);
    } else {
        gimp_layer_resize (layer_ID, new_width, new_height, 0, 0);
    }

    if (vals->output_seams == TRUE) {
        display_carver_seams(gimp_drawable_get(seams_layer_ID), carver);
    } 

    gint ntiles = new_width / gimp_tile_width() + 1;
    gimp_tile_cache_size((gimp_tile_width() * gimp_tile_height() * ntiles * 4 * 2) / 1024 + 1);
    write_carver_to_layer(carver, layer_ID);
    lqr_carver_destroy(carver);

    gimp_drawable_set_visible (layer_ID, TRUE);
    gimp_image_set_active_layer (image_ID, layer_ID);
    if (energy_layer_ID != -1) {
        gimp_display_new(energy_image_ID);
    }
    if (seams_layer_ID != -1) {
        gimp_display_new(seams_image_ID);
    }

    free_1d_int(params.ip);
    free_1d_double(params.w);
    free_2d_double(params.data);

    return;
}

void dct_energy_preview(GimpDrawable *drawable, GimpPreview  *preview) {
    
    gint blocksize;
    gint x1, y1, x2, y2, width, height;
    gint i, row_number;
    gint channels;
    GimpPixelRgn rgn_in, rgn_out;
    guchar **current_rows, *tmp_row; // blocksize rows from the image
    gdouble* energy_image; // Actual energy image
    guchar *output_image; //normalized energy image (for display)
    gint update_step;

  
    if (preview) {
        gimp_preview_get_position(preview, &x1, &y1);
        gimp_preview_get_size(preview, &width, &height);
        x2 = x1 + width;
		y2 = y1 + height;
    } else {
		gimp_drawable_mask_bounds(drawable->drawable_id, &x1, &y1, &x2, &y2);
		width = x2 - x1;
		height = y2 - y1;
        gimp_progress_init(("Computing energy image..."));
        update_step = MAX((height - 1) / 20, 1);
	}
    gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));
    gimp_pixel_rgn_init(&rgn_in, drawable, x1, y1, width, height, FALSE, FALSE);
    gimp_pixel_rgn_init(&rgn_out, drawable, x1, y1, width, height, preview == NULL, TRUE);

    blocksize = vals.blocksize;
    channels = gimp_drawable_bpp(drawable->drawable_id);
    current_rows = g_new(guchar*, blocksize);
    tmp_row = g_new(guchar, width * channels);
    energy_image = g_new(gdouble, height * width); //energy image has only one channel
    output_image = g_new(guchar, height * width * channels); //output needs to have as many channels as the image

    for (i = 0; i < blocksize; i++) {
        current_rows[i] = g_new(guchar, width);
        gimp_pixel_rgn_get_row(&rgn_in, tmp_row, x1, y1 + CLAMP(i - (CENTER_ROW(blocksize) - 1), 0, height - 1), width);
        convert_row_to_luminance(tmp_row, current_rows[i], channels, width);
    }

    for (row_number = 0; row_number < height; row_number++) { // loop on all rows in image (preview)
        dct_energy_preview_rows(current_rows, energy_image, row_number, width);

        g_free(current_rows[0]);
        gimp_pixel_rgn_get_row(&rgn_in, tmp_row, x1, MIN(row_number + y1 + blocksize - (CENTER_ROW(blocksize) - 1), y1 + height - 1), width);
        for (i = 1; i < blocksize; i++) { // shuffle current rows
            current_rows[i - 1] = current_rows[i];
        }
        current_rows[blocksize-1] = g_new(guchar, width);
        convert_row_to_luminance(tmp_row, current_rows[blocksize-1], channels, width);

        if (!preview && (row_number % update_step == 0)) {
            gimp_progress_update((gdouble) row_number / (height - 1));
        }
    }

    normalize_image(energy_image, output_image, height, width, channels);
    gimp_pixel_rgn_set_rect(&rgn_out, output_image, x1, y1, width, height); 

    for (i = 0; i < blocksize; i++) {
        g_free(current_rows[i]);
    }
    g_free(current_rows);
    g_free(tmp_row);
    g_free(energy_image);
    g_free(output_image);

    if (preview) {
		gimp_drawable_preview_draw_region(GIMP_DRAWABLE_PREVIEW(preview), &rgn_out);
	} else {
		gimp_drawable_flush(drawable);
		gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);
		gimp_drawable_update(drawable->drawable_id, x1, y1, width, height);
	}
    gimp_progress_end();
    return;
}


