#
# Update tk.tcl to autoload from our library rather than explictly source
# the files.  We pick an entry point in the file to force it to be brought
# in.
#
s@^source \$tk_library/button.tcl@auto_load tkButtonEnter@
s@^source \$tk_library/entry.tcl@auto_load tkEntryButton1@
s@^source \$tk_library/listbox.tcl@auto_load tkListboxMotion@
s@^source \$tk_library/menu.tcl@auto_load tkMbEnter@
s@^source \$tk_library/scale.tcl@auto_load tkScaleActivate@
s@^source \$tk_library/scrollbar.tcl@auto_load tkScrollSelect@
s@^source \$tk_library/text.tcl@auto_load tkTextButton1@
