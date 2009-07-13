
#ifndef __INTERFACE_H__
#define __INTERFACE_H__


/*  Public functions  */

gint
gui_dialog(PlugInVals *vals, PlugInImageVals *image_vals, 
           PlugInDrawableVals *drawable_vals, PlugInUIVals *ui_vals);

gint
gui_interactive_dialog(PlugInVals *vals, PlugInImageVals *image_vals,
                       PlugInDrawableVals *drawable_vals);

void error(const gchar* message);

#endif /* __INTERFACE_H__ */
