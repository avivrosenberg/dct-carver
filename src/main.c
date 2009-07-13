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
    0.5f,   //edges
    0.5f,	//textures
    8,      //blocksize
    20,     //seams_number
    FALSE,	//new_layer
    TRUE,	//resize_canvas
    TRUE,	//output_energy
    TRUE,	//output_seams
    TRUE	//vertically
};

const PlugInImageVals default_image_vals = {
    0,  //image_id
    0,  //energy_image_id
    0   //seams_image_id
};

const PlugInDrawableVals default_drawable_vals = {
    0,  //drawable_id
    0,  //energy_layer_id
    0   //seams_layer_id
};

const PlugInUIVals default_ui_vals = {
    NULL,   //vals
    NULL,   //seams_number_spinbutton_adj
    0,      //width
    0,      //height
    NULL,   //preview
    NULL    //drawable
};

static  PlugInVals			vals;
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
        {GIMP_PDB_FLOAT,	"edges-factor",		"Edge preservation factor"		},
        {GIMP_PDB_FLOAT,	"textures-factor",	"Texture preservation factor"	},
        {GIMP_PDB_INT32,	"block-size",		"Size of DCT base images"		},
        {GIMP_PDB_INT32,	"seams-number",		"Number of seams to add/remove (can be negative)"},
        {GIMP_PDB_INT32,    "new-layer",        "Whether to output on a new layer"},
        {GIMP_PDB_INT32,    "resize-canvas",    "Whether to resize image canvas"},
        {GIMP_PDB_INT32,    "output-energy",    "Whether to output energy image"},
        {GIMP_PDB_INT32,    "output-seams",     "Whether to output seams image"},
        {GIMP_PDB_INT32,    "vertically",       "Direction (TRUE for vertically)"}
    };

    gimp_install_procedure(
        PLUGIN_NAME,
        "DCT Carver",
        "Seam-carving with a novel DCT-based energy function",
        "Aviv Rosenberg & Ben-Ami Zilber",
        "VISL, Technion Israel Institute of Technology",
        "July 2009",
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

    static GimpParam  return_values[5];
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GimpRunMode       run_mode;
    GimpDrawable     *drawable;
    gint32 			image_ID;
    gboolean        run_dialog = TRUE, run_render = TRUE;
    gint            dialog_resp, dialog_I_resp;

    /* Setting mandatory output values */
    *nreturn_vals = 5;
    *return_vals  = return_values;

    /* Getting run_mode - we won't display a dialog if
     * we are in NONINTERACTIVE mode */
    run_mode = param[0].data.d_int32;
    image_ID = param[1].data.d_image;
    /*  Get the specified drawable  */
    drawable = gimp_drawable_get(param[2].data.d_drawable);
    /*  Initialize with default values  */
    vals          = default_vals;
    image_vals    = default_image_vals;
    image_vals.image_id = image_ID;
    drawable_vals = default_drawable_vals;
    drawable_vals.drawable_id = param[2].data.d_drawable;
    ui_vals       = default_ui_vals;
    
    gimp_image_undo_group_start(image_ID); //make whole plugin a single undo step
    switch (run_mode) {

        case GIMP_RUN_NONINTERACTIVE:

            if (nparams != 12)
                status = GIMP_PDB_CALLING_ERROR;
            else {
                vals.edges = param[3].data.d_int32;
                vals.textures = param[4].data.d_int32;
                vals.blocksize = param[5].data.d_int32;
                vals.seams_number = param[6].data.d_int32;
                vals.new_layer = param[7].data.d_int32;
                vals.resize_canvas = param[8].data.d_int32;
                vals.output_energy = param[9].data.d_int32;
                vals.output_seams = param[10].data.d_int32;
                vals.vertically = param[11].data.d_int32;
            }

            break;

        case GIMP_RUN_INTERACTIVE:
            /* Get options last values if needed */
            gimp_get_data(DATA_KEY_VALS,    &vals);
            gimp_get_data(DATA_KEY_UI_VALS, &ui_vals);

            /* Display the dialog */
            while (run_dialog) {
                dialog_resp = gui_dialog(&vals, &image_vals, &drawable_vals, &ui_vals);
                switch(dialog_resp) {
                    case GTK_RESPONSE_OK:
                        run_dialog = FALSE;
                        break;
                    case DC_INTERACTIVE:
                        dialog_I_resp = gui_interactive_dialog(&vals, &image_vals, &drawable_vals);
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
            render(&vals, &image_vals, &drawable_vals);
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

    /*  Finally, set options in the core  
    if (run_mode == GIMP_RUN_INTERACTIVE) {
        gimp_set_data(DATA_KEY_VALS, &vals, sizeof(PlugInVals));
        gimp_set_data(DATA_KEY_UI_VALS, &ui_vals, sizeof(PlugInUIVals));
    } */

    return_values[0].type = GIMP_PDB_STATUS;
    return_values[0].data.d_status = status;
    return_values[1].type = GIMP_PDB_INT32;
    return_values[1].data.d_int32 = image_vals.energy_image_id;
    return_values[2].type = GIMP_PDB_INT32;
    return_values[2].data.d_int32 = drawable_vals.energy_layer_id;
    return_values[3].type = GIMP_PDB_INT32;
    return_values[3].data.d_int32 = image_vals.seams_image_id;
    return_values[4].type = GIMP_PDB_INT32;
    return_values[4].data.d_int32 = drawable_vals.seams_layer_id;
    gimp_image_undo_group_end(image_ID);
    return;
}
