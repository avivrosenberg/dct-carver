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

void new_image_from_layer_with_filename(gint* new_image_ID, gint* new_layer_ID, gint layer_ID, gint width, gint height, gchar* filename) {

    if (layer_ID != -1) { //seams image
        *new_image_ID = gimp_image_new(width, height, GIMP_RGB);
        *new_layer_ID = gimp_layer_new_from_drawable(layer_ID, *new_image_ID);
        gimp_drawable_set_name(*new_layer_ID, filename);
    } else { // energy image
        *new_image_ID = gimp_image_new(width, height, GIMP_GRAY);
        *new_layer_ID = gimp_layer_new(*new_image_ID, filename, width, height, GIMP_GRAY_IMAGE, 100.0, GIMP_NORMAL_MODE);
    }
    gimp_image_add_layer(*new_image_ID, *new_layer_ID, -1);
    gimp_image_set_filename(*new_image_ID, filename);
    return;
}

static void
dct_energy_preview_rows(PlugInVals *vals, guchar **current_rows, gdouble *energy_image, gint row_number, gint width) {
    gint j;
    gint blocksize = vals->blocksize;
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
        max_in_pixel = weighted_max_dct_correlation(blocksize, data, vals->edges, vals->textures); 

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

void display_carver_energy(GimpDrawable *drawable, CarverData carver_data) {
    LqrCarver* carver;
    gint w, h;
    gint channels;
    guchar *output_image; 
    GimpPixelRgn rgn_out;

    gimp_progress_init("Calculating energy function...");
    carver = carver_data.carver;
    w = carver_data.old_width;
    h = carver_data.old_height;
    channels = carver_data.channels;
    gimp_pixel_rgn_init(&rgn_out, drawable, 0, 0, w, h, TRUE, FALSE);
    gimp_progress_update(10);
    output_image = g_new(guchar, w * h * channels);

    lqr_carver_get_energy_image(carver, (void*)output_image, 0, LQR_COLDEPTH_8I, LQR_GREY_IMAGE); //0 = horizontal orientation; should not matter
    gimp_progress_update(40);
    gimp_pixel_rgn_set_rect(&rgn_out, output_image, 0, 0, w, h); 
    gimp_progress_update(70);

    gimp_drawable_update(drawable->drawable_id, 0, 0, w, h);
    gimp_drawable_detach(drawable);
    gimp_progress_update(100);

    g_free(output_image);
    gimp_progress_end();
}

void display_carver_seams(GimpDrawable *drawable, LqrCarver *carver) {

    LqrVMap *vmap;
    LqrVMapList *list;
    gint* buffer;
    gint w, h, depth, vis, x, y;
    GimpPixelRgn rgn;
    guchar pixel_value[] = {0, 255, 0};

    gimp_progress_init("Outputting seams...");
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
        if((x % 50) == 0) {
            gimp_progress_update( ((gdouble)x)/((gdouble)w) );
        }
    }

    gimp_drawable_update(drawable->drawable_id, 0, 0, w, h);
    gimp_drawable_detach(drawable);
    gimp_progress_end();
}

/*  Public functions  */

void write_carver_to_layer(LqrCarver * r, gint32 layer_ID, gboolean progress_update) {
    GimpDrawable * drawable;
    gint y;
    gint w, h;
    GimpPixelRgn rgn_out;
    guchar *out_line;
    gint update_step = 0;

    if (progress_update) {
        gimp_progress_init(("Applying changes..."));
        update_step = MAX((lqr_carver_get_height(r) - 1) / 20, 1);
    }

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

        if ((progress_update) && (y % update_step == 0)) {
            gimp_progress_update((gdouble) y / (lqr_carver_get_height(r) - 1));
        }
    }

    gimp_drawable_flush(drawable);
    //gimp_drawable_merge_shadow(layer_ID, FALSE);
    gimp_drawable_update(layer_ID, 0, 0, w, h);
    gimp_drawable_detach(drawable);
    gimp_displays_flush();
    if (progress_update) {
        gimp_progress_end();
    }
}

CarverData init_carver_from_vals(gint layer_ID, PlugInVals *vals) {
    CarverData carver_data;
    LqrCarver *carver;
    EnergyParameters *energy_params;
    LqrProgress* progress;
    gint bpp;
    gint old_width, old_height;
    gint seams_number;
    guchar* rgb_buffer;

    energy_params = g_new(EnergyParameters, 1);
    progress = progress_init();
    seams_number = vals->seams_number;
    energy_params->edges = vals->edges;
    energy_params->textures = vals->textures;
    energy_params->blocksize = vals->blocksize;
    energy_params->ip = alloc_1d_int(2 + (int) sqrt(vals->blocksize/2 + 0.5));
    energy_params->w = alloc_1d_double(vals->blocksize*3/2);
    energy_params->data = alloc_2d_double(vals->blocksize, vals->blocksize);
    energy_params->ip[0] = 0; 

    old_width = gimp_drawable_width(layer_ID);
    old_height = gimp_drawable_height(layer_ID);
    bpp = gimp_drawable_bpp(layer_ID);
    rgb_buffer = rgb_buffer_from_layer(layer_ID);

    carver = lqr_carver_new(rgb_buffer, old_width, old_height, bpp);
    lqr_carver_init(carver, 1, 0); //numbers are delta_x and rigidity
    lqr_carver_set_energy_function(carver, dct_pixel_energy, 
                                   vals->blocksize / 2, LQR_ER_LUMA, (void*) energy_params);
    lqr_carver_set_progress(carver, progress);

    carver_data.carver = carver;
    carver_data.progress = progress;
    carver_data.old_width = old_width;
    carver_data.old_height = old_height;
    carver_data.channels = bpp;
    carver_data.energy_params = energy_params;
    return carver_data;
}

void render(PlugInVals *vals, PlugInImageVals *image_vals, PlugInDrawableVals *drawable_vals) {

    CarverData carver_data;
    LqrCarver *carver;
    gint32 image_ID;
    gint32 layer_ID;
    gint old_width, old_height;
    gint new_width, new_height;
    gint32 energy_image_ID, energy_layer_ID = -1;
    gint32 seams_image_ID, seams_layer_ID = -1;
    gint x_off,y_off;
    gchar new_layer_name[LQR_MAX_NAME_LENGTH];

    image_ID = image_vals->image_id;
    layer_ID = drawable_vals->drawable_id;
    carver_data = init_carver_from_vals(layer_ID, vals);
    carver = carver_data.carver;
    old_width = carver_data.old_width;
    old_height = carver_data.old_height;

    if (vals->new_layer) {
      g_snprintf(new_layer_name, LQR_MAX_NAME_LENGTH, "%s (copy)", gimp_drawable_get_name(layer_ID));
      layer_ID = gimp_layer_copy(layer_ID);
      gimp_image_add_layer(image_ID, layer_ID, -1);
      gimp_drawable_set_name(layer_ID, new_layer_name);
      gimp_drawable_set_visible(layer_ID, FALSE);
    }

    old_width = gimp_drawable_width(layer_ID);
    old_height = gimp_drawable_height(layer_ID);

    if (vals->vertically) { //check resize direction
    	new_width = old_width;
        new_height = old_height + vals->seams_number;
    } else {
        new_width = old_width + vals->seams_number;
        new_height = old_height;
    }

    if (vals->output_energy) {
        g_snprintf(new_layer_name, LQR_MAX_NAME_LENGTH, "Energy (b=%d, e=%.2f, t=%.2f)",
                   vals->blocksize, vals->edges, vals->textures);
        new_image_from_layer_with_filename(&energy_image_ID, &energy_layer_ID, -1, old_width, old_height, new_layer_name);
        display_carver_energy(gimp_drawable_get(energy_layer_ID), carver_data);
    }

    if ((vals->output_seams) && (vals->seams_number != 0)) {
        lqr_carver_set_dump_vmaps(carver);
    } 

    lqr_carver_resize(carver, new_width, new_height);

    if ((vals->output_seams) && (vals->seams_number != 0)) {
        g_snprintf(new_layer_name, LQR_MAX_NAME_LENGTH, "Seams (b=%d, e=%.2f, t=%.2f)",
                   vals->blocksize, vals->edges, vals->textures);
        new_image_from_layer_with_filename(&seams_image_ID, &seams_layer_ID, layer_ID, old_width, old_height, new_layer_name);
        display_carver_seams(gimp_drawable_get(seams_layer_ID), carver);
    } 

    if (vals->resize_canvas == TRUE) {
        gimp_drawable_offsets(layer_ID, &x_off, &y_off);
        gimp_image_resize (image_ID, new_width, new_height, -x_off, -y_off);
        gimp_layer_resize_to_image_size (layer_ID);
    } else {
        gimp_layer_resize (layer_ID, new_width, new_height, 0, 0);
    }

    gint ntiles = new_width / gimp_tile_width() + 1;
    gimp_tile_cache_size((gimp_tile_width() * gimp_tile_height() * ntiles * 4 * 2) / 1024 + 1);
    write_carver_to_layer(carver, layer_ID, TRUE);

    if (vals->new_layer) {
        gimp_drawable_set_visible(layer_ID, TRUE);
        gimp_image_set_active_layer (image_ID, layer_ID);
    }
    if (energy_layer_ID != -1) {
        image_vals->energy_image_id = energy_image_ID;
        drawable_vals->energy_layer_id = energy_layer_ID;
        gimp_display_new(energy_image_ID);
    }
    if (seams_layer_ID != -1) {
        image_vals->seams_image_id = seams_image_ID;
        drawable_vals->seams_layer_id = seams_layer_ID;
        gimp_display_new(seams_image_ID);
    }

    lqr_carver_destroy(carver);
    free_1d_int(carver_data.energy_params->ip);
    free_1d_double(carver_data.energy_params->w);
    free_2d_double(carver_data.energy_params->data);

    return;
}

void dct_energy_preview(PlugInUIVals *ui_vals, GimpPreview  *preview) {
    
    GimpDrawable *drawable;
    gint blocksize;
    gint x1, y1, x2, y2, width, height;
    gint i, row_number;
    gint channels;
    GimpPixelRgn rgn_in, rgn_out;
    guchar **current_rows, *tmp_row; // blocksize rows from the image
    gdouble* energy_image; // Actual energy image
    guchar *output_image; //normalized energy image (for display)
    gint update_step = 0;

    drawable = ui_vals->drawable;

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

    blocksize = ui_vals->vals->blocksize;
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
        dct_energy_preview_rows(ui_vals->vals, current_rows, energy_image, row_number, width);

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


