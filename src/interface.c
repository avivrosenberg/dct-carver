#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <lqr.h>


#include "dct.h"
#include "main.h"
#include "interface.h"
#include "render.h"



/*  Constants  */

#define SCALE_WIDTH        180
#define SPIN_BUTTON_WIDTH   75

/*  Local function prototypes  */

void callback_toggle_checkbox(GtkToggleButton *toggle_button, gpointer data);
void callback_change_blocksize(GimpIntComboBox *box, gpointer data);
void callback_preference_slider(GtkHScale *slider, gpointer data);
//void callback_change_preference(GimpPreview *gimppreview, gpointer data);
void callback_resize_slider(GtkHScale *slider, gpointer data);
void callback_update_spinbutton_boundries_vert(GtkToggleButton *toggle_button, gpointer data);
void callback_update_spinbutton_boundries_horz(GtkToggleButton *toggle_button, gpointer data);


/*  Local variables  */

//static PlugInUIVals *ui_state = NULL;


/*  Public functions  */

gint
gui_interactive_dialog(PlugInVals *vals, PlugInImageVals *image_vals,
                       PlugInDrawableVals *drawable_vals) {

    gint32 image_ID;
    gint32 layer_ID;
    gint response_id;
    gint old_width, old_height;
    CarverData carver_data;
    LqrCarver *carver;
    LqrProgress* progress;
    PlugInUIIVals ui_i_vals;

    GtkWidget *dialog;
    GtkWidget *main_vbox;
    GtkWidget *slider_frame;
    GtkWidget *slider_alignment;
    GtkWidget *slider_hbox;
    GtkWidget *min_label;
    GtkWidget *slider_hscale;
    GtkWidget *max_label;
    GtkWidget *slider_frame_label;
    gchar min_label_str[LQR_MAX_NAME_LENGTH];
    gchar max_label_str[LQR_MAX_NAME_LENGTH];

    image_ID = image_vals->image_id;
    layer_ID = drawable_vals->drawable_id; 

    dialog = gimp_dialog_new("DCT Carver Interactive", "dct-carver",
                             NULL, 0,
                             gimp_standard_help_func, PLUGIN_NAME,
                             GTK_STOCK_GO_BACK, DC_BACK_TO_MAIN,
                             GTK_STOCK_OK,     GTK_RESPONSE_OK,
                             NULL);
                             
    main_vbox = gtk_vbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);
    gtk_widget_show(main_vbox);
    
    slider_frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(main_vbox), slider_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(slider_frame), 6);
    gtk_widget_show(slider_frame);

	slider_alignment = gtk_alignment_new(0.5, 0.5, 1, 0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(slider_alignment), 6, 6, 6, 6);
    gtk_widget_show(slider_alignment);
    gtk_container_add(GTK_CONTAINER(slider_frame), slider_alignment);
    
	slider_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(slider_hbox);
    gtk_container_add(GTK_CONTAINER(slider_alignment), slider_hbox);
    
    g_snprintf(min_label_str, LQR_MAX_NAME_LENGTH, "%d seams",-1*vals->seams_number);
    min_label = gtk_label_new_with_mnemonic(min_label_str);
   // gtk_box_pack_start(GTK_BOX(slider_hbox), min_label, FALSE, FALSE, 1);
    gtk_widget_show(min_label);
    gtk_label_set_justify(GTK_LABEL(min_label), GTK_JUSTIFY_RIGHT);
    
    slider_hscale = gtk_hscale_new_with_range(-1*(ABS(vals->seams_number)), ABS(vals->seams_number), 1);
    gtk_range_set_value(GTK_RANGE(slider_hscale), 0);
    gtk_range_set_update_policy(GTK_RANGE(slider_hscale), GTK_UPDATE_CONTINUOUS);
   	gtk_scale_set_draw_value(GTK_SCALE(slider_hscale), TRUE);
   	gtk_range_set_show_fill_level(GTK_RANGE(slider_hscale), TRUE);
   	gtk_range_set_fill_level(GTK_RANGE(slider_hscale), ABS(vals->seams_number));
    gtk_widget_show(slider_hscale);
    gtk_box_pack_start(GTK_BOX(slider_hbox), slider_hscale, TRUE, TRUE, 5);
    
    g_snprintf(max_label_str, LQR_MAX_NAME_LENGTH, "%d seams",vals->seams_number);
    max_label = gtk_label_new_with_mnemonic(max_label_str);
    //gtk_box_pack_start(GTK_BOX(slider_hbox), max_label, FALSE, FALSE, 1);
    gtk_widget_show(max_label);
    gtk_label_set_justify(GTK_LABEL(max_label), GTK_JUSTIFY_RIGHT);
    
	slider_frame_label = gtk_label_new("<b>Resize</b>");
    gtk_widget_show(slider_frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(slider_frame), slider_frame_label);
    gtk_label_set_use_markup(GTK_LABEL(slider_frame_label), TRUE);

    carver_data = init_carver_from_vals(layer_ID, vals);
    carver = carver_data.carver;
    progress = carver_data.progress;
    old_width = carver_data.old_width;
    old_height = carver_data.old_height;
    ui_i_vals.vals = vals;
    ui_i_vals.carver = carver;
    ui_i_vals.old_width = old_width;
    ui_i_vals.old_height = old_height;
    gimp_drawable_offsets(layer_ID, &(ui_i_vals.x_off), &(ui_i_vals.y_off));
    ui_i_vals.image_ID = image_ID;
    ui_i_vals.layer_ID = layer_ID;

    lqr_progress_set_init_width_message(progress, ("Calculating seams..."));
    lqr_progress_set_init_height_message(progress, ("Calculating seams..."));
    if(vals->vertically) {
        lqr_carver_resize(carver, old_width, old_height + vals->seams_number);
    } else {
        lqr_carver_resize(carver, old_width + vals->seams_number, old_height);
    }

	g_signal_connect(slider_hscale, "value_changed",
					 G_CALLBACK(callback_resize_slider),
					 (gpointer)(&ui_i_vals));
	
    gtk_widget_show(dialog);
    response_id = gimp_dialog_run(GIMP_DIALOG(dialog));
    if (response_id == DC_BACK_TO_MAIN) {
    	gtk_range_set_value(GTK_RANGE(slider_hscale), 0);
		}
    gtk_widget_destroy(dialog);
    lqr_carver_destroy(carver);

    free_1d_int(carver_data.energy_params->ip);
    free_1d_double(carver_data.energy_params->w);
    free_2d_double(carver_data.energy_params->data);

    return response_id;
}

gint
gui_dialog(PlugInVals *vals, PlugInImageVals *image_vals, 
           PlugInDrawableVals *drawable_vals, PlugInUIVals *ui_vals) {

    gint32 image_ID;
    GimpDrawable *drawable;

    GtkWidget *dialog;
    GtkWidget *main_vbox;
    GtkWidget *energy_hbox;
    GtkWidget *energy_vbox;
    GtkWidget *carver_hbox;
    GtkWidget *separator;
    
    GtkWidget *preview;
    
    GtkWidget *blocksize_hbox;
    GtkWidget *blocksize_frame;
    GtkWidget *blocksize_label;
    GtkWidget *blocksize_alignment;
    GtkWidget *blocksize_combobox;
    //GtkObject *blocksize_spinbutton_adj;
    GtkWidget *blocksize_frame_label;
    
    GtkWidget *sliders_frame;
    GtkWidget *sliders_alignment;
    GtkWidget *sliders_frame_label;
    //GtkWidget *sliders_table;
    GtkWidget *sliders_hbox;
    GtkWidget *slider_hscale;
    GtkWidget *edges_label;
    GtkWidget *textures_label;
    //GtkObject *edges_adj;
    //GtkObject *textures_adj;
    gint response_id;
    //gint       row;
    
    GtkWidget *resize_frame;
    GtkWidget *resize_vbox;
    GtkWidget *resize_frame_label;
    
    gint width, height, seams_bound;
    GtkWidget *seams_number_label;
    GtkWidget *seams_number_alignment;
    GtkWidget *seams_number_hbox;
    GtkWidget *seams_number_spinbutton;
    GtkObject *seams_number_spinbutton_adj;
    
    GtkWidget *options_frame;
    GtkWidget *options_vbox;
    GtkWidget *new_layer_button;
    GtkWidget *resize_canvas_button;
    GtkWidget *output_energy_button;
    GtkWidget *output_seams_button;
    GtkWidget *options_frame_label;
    
    
    GtkWidget *vert;
    GtkWidget *horizon;
    GtkWidget *radio_frame;
    GtkWidget *radio_frame_label;
    GtkWidget *radio_vbox;
    
    image_ID = image_vals->image_id;
    drawable = gimp_drawable_get(drawable_vals->drawable_id);
	ui_vals->vals = vals;

    gimp_ui_init("dct-carver", FALSE);

    dialog = gimp_dialog_new("DCT Carver", "dct-carver",
                             NULL, 0,
                             gimp_standard_help_func, PLUGIN_NAME,
                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                             GTK_STOCK_OK,     GTK_RESPONSE_OK,
                             "_Interactive", DC_INTERACTIVE,
                             NULL);

    main_vbox = gtk_vbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);
    gtk_widget_show(main_vbox);
    
    //energy parameters - start
    
    energy_hbox = gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(main_vbox), energy_hbox, TRUE, TRUE, 0);
    gtk_widget_show(energy_hbox);
	
	//preview - start
	
    preview = gimp_drawable_preview_new(drawable, NULL);
    gtk_box_pack_start(GTK_BOX(energy_hbox), preview, TRUE, TRUE, 0);
    gtk_widget_show(preview);
    
    ui_vals->preview = GIMP_PREVIEW(preview);
    ui_vals->drawable = drawable;
    
	//preview - end

	energy_vbox = gtk_vbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(energy_hbox), energy_vbox, TRUE, TRUE, 0);
    gtk_widget_show(energy_vbox);
	

	//blocksize - start

    blocksize_frame = gtk_frame_new(NULL);
    gtk_widget_show(blocksize_frame);
    gtk_box_pack_start(GTK_BOX(energy_vbox), blocksize_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(blocksize_frame), 6);

    blocksize_alignment = gtk_alignment_new(0, 0.5, 0, 0);
    gtk_widget_show(blocksize_alignment);
    gtk_container_add(GTK_CONTAINER(blocksize_frame), blocksize_alignment);
    gtk_alignment_set_padding(GTK_ALIGNMENT(blocksize_alignment), 6, 6, 6, 6);

    blocksize_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(blocksize_hbox);
    gtk_container_add(GTK_CONTAINER(blocksize_alignment), blocksize_hbox);

    blocksize_label = gtk_label_new_with_mnemonic("_Block Size:");
    gtk_box_pack_start(GTK_BOX(blocksize_hbox), blocksize_label, FALSE, FALSE, 6);
    gtk_widget_show(blocksize_label);
    gtk_label_set_justify(GTK_LABEL(blocksize_label), GTK_JUSTIFY_RIGHT);

    //blocksize_spinbutton = gimp_spin_button_new(&blocksize_spinbutton_adj, vals->blocksize, 2, 16, 1, 1, 0, 5, 0);
    blocksize_combobox = gimp_int_combo_box_new("2",2,"4",4,"8",8,"16",16,NULL);
    gimp_int_combo_box_connect(GIMP_INT_COMBO_BOX(blocksize_combobox), vals->blocksize, G_CALLBACK(callback_change_blocksize), ui_vals);
    gtk_box_pack_start(GTK_BOX(blocksize_hbox), blocksize_combobox, FALSE, FALSE, 0);
    gtk_widget_show(blocksize_combobox);

    blocksize_frame_label = gtk_label_new("<b>Modify block size</b>");
    gtk_widget_show(blocksize_frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(blocksize_frame), blocksize_frame_label);
    gtk_label_set_use_markup(GTK_LABEL(blocksize_frame_label), TRUE);
    
    // blocksize - end


    /* Sliders:
     * */
    sliders_frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(energy_vbox), sliders_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(sliders_frame), 6);
    gtk_widget_show(sliders_frame);

	sliders_alignment = gtk_alignment_new(0.5, 0.5, 1, 0);
    gtk_widget_show(sliders_alignment);
    gtk_container_add(GTK_CONTAINER(sliders_frame), sliders_alignment);
    gtk_alignment_set_padding(GTK_ALIGNMENT(sliders_alignment), 6, 6, 6, 6);

	sliders_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(sliders_hbox);
    gtk_container_add(GTK_CONTAINER(sliders_alignment), sliders_hbox);
    
    edges_label = gtk_label_new_with_mnemonic("Edges");
    gtk_box_pack_start(GTK_BOX(sliders_hbox), edges_label, FALSE, FALSE, 1);
    gtk_widget_show(edges_label);
    gtk_label_set_justify(GTK_LABEL(edges_label), GTK_JUSTIFY_RIGHT);
    
    slider_hscale = gtk_hscale_new_with_range(0, 1, 0.01);
    gtk_range_set_value(GTK_RANGE(slider_hscale), vals->textures);
    gtk_range_set_update_policy(GTK_RANGE(slider_hscale), GTK_UPDATE_CONTINUOUS);
    //gtk_scale_add_mark(GTK_SCALE(slider_hscale), 0, GTK_POS_BOTTOM, "Textures" );
    //gtk_scale_add_mark(GTK_SCALE(slider_hscale), 1, GTK_POS_BOTTOM, "Edges" );
    //gtk_scale_add_mark(GTK_SCALE(slider_hscale), 0.5, GTK_POS_BOTTOM, "No preference" );
    gtk_scale_set_draw_value(GTK_SCALE(slider_hscale), FALSE);
    gtk_widget_show(slider_hscale);
    gtk_box_pack_start(GTK_BOX(sliders_hbox), slider_hscale, TRUE, TRUE, 20);
    
    textures_label = gtk_label_new_with_mnemonic("Textures");
    gtk_box_pack_start(GTK_BOX(sliders_hbox), textures_label, FALSE, FALSE, 1);
    gtk_widget_show(textures_label);
    gtk_label_set_justify(GTK_LABEL(textures_label), GTK_JUSTIFY_RIGHT);

	sliders_frame_label = gtk_label_new("<b>Egde/texture preservation preference</b>");
    gtk_widget_show(sliders_frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(sliders_frame), sliders_frame_label);
    gtk_label_set_use_markup(GTK_LABEL(sliders_frame_label), TRUE);

	//energy parametrs - end
	
	//***************************************************

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), separator, TRUE, TRUE, 0);
	gtk_widget_show(separator);
	
	carver_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_vbox), carver_hbox, TRUE, TRUE, 0);
	gtk_widget_show(carver_hbox);
	
	
	//resize parameters - start

    resize_frame = gtk_frame_new(NULL);
    gtk_widget_show(resize_frame);
    gtk_box_pack_start(GTK_BOX(carver_hbox), resize_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(resize_frame), 6);
	
	resize_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(resize_vbox);
	gtk_container_add(GTK_CONTAINER(resize_frame), resize_vbox);
    
	//seams number - start
    seams_number_alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(seams_number_alignment);
    gtk_box_pack_start(GTK_BOX(resize_vbox), seams_number_alignment, TRUE, TRUE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(seams_number_alignment), 6, 6, 6, 6);

    seams_number_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(seams_number_hbox);
    gtk_container_add(GTK_CONTAINER(seams_number_alignment), seams_number_hbox);

    seams_number_label = gtk_label_new_with_mnemonic("_Seams Number:");
    gtk_widget_show(seams_number_label);
    gtk_box_pack_start(GTK_BOX(seams_number_hbox), seams_number_label, FALSE, FALSE, 6);
    gtk_label_set_justify(GTK_LABEL(seams_number_label), GTK_JUSTIFY_RIGHT);

	width = drawable->width;
	height = drawable->height;
	seams_bound = (vals->vertically) ? height : width;
	
    if (vals->seams_number < (-1*seams_bound + 1)) {
        vals->seams_number = -1*seams_bound + 1;
    } else if (vals->seams_number > (seams_bound - 1)) {
        vals->seams_number = seams_bound - 1;
    }

    seams_number_spinbutton = gimp_spin_button_new(&seams_number_spinbutton_adj, vals->seams_number,
                                      -1*seams_bound + 1, seams_bound - 1, 1, 1, 0, 5, 0);
    gtk_box_pack_start(GTK_BOX(seams_number_hbox), seams_number_spinbutton, FALSE, FALSE, 0);
    gtk_widget_show(seams_number_spinbutton);
    
    gimp_help_set_help_data (seams_number_spinbutton,
			   ("(-) for reduction (+) for enlarging."
			    " the absolute value will be taken "
			    "for the Interacive option"), NULL);
	
	ui_vals->seams_number_spinbutton_adj = seams_number_spinbutton_adj;
	ui_vals->width = width;
	ui_vals->height = height;
	
   	//seams number - end
	
	//direction - start
	
	//direction_alignment = gtk_alignment_new(0.4, 0.5, 1, 1);
    //gtk_widget_show(direction_alignment);
    //gtk_box_pack_start(GTK_BOX(resize_vbox), direction_alignment, TRUE, TRUE, 0);
    //gtk_alignment_set_padding(GTK_ALIGNMENT(direction_alignment), 6, 6, 6, 6);
	//
	//direction_radio_button_vbox = gimp_int_radio_group_new(TRUE, "Carving Direction",
    //                                G_CALLBACK (gimp_radio_button_update),
	//			    				&(vals->direction), vals->direction,
	//			    				"Vertically", 	NULL, NULL,
	//			    				"Horizontally", NULL, NULL,
	//			    				NULL);
	//gtk_widget_show(direction_radio_button_vbox);
	//gtk_container_add(GTK_CONTAINER(direction_alignment), direction_radio_button_vbox);
	
	radio_frame = gtk_frame_new(NULL);
    gtk_widget_show(radio_frame);
    gtk_box_pack_start(GTK_BOX(resize_vbox), radio_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(radio_frame), 6);
	
	radio_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(radio_frame), radio_vbox);
	gtk_widget_show(radio_vbox);
	
	vert = gtk_radio_button_new_with_label(NULL, "Vertically");
	gtk_widget_show(vert);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(vert), vals->vertically);

	horizon = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(vert), "Horizontally");
	gtk_widget_show(horizon);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(horizon), !vals->vertically);
	
	gtk_box_pack_start(GTK_BOX(radio_vbox), vert, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(radio_vbox), horizon, TRUE, TRUE, 0);
	
	
	
	radio_frame_label = gtk_label_new("<b>Carving direction</b>");
    gtk_widget_show(radio_frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(radio_frame), radio_frame_label);
    gtk_label_set_use_markup(GTK_LABEL(radio_frame_label), TRUE);
	
	
	//direction - end

    resize_frame_label = gtk_label_new("<b>Resize parameters</b>");
    gtk_widget_show(resize_frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(resize_frame), resize_frame_label);
    gtk_label_set_use_markup(GTK_LABEL(resize_frame_label), TRUE);
    
    // resize parameters - end


	// output options - start
	
	options_frame = gtk_frame_new(NULL);
    gtk_widget_show(options_frame);
    gtk_box_pack_start(GTK_BOX(carver_hbox), options_frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(options_frame), 6);
	
	options_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_add(GTK_CONTAINER(options_frame), options_vbox);
    gtk_widget_show (options_vbox);

    new_layer_button =
      gtk_check_button_new_with_label (("Output on a new layer"));

   	gtk_box_pack_start (GTK_BOX (options_vbox), new_layer_button, FALSE, FALSE, 0);
   	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (new_layer_button),
				vals->new_layer);
   	gtk_widget_show (new_layer_button);

   	gimp_help_set_help_data (new_layer_button,
			   ("Outputs the resulting image "
			     "on a new layer"), NULL);

   	resize_canvas_button =
     gtk_check_button_new_with_label (("Resize image canvas"));

   	gtk_box_pack_start (GTK_BOX (options_vbox), resize_canvas_button, FALSE, FALSE, 0);
   	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (resize_canvas_button),
				vals->resize_canvas);
   	gtk_widget_show (resize_canvas_button);

    gimp_help_set_help_data (resize_canvas_button,
			   ("Resize and translate the image "
			     "canvas to fit the resized layer"), NULL);
	
	output_energy_button =
     gtk_check_button_new_with_label (("Output Energy"));

   	gtk_box_pack_start (GTK_BOX (options_vbox), output_energy_button, FALSE, FALSE, 0);
   	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (output_energy_button),
				vals->output_energy);
   	gtk_widget_show (output_energy_button);

    gimp_help_set_help_data (output_energy_button,
			   ("Output the energy that was computed "
			     "onto a new image"), NULL);
			     
	output_seams_button =
     gtk_check_button_new_with_label (("Output Seams"));

   	gtk_box_pack_start (GTK_BOX (options_vbox), output_seams_button, FALSE, FALSE, 0);
   	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (output_seams_button),
				vals->output_seams);
   	gtk_widget_show (output_seams_button);

    gimp_help_set_help_data (output_seams_button,
			   ("Output the seams that was computed "
			     "onto the original image"), NULL);
	
	options_frame_label = gtk_label_new("<b>Output options</b>");
    gtk_widget_show(options_frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(options_frame), options_frame_label);
    gtk_label_set_use_markup(GTK_LABEL(options_frame_label), TRUE);
	
	// output options - end


    /* Signals:
     * */
    g_signal_connect_swapped(preview, "invalidated",
                             G_CALLBACK(dct_energy_preview),
                             (gpointer)ui_vals);
                             
    /* g_signal_connect(preview, "invalidated",
					 G_CALLBACK(callback_change_preference),
					 (gpointer)vals); */

	g_signal_connect(slider_hscale, "value_changed",
					 G_CALLBACK(callback_preference_slider),
					 (gpointer)ui_vals);
                     
    g_signal_connect(seams_number_spinbutton_adj, "value_changed",
                     G_CALLBACK(gimp_int_adjustment_update),
                     &(vals->seams_number));
                     
    g_signal_connect (vert, "toggled",
                     G_CALLBACK (callback_toggle_checkbox), &(vals->vertically));
    
    g_signal_connect (vert, "toggled",
                     G_CALLBACK (callback_update_spinbutton_boundries_vert), (gpointer)ui_vals);

    g_signal_connect (horizon, "toggled",
                     G_CALLBACK (callback_update_spinbutton_boundries_horz), (gpointer)ui_vals);
    
                     
    g_signal_connect (new_layer_button, "toggled",
                     G_CALLBACK (callback_toggle_checkbox), &(vals->new_layer));
                     
    g_signal_connect (resize_canvas_button, "toggled",
                     G_CALLBACK (callback_toggle_checkbox), &(vals->resize_canvas)); 
                  
    g_signal_connect (output_energy_button, "toggled",
                     G_CALLBACK (callback_toggle_checkbox), &(vals->output_energy)); 
                     
    g_signal_connect (output_seams_button, "toggled",
                     G_CALLBACK (callback_toggle_checkbox), &(vals->output_seams));         

    gtk_widget_show(dialog);

    response_id = gimp_dialog_run(GIMP_DIALOG(dialog));

    gtk_widget_destroy(dialog);

    return response_id;
}

void
error(const gchar* message) {
	gimp_message(message);
}

void
callback_toggle_checkbox(GtkToggleButton *toggle_button, gpointer data) {
	*((gboolean*)data) = gtk_toggle_button_get_active(toggle_button);
}

void callback_update_spinbutton_boundries_vert(GtkToggleButton *toggle_button, gpointer data) {
	PlugInUIVals* ui_vals = (PlugInUIVals*)data;
	gdouble value;
	GtkAdjustment* adj =GTK_ADJUSTMENT(ui_vals->seams_number_spinbutton_adj);
							  	 
	if (gtk_toggle_button_get_mode(toggle_button)) {
		gtk_adjustment_set_lower (adj, (-1*(ui_vals->height))+1);
		gtk_adjustment_set_upper (adj, (ui_vals->height));
		}
	
	value = gtk_adjustment_get_value(adj);
	if (value < -1*(ui_vals->height) + 1) {
		gtk_adjustment_set_value(adj, -1*(ui_vals->height) + 1);
        ui_vals->vals->seams_number = -1*(ui_vals->height) + 1;
		}
    if (value > (ui_vals->height)){
		gtk_adjustment_set_value(adj, (ui_vals->height));
        ui_vals->vals->seams_number = ui_vals->height;
		}
	
}

void callback_update_spinbutton_boundries_horz(GtkToggleButton *toggle_button, gpointer data) {
	PlugInUIVals* ui_vals = (PlugInUIVals*)data;
	gdouble value;
	GtkAdjustment* adj =GTK_ADJUSTMENT(ui_vals->seams_number_spinbutton_adj);
							  	 
	if (gtk_toggle_button_get_mode(toggle_button)) {
		gtk_adjustment_set_lower (adj, (-1*(ui_vals->width))+1);
		gtk_adjustment_set_upper (adj, (ui_vals->width));
		}
	
	value = gtk_adjustment_get_value(adj);
	if (value < -1*(ui_vals->width)+1) {
		gtk_adjustment_set_value(adj, -1*(ui_vals->width)+1);
        ui_vals->vals->seams_number = -1*(ui_vals->width)+1;
		}
    if (value > (ui_vals->width)){
		gtk_adjustment_set_value(adj, (ui_vals->width));
        ui_vals->vals->seams_number = ui_vals->width;
		}
}


void
callback_change_blocksize(GimpIntComboBox *box, gpointer data) {
	PlugInUIVals* ui_vals = (PlugInUIVals*)data;
	gimp_int_combo_box_get_active(box, &((ui_vals->vals)->blocksize));
	gimp_preview_invalidate(ui_vals->preview);
}

void 
callback_preference_slider(GtkHScale *slider, gpointer data) {
	PlugInUIVals* ui_vals = (PlugInUIVals*)data;
	gdouble slider_val = gtk_range_get_value(GTK_RANGE(slider));
	
	(ui_vals->vals)->textures = slider_val;
	(ui_vals->vals)->edges = 1 - slider_val;
	gimp_preview_invalidate(ui_vals->preview);
}

/* void
callback_change_preference(GimpPreview *gimppreview, gpointer data) {
	PlugInVals* vals = (PlugInVals*)data;
	vals->preview = gimp_preview_get_update(gimppreview);
} */
	
void
callback_resize_slider(GtkHScale *slider, gpointer data) {
    PlugInUIIVals* ui_i_vals = (PlugInUIIVals*) data;
	gdouble slider_val = gtk_range_get_value(GTK_RANGE(slider));
    gint new_width, new_height;
    gint ntiles;

    if (ui_i_vals->vals->vertically) {
        new_width = ui_i_vals->old_width;
        new_height = ui_i_vals->old_height + slider_val;
    } else {
        new_width = ui_i_vals->old_width + slider_val;
        new_height = ui_i_vals->old_height;
    }

    lqr_carver_resize(ui_i_vals->carver, new_width, new_height);
    gimp_image_resize(ui_i_vals->image_ID, new_width, new_height, -ui_i_vals->x_off, -ui_i_vals->y_off);
    gimp_layer_resize_to_image_size(ui_i_vals->layer_ID);

    ntiles = new_width / gimp_tile_width() + 1;
    gimp_tile_cache_size((gimp_tile_width() * gimp_tile_height() * ntiles * 4 * 2) / 1024 + 1);
    write_carver_to_layer(ui_i_vals->carver, ui_i_vals->layer_ID, FALSE);

}
