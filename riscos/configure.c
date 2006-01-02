/*
 * This file is part of NetSurf, http://netsurf.sourceforge.net/
 * Licensed under the GNU General Public License,
 *                http://www.opensource.org/licenses/gpl-license
 * Copyright 2005 Richard Wilson <info@tinct.net>
 */

/** \file
 * RISC OS option setting (implemenation).
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "oslib/os.h"
#include "oslib/wimp.h"
#include "netsurf/riscos/dialog.h"
#include "netsurf/riscos/configure.h"
#include "netsurf/riscos/wimp_event.h"
#include "netsurf/riscos/configure/configure.h"
#include "netsurf/utils/log.h"
#include "netsurf/utils/utils.h"
#include "netsurf/utils/messages.h"

#define CONFIGURE_ICON_PADDING_H 32
#define CONFIGURE_ICON_PADDING_V 32

struct configure_tool {
	const char *name;
	const char *translated;
	char *validation;
	bool (*initialise)(wimp_w w);
	void (*finalise)(wimp_w w);
	wimp_w w;
	wimp_i i;
	bool open;
	struct configure_tool *next;
};

static wimp_w configure_window;
static int configure_icons = 0;
static struct configure_tool *configure_tools = NULL;
static int configure_icon_width = 68 + CONFIGURE_ICON_PADDING_H;
static int configure_icon_height = 112 + CONFIGURE_ICON_PADDING_V;
static int configure_icons_per_line = 0;
static int configure_width;
static int configure_height;

static bool ro_gui_configure_click(wimp_pointer *pointer);
static void ro_gui_configure_open_window(wimp_open *open);
static void ro_gui_configure_close(wimp_w w);

void ro_gui_configure_initialise(void) {
	/* create our window */
	configure_window = ro_gui_dialog_create("configure");
	ro_gui_wimp_event_register_open_window(configure_window,
			ro_gui_configure_open_window);
	ro_gui_wimp_event_register_mouse_click(configure_window,
			ro_gui_configure_click);
	ro_gui_wimp_event_set_help_prefix(dialog_zoom, "HelpConfigure");
	
	/* add in our option windows */
	ro_gui_configure_register("con_fonts",
			ro_gui_options_fonts_initialise,
			ro_gui_wimp_event_finalise);
	ro_gui_configure_register("con_memory",
			ro_gui_options_memory_initialise,
			ro_gui_wimp_event_finalise);
	ro_gui_configure_register("con_image",
			ro_gui_options_image_initialise,
			ro_gui_options_image_finalise);
}

void ro_gui_configure_show(void) {
  	int width, height;

	width = configure_icon_width * 5;
	height = (configure_icons % 5) * configure_icon_height;
	ro_gui_dialog_open_top(configure_window, NULL, width, height);
}

bool ro_gui_configure_click(wimp_pointer *pointer) {
  	struct configure_tool *tool;
  	
  	if (pointer->buttons == wimp_CLICK_MENU)
  		return true;
  	
  	for (tool = configure_tools; tool; tool = tool->next) {
  		if (tool->i == pointer->i) {
  			if (!tool->open) {
  			  	tool->open = true;
  				if (!tool->initialise(tool->w))
  					return false;
  				ro_gui_dialog_open_persistent(
  						configure_window,
  						tool->w, true);
  				ro_gui_wimp_event_register_close_window(
  						tool->w,
  						ro_gui_configure_close);
  				
  			} else {
				ro_gui_dialog_open_top(tool->w, NULL, 0, 0);
			}
  			break;
  		}
  	}
	return true;
}

void ro_gui_configure_close(wimp_w w) {
  	struct configure_tool *tool;
  	
  	for (tool = configure_tools; tool; tool = tool->next) {
  		if (tool->w == w) {
  			tool->open = false;
  			if (tool->finalise)
  				tool->finalise(w);
  			break;
  		}
  	}
}

void ro_gui_configure_open_window(wimp_open *open) {
	os_error *error;
	int screen_width, screen_height;
	int height, width;
	int icons_per_line, icon_lines;
	int max_icons_per_line, max_height;
	os_box extent = { 0, 0, 0, 0 };
	int x, y, l;
	struct configure_tool *tool;

	width = open->visible.x1 - open->visible.x0;
	height = open->visible.y1 - open->visible.y0;
	icons_per_line = width / configure_icon_width;
	if (icons_per_line < 1)
		icons_per_line = 1;

	/* move our icons */ 
	if (icons_per_line != configure_icons_per_line) {
		configure_icons_per_line = icons_per_line;
		x = CONFIGURE_ICON_PADDING_H / 2;
		y = -configure_icon_height + (CONFIGURE_ICON_PADDING_V / 2);
		l = 0;
		for (tool = configure_tools; tool; tool = tool->next) {
			error = xwimp_resize_icon(configure_window,
				tool->i,
				x,
				y,
				x + configure_icon_width -
						CONFIGURE_ICON_PADDING_H,
				y + configure_icon_height -
						CONFIGURE_ICON_PADDING_V);
			if (error) {
				LOG(("xwimp_resize_icon: 0x%x: %s",
						error->errnum, error->errmess));
			}
			x += configure_icon_width;
			l++;
			if (l >= icons_per_line) {
				x = CONFIGURE_ICON_PADDING_H / 2;
				l = 0;
				y -= configure_icon_height; 
                        } 
		}
		error = xwimp_force_redraw(configure_window,
				0, -16384, 16384, 0);
		if (error) {
			LOG(("xwimp_force_redraw: 0x%x: %s",
					error->errnum, error->errmess));
			warn_user("WimpError", error->errmess);
		}
	}
	
	/* restrict our height */
	icon_lines = (configure_icons + icons_per_line - 1) /
			icons_per_line;
	max_height = (icon_lines * configure_icon_height);
	if (height > max_height)
		open->visible.y0 = open->visible.y1 - max_height;
	
	/* set the extent */
	if ((configure_height != height) || (configure_width != width)) {
		ro_gui_screen_size(&screen_width, &screen_height);
		max_icons_per_line = screen_width / configure_icon_width;
		if (max_icons_per_line > configure_icons)
			max_icons_per_line = configure_icons;
		extent.x1 = configure_icon_width * max_icons_per_line;
		extent.y0 = -max_height;
		error = xwimp_set_extent(open->w, &extent);
		if (error) {
			LOG(("xwimp_set_extent: 0x%x: %s",
					error->errnum, error->errmess));
			warn_user("WimpError", error->errmess);
			return;
		}
		configure_height = height;
		configure_width = width;
	}

	/* open the window */
	error = xwimp_open_window(open);
	if (error) {
		LOG(("xwimp_open_window: 0x%x: %s",
				error->errnum, error->errmess));
		warn_user("WimpError", error->errmess);
		return;
	}
}

void ro_gui_configure_register(const char *window,
		bool (*initialise)(wimp_w w), void (*finalise)(wimp_w w)) {
	wimp_icon_create new_icon;
	struct configure_tool *tool;
	struct configure_tool *link;
	int icon_width;
	os_error *error;

	/* create our tool */
	tool = calloc(sizeof(struct configure_tool), 1);
	if (!tool) {
	  	LOG(("Insufficient memory for calloc()"));
		die("Insufficient memory");
	}
	tool->name = window;
	tool->translated = messages_get(window);
	tool->validation = malloc(strlen(window) + 1);
	if (!tool->validation) {
	  	LOG(("Insufficient memory for malloc()"));
		die("Insufficient memory");
	}
	sprintf(tool->validation, "S%s", window);
	tool->initialise = initialise;
	tool->finalise = finalise;
	tool->w = ro_gui_dialog_create(tool->name);
	
	/* update the width */
	error = xwimptextop_string_width(tool->translated,
			strlen(tool->translated), &icon_width);
	if (error) {
		LOG(("xwimptextop_string_width: 0x%x: %s",
				error->errnum, error->errmess));
		die(error->errmess);
	}
	icon_width += CONFIGURE_ICON_PADDING_H;
	if (icon_width > configure_icon_width)
		configure_icon_width = icon_width;

	/* create the icon */
	new_icon.w = configure_window;
	new_icon.icon.extent.x0 = 0;
	new_icon.icon.extent.x1 = configure_icon_width;
	new_icon.icon.extent.y1 = 0;
	new_icon.icon.extent.y0 = -configure_icon_height;
	new_icon.icon.flags = wimp_ICON_TEXT | wimp_ICON_SPRITE |
			wimp_ICON_INDIRECTED | wimp_ICON_HCENTRED |
			(wimp_COLOUR_VERY_LIGHT_GREY << wimp_ICON_BG_COLOUR_SHIFT) |
			(wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
			(wimp_BUTTON_CLICK << wimp_ICON_BUTTON_TYPE_SHIFT);
	new_icon.icon.data.indirected_text_and_sprite.text =
			tool->translated;
	new_icon.icon.data.indirected_text_and_sprite.validation =
			tool->validation;
	new_icon.icon.data.indirected_text_and_sprite.size =
			strlen(tool->translated);
	error = xwimp_create_icon(&new_icon, &tool->i);
	if (error) {
		LOG(("xwimp_create_icon: 0x%x: %s",
				error->errnum, error->errmess));
		die(error->errmess);
	}
	
	/* link into our list alphabetically */
	if ((!configure_tools) ||
			(strcmp(configure_tools->translated,
					tool->translated) > 0)) {
		tool->next = configure_tools;
		configure_tools = tool;
	} else {
		for (link = configure_tools; link; link = link->next) {
			if (link->next) {
				if (strcmp(link->next->translated,
						tool->translated) > 0) {
					tool->next = link->next;
					link->next = tool;
					break;
				}
			} else {
				link->next = tool;
				break;
			}
		}
	}
	configure_icons++;
}
