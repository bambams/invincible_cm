#include <allegro5/allegro.h>

#include <stdio.h>
#include <math.h>

#include "m_config.h"


#include "g_header.h"

#include "c_header.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_editor.h"
#include "e_log.h"

#include "g_world.h"
#include "g_misc.h"
#include "g_proc.h"
#include "g_packet.h"
#include "g_motion.h"
#include "g_client.h"
#include "m_globvars.h"
#include "m_maths.h"
#include "t_template.h"

/*

This file contains code for running complex methods of extended type.

Ideally it should contain only functions called from itself or from g_method.c

*/

