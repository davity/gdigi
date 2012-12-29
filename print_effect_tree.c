/*
 *  Copyright (c) 2009-2011 Tomasz Moń <desowin@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "gdigi.h"
#include "gui.h"
#include "effects.h"
#include "preset.h"
#include "gtkknob.h"
#include "images/gdigi_icon.h"
#include "gdigi_xml.h"

void print_effect_tree(Device *device)
{
    gint i, j, l, m, n;
    EffectPage *effect_page;
    EffectList *effects_list;
    Effect *effect;
    EffectGroup *group;
    EffectSettings *settings;

    printf("Device Name: %s\n", device->name);

    for (i = 0; i < device->n_pages; i++) {
        effect_page = device->pages + i;
        printf("Effect Page: %s\n", effect_page->name);

        for (j = 0; j < effect_page->n_effects; j++) {
            effects_list = device->pages[i].effects + j;
            printf("    Effect List: %s\n", effects_list->label);

            for(l = 0; l < effects_list->amt; l++) {
                effect = effects_list->effect + l;
                printf("        Effect: %d %d %s\n", effect->position, effect->type, effect->label);
                if (effect->id != -1) {
                    printf("            On/Off: %d %d\n", effect->position, effect->id);
                }

                for (m = 0; m < effect->group_amt; m++) {
                    group = effect->group + m;
                    printf("            Group: %d %s\n", group->type, group->label);

                    for (n = 0; n < group->settings_amt; n++) {
                        settings = group->settings + n;
                        printf("                Settings: %d %d %s ", settings->position, settings->id, settings->label);

                        printf("min %d max %d\n", (int)settings->values->min, (int)settings->values->max);
                    }
                }
            }
        }
    }
}

