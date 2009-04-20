#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dct.h"
#include "main.h"
#include "interface.h"
#include "render.h"



/*  Constants  */

#define SCALE_WIDTH        180
#define SPIN_BUTTON_WIDTH   75
#define RANDOM_SEED_WIDTH  100


/*  Local function prototypes  */


/*  Local variables  */

static PlugInUIVals *ui_state = NULL;


/*  Public functions  */

gboolean
gui_dialog (gint32				image_ID,
		   	GimpDrawable		*drawable,
		   	PlugInVals			*vals,
		   	PlugInImageVals		*image_vals,
		   	PlugInDrawableVals	*drawable_vals,
		   	PlugInUIVals		*ui_vals)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *main_hbox;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *blocksize_label;
  GtkWidget *alignment;
  GtkWidget *spinbutton;
  GtkObject *spinbutton_adj;
  GtkWidget *frame_label;
  gboolean   run;

  
  
  gimp_ui_init ("dctenergy", FALSE);

  dialog = gimp_dialog_new ("DCT Energy", "dctenergy",
                            NULL, 0,
                            gimp_standard_help_func, PLUGIN_NAME,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);
  
  
  
  	
  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, &(vals->preview));
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  
  
  
  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);

  alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_show (alignment);
  gtk_container_add (GTK_CONTAINER (frame), alignment);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 6, 6, 6);

  main_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (main_hbox);
  gtk_container_add (GTK_CONTAINER (alignment), main_hbox);

  blocksize_label = gtk_label_new_with_mnemonic ("_Block Size:");
  gtk_widget_show (blocksize_label);
  gtk_box_pack_start (GTK_BOX (main_hbox), blocksize_label, FALSE, FALSE, 6);
  gtk_label_set_justify (GTK_LABEL (blocksize_label), GTK_JUSTIFY_RIGHT);

  spinbutton = gimp_spin_button_new (&spinbutton_adj, vals->blocksize, 
                                     1, 32, 1, 1, 1, 5, 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  frame_label = gtk_label_new ("<b>Modify block size</b>");
  gtk_widget_show (frame_label);
  gtk_frame_set_label_widget (GTK_FRAME (frame), frame_label);
  gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (dct_energy),
                            drawable);
  g_signal_connect_swapped (spinbutton_adj, "value_changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  
  
  dct_energy (drawable, GIMP_PREVIEW (preview));
  
  
  g_signal_connect (spinbutton_adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &(vals->radius));
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
