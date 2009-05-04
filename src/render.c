#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <lqr.h>

#include "dct.h"
#include "main.h"
#include "render.h"

/* 	Local variables */

/* 	Private functions */

static void
init_mem(gint blocksize,
         guchar ***row,
         guchar  **outrow,
         gint      num_bytes) {
    gint i;

    /* Allocate enough memory for row and outrow */
    *row = g_new(guchar *, blocksize);

    for (i = 0; i < blocksize; i++)
        (*row)[i] = g_new(guchar, num_bytes);

    *outrow = g_new(guchar, num_bytes);
}

static void
process_row(guchar **row,
            gdouble  *max_in_pixel_buf,
            gint	  line_number,
            gint     width,
            gint     channels,
            gdouble  *max_in_picture,
            gdouble  *min_in_picture,
            gboolean *is_first_row) {
    gint j;
    gint blocksize = dctAtomDB.blocksize;

    for (j = 0; j < width; j++) {
        gint k, ii, jj;
        gint left = j - (CENTER_COL(blocksize) - 1),
                    right = j + blocksize - CENTER_COL(blocksize);

        /* For each layer, compute the maximum product of the
         * (BLOCK_SIZE-1)x(BLOCK_SIZE-1) pixels with the DCT atoms*/

        for (k = 0; k < channels; k++) {
            gdouble max_in_pixel = 0; //working with positive values only
            gdouble min_in_pixel;
            gint k1, k2;
            gint max_index = 0;
            gdouble factor = 1.0f;

            for (k1 = 0; k1 < blocksize; k1++) {
                for (k2 = 0; k2 < blocksize; k2++) {
                    if ((!k1) && (!k2)) continue;

                    gdouble sum = 0, fsum;

                    DCTAtom atom = get_atom(dctAtomDB, k1, k2);

                    for (ii = 0; ii < blocksize; ii++) {
                        for (jj = left; jj <= right; jj++) {
                            gint x = ii;
                            gint y = jj - left;
                            sum += ((gdouble)row[ii][channels * CLAMP(jj, 0, width - 1) + k]) * atom.matrix[x][y];
                        }
                    }

                    if (ABS(sum) > max_in_pixel) {
                        max_in_pixel = ABS(sum);
                        max_index = (k1 * blocksize + k2);
                        factor = (IS_EDGE_ATOM(blocksize, k1, k2) ? ((gdouble)vals.edges) : ((gdouble)vals.textures));
                    }

                    //   if ((k1==0)&&(k2==0)) { min_in_pixel = ABS(sum); }
                    //   else if(ABS(sum) < min_in_pixel) { min_in_pixel = ABS(sum); }
                }
            }

            max_in_pixel *= factor;

            max_in_pixel_buf[line_number*width*channels + j*channels + k] = max_in_pixel;
            //max_in_pixel_buf[line_number*width*channels + j*channels + k] = 4*max_index;

            if (max_in_pixel > (*max_in_picture)) {
                (*max_in_picture) = max_in_pixel;
            }

            if (*is_first_row) {
                (*min_in_picture) = max_in_pixel;
                (*is_first_row) = FALSE;
            } else if (max_in_pixel < (*min_in_picture)) {
                (*min_in_picture) = max_in_pixel;
            }
        }
    }
}

static void
shuffle(GimpPixelRgn *rgn_in,
        guchar      **row,
        gint          x1,
        gint          y1,
        gint          width,
        gint          height,
        gint          ypos) {

    gint    i;
    guchar *tmp_row;
    gint blocksize = dctAtomDB.blocksize;

    /* Get tile row (i + BLOCK_SIZE + 1) into row[0] */
    gimp_pixel_rgn_get_row(rgn_in,
                           row[0],
                           x1, MIN(ypos + y1 + blocksize - (CENTER_ROW(blocksize) - 1), y1 + height - 1),
                           width);

    /* Permute row[i] with row[i-1] and row[0] with row[2r] */
    tmp_row = row[0];

    for (i = 1; i < blocksize; i++)
        row[i - 1] = row[i];

    row[blocksize-1] = tmp_row;
}

/*
 * */

LqrProgress* progress_init() {
    LqrProgress * progress = lqr_progress_new();
    lqr_progress_set_init(progress, (LqrProgressFuncInit) gimp_progress_init);
    lqr_progress_set_update(progress, (LqrProgressFuncUpdate) gimp_progress_update);
    lqr_progress_set_end(progress, (LqrProgressFuncEnd) gimp_progress_end);
    lqr_progress_set_init_width_message(progress, ("Resizing width..."));
    lqr_progress_set_init_height_message(progress, ("Resizing height..."));
    return progress;
}


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

    gimp_pixel_rgn_init(&rgn_out, drawable, 0, 0, w, h, TRUE, TRUE);

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

    gimp_drawable_merge_shadow(layer_ID, TRUE);
    gimp_drawable_update(layer_ID, 0, 0, w, h);
    gimp_drawable_detach(drawable);
    gimp_progress_end();
    return LQR_OK;
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

gdouble convolve(gint k1, gint k2, gint x, gint y, gint w, gint h, LqrReaderWindow *rw) {

    gint i, ii, j, jj;
    gint radius = lqr_rwindow_get_radius(rw);  //radius of the window is equal to dct atom blocksize
    gdouble sum = 0;
    DCTAtom atom = get_atom(dctAtomDB, k1, k2);

    for (i = -radius + 1; i <= radius; i++) {
        for (j = -radius + 1; j <= radius; j++) {
            /* lqr_rwindow_read (rw, i, j, 0) reads the image brightness
             * at pixel (x + i, y + j)
             * The last argument (i.e. the channel) is 0 because
             * we're using LQR_ER_BRIGHT (which only returns one channel) */
            ii = clamp_offset_to_border(x, i, 0, h - 1);
            jj = clamp_offset_to_border(y, j, 0, w - 1);
            sum += atom.matrix[i+radius-1][j+radius-1] * lqr_rwindow_read(rw, ii, jj, 0);
        }
    }

    return ABS(sum);
}

gfloat dct_pixel_energy(gint x, gint y, gint w, gint h, LqrReaderWindow *rw, gpointer extra_data) {
    /* read parameters */
    EnergyParameters * params = (EnergyParameters *) extra_data;
    gint k1, k2;
    gint blocksize = params->blocksize;
    gfloat edges = params->edges;
    gfloat textures = params->textures;
    gdouble factor = 1.0;
    gdouble max_sum = 0;
    gdouble curr_sum;

    for (k1 = 0; k1 < blocksize; k1++) {
        for (k2 = 0; k2 < blocksize; k2++) {
            if ((!k1) && (!k2)) continue;

            curr_sum = convolve(k1, k2, x, y, w, h, rw);

            if (curr_sum > max_sum) {
                max_sum = curr_sum;
                factor = (IS_EDGE_ATOM(blocksize, k1, k2) ? (edges) : (textures));
            }
        }
    }

    return ((gfloat) max_sum*factor);
}

guchar* rgb_buffer_from_layer(gint layer_ID) {
    guchar* rgb_buffer;
    gint w = gimp_drawable_width(layer_ID);
    gint h = gimp_drawable_height(layer_ID);
    gint bpp = gimp_drawable_bpp(layer_ID);
    TRY_N_N(rgb_buffer = g_try_new(guchar, bpp * w * h));
    GimpDrawable* drawable = gimp_drawable_get(layer_ID);
    GimpPixelRgn rgn_in;

    gimp_pixel_rgn_init(&rgn_in, drawable, 0, 0, w, h, FALSE, FALSE);
    gimp_pixel_rgn_get_rect(&rgn_in, rgb_buffer, 0, 0, w, h);

    return rgb_buffer;

}

/*  Public functions  */

void render(gint32 image_ID, PlugInVals *vals, PlugInImageVals *image_vals, PlugInDrawableVals *drawable_vals) {

    EnergyParameters params;
    LqrCarver *carver;
    LqrProgress* progress = progress_init();
    gint y, bpp;
    gint old_width, old_height;
    gint new_width, new_height;
    gint seams_number = vals->seams_number;
    GimpDrawable *drawable;
    GimpPixelRgn rgn_in;
    gint layer_ID = gimp_image_get_active_layer(image_ID);
    guchar* rgb_buffer = rgb_buffer_from_layer(layer_ID);
    gint delta_x = 1;
    gint rigidity = 0;

    params.edges = vals->edges;
    params.textures = vals->textures;
    params.blocksize = vals->blocksize;

    old_width = gimp_drawable_width(layer_ID);
    old_height = gimp_drawable_height(layer_ID);
    bpp = gimp_drawable_bpp(layer_ID);
    seams_number = vals->seams_number;

    if (1) { //check resize direction
        new_width = old_width - seams_number;
        new_height = old_height;
    } else {
        new_width = old_width;
        new_height = old_height - seams_number;
    }

    carver = lqr_carver_new(rgb_buffer, old_width, old_height, bpp);

    lqr_carver_set_energy_function(carver, dct_pixel_energy, vals->blocksize / 2, LQR_ER_BRIGHT, (void*) &params);
    lqr_carver_init(carver, delta_x, rigidity);
    lqr_carver_set_progress(carver, progress);
    lqr_carver_resize(carver, new_width, new_height);

    gimp_layer_resize(layer_ID, new_width, new_height, 0, 0);

    gint ntiles = new_width / gimp_tile_width() + 1;
    gimp_tile_cache_size((gimp_tile_width() * gimp_tile_height() * ntiles * 4 * 2) / 1024 + 1);
    write_carver_to_layer(carver, layer_ID);
}

void dct_energy(GimpDrawable *drawable,
                GimpPreview  *preview) {
    gint         i, j, k, ii, channels;
    gint         x1, y1, x2, y2;
    GimpPixelRgn rgn_in, rgn_out;
    guchar     **row;
    guchar      *outrow;
    gint         width, height;
    gint 		   blocksize;

    if (vals.blocksize != dctAtomDB.blocksize) {
        atomdb_free(dctAtomDB);
        init_dctatomdb(&dctAtomDB, vals.blocksize);
    }

    blocksize = dctAtomDB.blocksize;

    if (! preview)
        gimp_progress_init("Computing DCT energy function...");

    /* Gets upper left and lower right coordinates,
     * and layers number in the image */
    if (preview) {
        gimp_preview_get_position(preview, &x1, &y1);
        gimp_preview_get_size(preview, &width, &height);
        x2 = x1 + width;
        y2 = y1 + height;
    } else {
        gimp_drawable_mask_bounds(drawable->drawable_id,
                                  &x1, &y1,
                                  &x2, &y2);
        width = x2 - x1;
        height = y2 - y1;
    }

    channels = gimp_drawable_bpp(drawable->drawable_id);

    /* Allocate a big enough tile cache */
    gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

    /* Initialises two PixelRgns, one to read original data,
     * and the other to write output data. That second one will
     * be merged at the end by the call to
     * gimp_drawable_merge_shadow() */
    gimp_pixel_rgn_init(&rgn_in,
                        drawable,
                        x1, y1,
                        width, height,
                        FALSE, FALSE);
    gimp_pixel_rgn_init(&rgn_out,
                        drawable,
                        x1, y1,
                        width, height,
                        preview == NULL, TRUE);


    /* Allocate memory for input and output tile rows */
    init_mem(blocksize, &row, &outrow, width * channels);


    for (ii = 0; ii < blocksize; ii++) {
        gimp_pixel_rgn_get_row(&rgn_in,
                               row[ii],
                               x1, y1 + CLAMP(ii - (CENTER_ROW(blocksize) - 1), 0, height - 1),
                               width);
    }

    gdouble max_in_picture = 0; //working with positive values only

    gdouble min_in_picture;
    gboolean is_first_row = TRUE;
    gdouble* max_in_pixel_buf = g_new(gdouble, height * width * channels);
    guchar* output = g_new(guchar, height * width * channels);


    for (i = 0; i < height; i++) {
        /* To be done for each tile row */

        process_row(row,
                    max_in_pixel_buf,
                    i,
                    width,
                    channels,
                    &max_in_picture,
                    &min_in_picture,
                    &is_first_row);
        //gimp_pixel_rgn_set_row (&rgn_out,
        //outrow,
        //x1, y1 + i,
        //width);

        /* shift tile rows to insert the new one at the end */

        shuffle(&rgn_in,
                row,
                x1, y1,
                width, height,
                i);

        if (! preview && i % 16 == 0)
            gimp_progress_update((gdouble) i / (gdouble) height);
    }


    for (k = 0; k < channels; k++) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                output[i*width*channels + j*channels + k] =
                    DOUBLE2GUCHAR(max_in_pixel_buf[i*width*channels + j*channels + k], min_in_picture, max_in_picture);
            }
        }
    }

    gimp_pixel_rgn_set_rect(&rgn_out,

                            output,
                            x1, y1,
                            width, height);

    /* We could also put that in a separate function but it's
     * rather simple */

    for (ii = 0; ii < blocksize; ii++) {
        g_free(row[ii]);
    }

    g_free(row);

    g_free(outrow);
    g_free(max_in_pixel_buf);
    g_free(output);


    /*  Update the modified region */

    if (preview) {
        gimp_drawable_preview_draw_region(GIMP_DRAWABLE_PREVIEW(preview),
                                          &rgn_out);
    } else {
        gimp_drawable_flush(drawable);
        gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);
        gimp_drawable_update(drawable->drawable_id,
                             x1, y1,
                             width, height);
    }
}
