#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <gdigi_api.h>

#include "const-c.inc"

MODULE = Gdigi		PACKAGE = Gdigi		

INCLUDE: const-xs.inc

void
gdigi_clear_debug()

void
gdigi_fini()

gint
gdigi_get_parameter(id, position, value)
	guint	id
	guint	position
	guint  &value
    OUTPUT:
        value

gint
gdigi_init()

void
gdigi_set_debug()

gint
gdigi_set_parameter(id, position, value)
	guint	id
	guint	position
	guint	value
