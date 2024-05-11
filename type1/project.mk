# #############################################################################
# Project Customization
# #############################################################################

PROJECT = delfx_low_pass_type1

UCSRC = $(wildcard ../../user/lib/*.c)

# NOTE: Relative path from `Makefile` that refer this file.
UCXXSRC = ../../user/unit.cpp

# NOTE: Relative path from `Makefile` that refer this file.
UINCDIR = ../../user/lib

UDEFS = -DFILTER_TYPE=1

ULIB =

ULIBDIR =
