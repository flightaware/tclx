#
# fmath.tcl --
#
#   Contains a package of procs that interface to the Tcl expr command built-in
# functions.  These procs provide compatibility with older versions of TclX and
# are also generally useful.
#------------------------------------------------------------------------------
# Copyright 1993 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: parray.tcl,v 1.2 1993/04/07 02:42:32 markd Exp $
#------------------------------------------------------------------------------

#@package: TclX-fmath acos asin atan ceil cos cosh exp fabs floor log log10 \
           sin sinh sqrt tan tanh fmod pow atan2 abs double int round

proc acos  x {expr acos($x)}
proc asin  x {expr asin($x)}
proc atan  x {expr atan($x)}
proc ceil  x {expr ceil($x)}
proc cos   x {expr cos($x)}
proc cosh  x {expr cosh($x)}
proc exp   x {expr exp($x)}
proc fabs  x {expr abs($x)}
proc floor x {expr floor($x)}
proc log   x {expr log($x)}
proc log10 x {expr log10($x)}
proc sin   x {expr sin($x)}
proc sinh  x {expr sinh($x)}
proc sqrt  x {expr sqrt($x)}
proc tan   x {expr tan($x)}
proc tanh  x {expr tanh($x)}

proc fmod {x n} {expr fmod($x,$n)}
proc pow {x n} {expr pow($x,$n)}

# New functions that TclX did not provide in eariler versions.

proc atan2  x {expr atan2($x)}
proc abs    x {expr abs($x)}
proc double x {expr double($x)}
proc int    x {expr int($x)}
proc round  x {expr round($x)}

