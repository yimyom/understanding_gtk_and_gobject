#include <girepository.h>
#include <glib.h>
#include <gio/gio.h> 
#include <gtk/gtk.h>
#include <stdio.h>

// https://docs.gtk.org/gobject/struct.Closure.html
// marshal function used by GClosure
// In theory this function should marshall the param_values to the types of
// the target language, then do the call the the user supplied function (the real callback)
// then unmarshall the result back to GValue
void sheriff(GClosure* closure, GValue* return_value,
			 guint n_param_values, const GValue* param_values,
			 gpointer invocation_hint, gpointer marshal_data)
{
	g_print("Calling marshal function with %d parameters\n", n_param_values);
	g_print("GClosure is %p, return_value is %p\n", closure, return_value);

	// From here we should do the callback
	// I replaced it with a dummy code for the sake of simplicity
	g_print("Doing a callback now....\n");
}

int main(int argc, char** argv)
{
	GError * error=NULL;
	
	// Load the namespace defined in 'lib'
	GITypelib* tl = g_irepository_require(NULL, // NULL for the global repository
			"Gtk", // name of the library to load
			NULL, // NULL = last version of the library
			G_IREPOSITORY_LOAD_FLAG_LAZY, // mandatory and only value available so far
			&error); // NULL or a pointer to a GError object

	// Also load other libraries
	g_irepository_require(NULL,"GLib",NULL,G_IREPOSITORY_LOAD_FLAG_LAZY,NULL);
	g_irepository_require(NULL,"GObject",NULL,G_IREPOSITORY_LOAD_FLAG_LAZY,NULL);
	g_irepository_require(NULL,"Gio",NULL,G_IREPOSITORY_LOAD_FLAG_LAZY,NULL);

	// Find the gtk_init function by its name
	GIFunctionInfo *fct_info = (GIFunctionInfo*) g_irepository_find_by_name(NULL,"Gtk","init");
	GIArgument retval; // used to retrieve return values from any function
	
	// call the gtk_init function
	if(!g_function_info_invoke(
		fct_info, // The GIFunctionInfo structure pointer
		NULL, 0, NULL, 0,
		&retval, // used to store the return value
		&error)) // to catch an error
	{
		g_print("Error initializing gtk library: %s\n",error->message);
		return 1;
	}

	// Free gtk_init as we don't need it anymore
	// and the inparam array too
	if(error)
		g_error_free(error); // After using a GError, free it
	g_base_info_unref(fct_info);
	
	// -------------------
	// Create a Gtk Window
	// -------------------
	
	// Search for GIR info on the class Gtk.Window
	GIObjectInfo* wininfo = (GIObjectInfo *) g_irepository_find_by_name(NULL,"Gtk","Window");
	// Get the GType of Gtk.Window so libgir can find its registered type and instantiate it
	GType wintype = g_registered_type_info_get_g_type(wininfo);
	
	// instantiate Gtk.Window
	GObject * gtk_window = G_OBJECT(g_object_new(wintype,"visible",TRUE,NULL));
	g_base_info_unref(wininfo);

	// -------------------------------------
	// Call a method from an existing object
	// -------------------------------------

	// Now we search for the class Gtk.Widget, from which Gtk.Window inherits (too)
	GIObjectInfo* widget = (GIObjectInfo *) g_irepository_find_by_name(NULL,"Gtk","Widget");
	fct_info = g_object_info_find_method(widget,"show"); // we then look for a method of this object
	GIArgument* inparam = (GIArgument *) g_new(GIArgument,1); // we allocate to pass one parameter
	inparam[0].v_pointer = (gpointer) gtk_window;

	if(!g_function_info_invoke(fct_info,
		(const GIArgument *) inparam, 1, // IN parameters 
		NULL, 0, // OUT parameters
		&retval, // to store the return value
		&error)) // to catch an error
	{
		g_print("Error calling Gtk.Widget.show_all(gtk_window): %s\n",error->message);
		return 1;
	}
	g_base_info_unref(widget);
	g_base_info_unref(fct_info);
	g_free(inparam);

	// -------------------------------------------
	// Connect a delete-event signal to our window
	// -------------------------------------------
	// we want to implement the following line using introspection:
	// g_signal_connect(win,"delete-event",G_CALLBACK(some_function),NULL);
	
	fct_info = (GIFunctionInfo *) g_irepository_find_by_name(NULL,"GObject","signal_connect_closure");
	// I use a default GClosure and set its marshal function where all the work is done
	GClosure* closure = g_closure_new_simple(sizeof(GClosure), NULL); 
	g_closure_set_marshal(closure, sheriff);

	inparam = (GIArgument *) g_new(GIArgument,4);
	inparam[0].v_pointer = (gpointer) gtk_window; // the window
	inparam[1].v_string  = (gchar *) "close-request"; // the signal
	inparam[2].v_pointer = (gpointer) closure; // the closure
	inparam[3].v_boolean = (gboolean) 1; // true to call after the default event handler

	if(!g_function_info_invoke(fct_info,(const GIArgument *) inparam, 4, NULL, 0, &retval, &error))
	{
		g_print("Error: %s\n",error->message);
		return 1;
	}

	// -----------------------
	// Start the GTK main loop
	// -----------------------

	// No introspection here to keep the code simple
	while (g_list_model_get_n_items (gtk_window_get_toplevels ()) > 0)
		g_main_context_iteration (NULL, TRUE);
}
