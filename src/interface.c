#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dct.h"
#include "main.h"
#include "interface.h"
#include "render.h"



/*  Constants  */

#define SCALE_WIDTH        180
#define SPIN_BUTTON_WIDTH   75

/*  Local function prototypes  */


/*  Local variables  */

static PlugInUIVals *ui_state = NULL;


/*  Public functions  */

gboolean
gui_dialog(gint32 image_ID, GimpDrawable *drawable, PlugInVals *vals, PlugInImageVals *image_vals, 
           PlugInDrawableVals *drawable_vals, PlugInUIVals *ui_vals) {

    GtkWidget *dialog;
    GtkWidget *main_vbox;
    GtkWidget *main_hbox;
    GtkWidget *preview;
    GtkWidget *blocksize_frame;
    GtkWidget *sliders_frame;
    GtkWidget *blocksize_label;
    GtkWidget *alignment;
    GtkWidget *spinbutton;
    GtkObject *spinbutton_adj;
    GtkObject *edges_adj;
    GtkObject *textures_adj;
    GtkWidget *blocksize_frame_label;
    GtkWidget *sliders_frame_label;
    GtkWidget *sliders_table;
    gboolean   run;
    gint row;



    gimp_ui_init("dct-carver", FALSE);

    dialog = gimp_dialog_new("DCT Carver", "dct-carver",
                             NULL, 0,
                             gimp_standard_help_func, PLUGIN_NAME,
                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                             GTK_STOCK_OK,     GTK_RESPONSE_OK,
                             NULL);

    main_vbox = gtk_vbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);
    gtk_widget_show(main_vbox);

    preview = gimp_drawable_preview_new(drawable, &(vals->preview));
    gimp_preview_set_update(GIMP_PREVIEW(preview), vals->preview);
    gtk_box_pack_start(GTK_BOX(main_vbox), preview, TRUE, TRUE, 0);
    gtk_widget_show(preview);

    blocksize_frame = gtk_frame_new(NULL);
    gtk_widget_show(blocksize_frame);
    gtk_box_pack_start(GTK_BOX(main_vbox), blocksize_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(blocksize_frame), 6);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment);
    gtk_container_add(GTK_CONTAINER(blocksize_frame), alignment);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 6, 6, 6);

    main_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(main_hbox);
    gtk_container_add(GTK_CONTAINER(alignment), main_hbox);

    blocksize_label = gtk_label_new_with_mnemonic("_Block Size:");
    gtk_widget_show(blocksize_label);
    gtk_box_pack_start(GTK_BOX(main_hbox), blocksize_label, FALSE, FALSE, 6);
    gtk_label_set_justify(GTK_LABEL(blocksize_label), GTK_JUSTIFY_RIGHT);

    spinbutton = gimp_spin_button_new(&spinbutton_adj, vals->blocksize,
                                      2, 16, 1, 1, 1, 5, 0);
    gtk_box_pack_start(GTK_BOX(main_hbox), spinbutton, FALSE, FALSE, 0);
    gtk_widget_show(spinbutton);

    blocksize_frame_label = gtk_label_new("<b>Modify block size</b>");
    gtk_widget_show(blocksize_frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(blocksize_frame), blocksize_frame_label);
    gtk_label_set_use_markup(GTK_LABEL(blocksize_frame_label), TRUE);


    /* Sliders Table:
     * */
    sliders_frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(main_vbox), sliders_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(sliders_frame), 6);
    gtk_widget_show(sliders_frame);

    sliders_frame_label = gtk_label_new("<b>Modify egde and texture preservation factors</b>");
    gtk_widget_show(sliders_frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(sliders_frame), sliders_frame_label);
    gtk_label_set_use_markup(GTK_LABEL(sliders_frame_label), TRUE);

    sliders_table = gtk_table_new(2, 3, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(sliders_table), 6);
    gtk_table_set_row_spacings(GTK_TABLE(sliders_table), 2);
    gtk_container_add(GTK_CONTAINER(sliders_frame), sliders_table);
    gtk_widget_show(sliders_table);

    row = 0;

    edges_adj = gimp_scale_entry_new(GTK_TABLE(sliders_table), 0, row++,
                                     ("Edges:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
                                     vals->edges, 0.0, 1.0, 0.01, 0.1, 2,
                                     TRUE, 0, 0,
                                     ("Scale factor for DCT atoms coresponding to edges"), NULL);

    textures_adj = gimp_scale_entry_new(GTK_TABLE(sliders_table), 0, row++,
                                        ("Textures:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
                                        vals->textures, 0.0, 1.0, 0.01, 0.1, 2,
                                        TRUE, 0, 0,
                                        ("Scale factor for DCT atoms coresponding to textures"), NULL);

    /* Signals:
     * */
    g_signal_connect_swapped(preview, "invalidated",
                             G_CALLBACK(dct_energy_preview),
                             drawable);

    g_signal_connect_swapped(spinbutton_adj, "value_changed",
                             G_CALLBACK(gimp_preview_invalidate),
                             preview);

    g_signal_connect_swapped(edges_adj, "value_changed",
                             G_CALLBACK(gimp_preview_invalidate),
                             preview);

    g_signal_connect_swapped(textures_adj, "value_changed",
                             G_CALLBACK(gimp_preview_invalidate),
                             preview);

    g_signal_connect(spinbutton_adj, "value_changed",
                     G_CALLBACK(gimp_int_adjustment_update),
                     &(vals->blocksize));

    g_signal_connect(edges_adj, "value_changed",
                     G_CALLBACK(gimp_float_adjustment_update),
                     &(vals->edges));

    g_signal_connect(textures_adj, "value_changed",
                     G_CALLBACK(gimp_float_adjustment_update),
                     &(vals->textures));

    gtk_widget_show(dialog);

    run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);

    gtk_widget_destroy(dialog);

    return run;
}

void
error(const gchar* message) {
	gimp_message(message);
}

