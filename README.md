# Understanding and experimenting with GTK and GObject

These are the experiments and small pieces of code I wrote to understand how GTK, GObject and especially the introspection works.

## `with_invoker.c`

This example shows how to create a window with GTK by using the GIR repository. It loads all the required `typelib` describing Gtk, Glib, GObject and Gio.
It then makes a window and connect a signal using a GClosure. I use `g_signal_connect_closure` because this is the only function available from introspection (and suitable for this example).
The regular `g_signal_connect` is a macro in C and not introspectable.

This example also shows how to build a simple GClosure and a marshall function.

Compile the example with
```bash
cc with_invoker.c $(pkg-config --cflags --libs glib-2.0 gobject-introspection-1.0 gobject-2.0 gtk4) && ./a.out 
```

Sample output
```
Calling marshal function with 1 parameters
GClosure is 0x561dc9d7bfd0, return_value is 0x7ffe7a264600
Doing a callback now....
```

