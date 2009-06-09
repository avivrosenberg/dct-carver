#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dct.h"
#include "main.h"
#include "interface.h"
#include "render.h"


/*
*/

/*  Local function prototypes  */

static void query(void);

static void run(const gchar      *name,
                gint              nparams,
                const GimpParam  *param,
                gint             *nreturn_vals,
                GimpParam       **return_vals);

/*
*/

/*  Local variables  */

const PlugInVals default_vals = {
    0.5f,	//edges
    0.5f,	//textures
    8,	//blocksize
    20,	//seams_number
    FALSE,	//preview
    FALSE,	//new_layer
    TRUE,	//resize_canvas
    TRUE,   //output_energy
    TRUE,	//vertically
    FALSE	//horizontally
};

const PlugInImageVals default_image_vals = {
    0
};

const PlugInDrawableVals default_drawable_vals = {
    0
};

const PlugInUIVals default_ui_vals = {
    TRUE
};

PlugInVals			vals;
static 	PlugInImageVals		image_vals;
static 	PlugInDrawableVals	drawable_vals;
static 	PlugInUIVals		ui_vals;
DCTAtomDB			dctAtomDB;


GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run,   /* run_proc   */
};

MAIN()

static void
query(void) {
    static GimpParamDef args[] = {
        {GIMP_PDB_INT32,	"run-mode",			"Run mode"			},
        {GIMP_PDB_IMAGE,	"image",			"Input image"		},
        {GIMP_PDB_DRAWABLE,	"drawable",			"Input drawable"	},
        {GIMP_PDB_FLOAT,	"edges-factor",		"edges-factor"		},
        {GIMP_PDB_FLOAT,	"textures-factor",	"textures-factor"	},
        {GIMP_PDB_INT32,	"seams-number",		"seams-number"		},
        {GIMP_PDB_INT32,	"block-size",		"block-size"		}
    };

    gimp_install_procedure(
        PLUGIN_NAME,
        "DCT Carver",
        "Computes an energy function for the image based on DCT atoms",
        "[author]",
        "[copyright]",
        "[date]",
        "DCT Carver",
        "RGB*, GRAY*",
        GIMP_PLUGIN,
        G_N_ELEMENTS(args), 0,
        args, NULL);

    gimp_plugin_menu_register(PLUGIN_NAME,
                              "<Image>/Filters/Misc");
}

static void
run(const gchar      *name,
    gint              nparams,
    const GimpParam  *param,
    gint             *nreturn_vals,
    GimpParam       **return_vals) {

    static GimpParam  values[1];
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GimpRunMode       run_mode;
    //GimpDrawable     *drawable_orig;
    GimpDrawable     *drawable;
    gint32 			image_ID;
    gint32			drawableID;

    /* Setting mandatory output values */
    *nreturn_vals = 1;
    *return_vals  = values;

    /* Getting run_mode - we won't display a dialog if
     * we are in NONINTERACTIVE mode */
    run_mode = param[0].data.d_int32;

    image_ID = param[1].data.d_image;

    /*  Get the specified drawable  */
    //drawable_orig = gimp_drawable_get(param[2].data.d_drawable);
    //drawableID = gimp_layer_new_from_drawable(param[2].data.d_drawable, image_ID);
    //gimp_image_add_layer(image_ID, drawableID, -1);
    //gimp_layer_flatten(param[2].data.d_drawable);
    //gimp_layer_flatten(drawableID);
    //drawable = gimp_drawable_get(drawableID);
    drawable = gimp_drawable_get(param[2].data.d_drawable);
    /*  Initialize with default values  */
    vals          = default_vals;
    image_vals    = default_image_vals;
    drawable_vals = default_drawable_vals;
    ui_vals       = default_ui_vals;

    init_dctatomdb(&dctAtomDB, vals.blocksize);

    switch (run_mode) {

        case GIMP_RUN_NONINTERACTIVE:

            if (nparams != 7)
                status = GIMP_PDB_CALLING_ERROR;
            else {
                vals.edges		= param[3].data.d_int32;
                vals.textures		= param[4].data.d_int32;
                vals.seams_number	= param[5].data.d_int32;
                vals.blocksize	= param[6].data.d_int32;
            }

            break;

        case GIMP_RUN_INTERACTIVE:
            /* Get options last values if needed */
            gimp_get_data(DATA_KEY_VALS,    &vals);
            gimp_get_data(DATA_KEY_UI_VALS, &ui_vals);

            /* Display the dialog */

            if (! gui_dialog(image_ID, drawable, &vals, &image_vals, &drawable_vals, &ui_vals)) {
                status = GIMP_PDB_CANCEL;
            }

            break;

        case GIMP_RUN_WITH_LAST_VALS:
            /*  Get options last values if needed  */
            gimp_get_data(DATA_KEY_VALS,    &vals);
            break;

        default:
            break;
    }

    if (status == GIMP_PDB_SUCCESS) {
        render(image_ID, &vals, &image_vals, &drawable_vals);

        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush();

        if (run_mode == GIMP_RUN_INTERACTIVE) {
            gimp_set_data(DATA_KEY_VALS,    &vals,    sizeof(vals));
            gimp_set_data(DATA_KEY_UI_VALS, &ui_vals, sizeof(ui_vals));
        }

        gimp_drawable_detach(drawable);

        //gimp_drawable_detach(drawable_orig);
    }

    /*  Finally, set options in the core  */
    if (run_mode == GIMP_RUN_INTERACTIVE) {
        gimp_set_data(DATA_KEY_VALS, &vals, sizeof(PlugInVals));
        gimp_set_data(DATA_KEY_UI_VALS, &ui_vals, sizeof(PlugInUIVals));
    }

    values[0].type = GIMP_PDB_STATUS;

    values[0].data.d_status = status;

    atomdb_free(dctAtomDB);

    return;
}
