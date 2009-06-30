#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

//#include "dct.h"
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
    TRUE,	//output_energy
    TRUE,	//output_seams
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
    NULL, //vals
    NULL, //seams_number_spinbutton_adj
    0, //width
    0, //height
    NULL //preview
};

PlugInVals			vals;
static 	PlugInImageVals		image_vals;
static 	PlugInDrawableVals	drawable_vals;
static 	PlugInUIVals		ui_vals;

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
        "Seam-carving with a novel DCT-based energy function",
        "[author]",
        "[copyright]",
        "[date]",
        "_DCT Carver",
        "RGB*, GRAY*",
        GIMP_PLUGIN,
        G_N_ELEMENTS(args), 0,
        args, NULL);

    gimp_plugin_menu_register(PLUGIN_NAME, "<Image>/Layer/");
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
    GimpDrawable     *drawable;
    gint32 			image_ID;
    gboolean        run_dialog = TRUE, run_render = TRUE;
    gint            dialog_resp, dialog_I_resp;

    /* Setting mandatory output values */
    *nreturn_vals = 1;
    *return_vals  = values;

    /* Getting run_mode - we won't display a dialog if
     * we are in NONINTERACTIVE mode */
    run_mode = param[0].data.d_int32;
    image_ID = param[1].data.d_image;

    /*  Get the specified drawable  */
    drawable = gimp_drawable_get(param[2].data.d_drawable);
    /*  Initialize with default values  */
    vals          = default_vals;
    image_vals    = default_image_vals;
    drawable_vals = default_drawable_vals;
    ui_vals       = default_ui_vals;
    gimp_image_undo_group_start(image_ID); //make whole plugin a single undo step
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
            while (run_dialog) {
                dialog_resp = gui_dialog(image_ID, drawable, &vals,
                                        &image_vals, &drawable_vals, &ui_vals);
                switch(dialog_resp) {
                    case GTK_RESPONSE_OK:
                        run_dialog = FALSE;
                        break;
                    case DC_INTERACTIVE:
                        dialog_I_resp = gui_interactive_dialog(image_ID, drawable->drawable_id, &vals);
                        switch(dialog_I_resp) {
                            case GTK_RESPONSE_OK:
                                run_dialog = FALSE;
                                run_render = FALSE;
                                break;
                            case DC_BACK_TO_MAIN:
                                run_dialog = TRUE;
                                break;
                            default:
                                run_dialog = FALSE;
                                run_render = FALSE;
                                status = GIMP_PDB_CANCEL;
                                break;
                        }
                        break;
                    default:
                        run_dialog = FALSE;
                        status = GIMP_PDB_CANCEL;
                }
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
        if (run_render == TRUE) {
            render(image_ID, &vals, &image_vals, &drawable_vals);
        }

        if (run_mode != GIMP_RUN_NONINTERACTIVE) {
            gimp_displays_flush();
        }

        if (run_mode == GIMP_RUN_INTERACTIVE) {
            gimp_set_data(DATA_KEY_VALS,    &vals,    sizeof(vals));
            gimp_set_data(DATA_KEY_UI_VALS, &ui_vals, sizeof(ui_vals));

            gimp_drawable_detach(drawable);
        }
    }

    /*  Finally, set options in the core  */
    if (run_mode == GIMP_RUN_INTERACTIVE) {
        gimp_set_data(DATA_KEY_VALS, &vals, sizeof(PlugInVals));
        gimp_set_data(DATA_KEY_UI_VALS, &ui_vals, sizeof(PlugInUIVals));
    }

    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;
    gimp_image_undo_group_end(image_ID);
    return;
}
