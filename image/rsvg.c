/*
 * Copyright 2007 Rob Kendrick <rjek@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
 * Content handler for image/svg using librsvg (implementation).
 *
 * SVG files are rendered to a NetSurf bitmap by creating a Cairo rendering
 * surface (content_rsvg_data.cs) over the bitmap's data, creating a Cairo
 * drawing context using that surface, and then passing that drawing context
 * to librsvg which then uses Cairo calls to plot the graphic to the bitmap.
 * We store this in content->bitmap, and then use the usual bitmap plotter
 * function to render it for redraw requests.
 */

#include "utils/config.h"
#ifdef WITH_RSVG

#include <stdbool.h>
#include <assert.h>
#include <sys/types.h>

#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>

#include "image/rsvg.h"
#include "content/content_protected.h"
#include "desktop/plotters.h"
#include "image/bitmap.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "utils/messages.h"
#include "utils/talloc.h"

static inline void rsvg_argb_to_abgr(uint8_t *pixels, 
		int width, int height, size_t rowstride);

bool rsvg_create(struct content *c, const struct http_parameter *params)
{
	struct content_rsvg_data *d = &c->data.rsvg;
	union content_msg_data msg_data;

	d->rsvgh = NULL;
	d->cs = NULL;
	d->ct = NULL;
	d->bitmap = NULL;

	if ((d->rsvgh = rsvg_handle_new()) == NULL) {
		LOG(("rsvg_handle_new() returned NULL."));
		msg_data.error = messages_get("NoMemory");
		content_broadcast(c, CONTENT_MSG_ERROR, msg_data);
		return false;
	}

	return true;
}

bool rsvg_process_data(struct content *c, const char *data,
			unsigned int size)
{
	struct content_rsvg_data *d = &c->data.rsvg;
	union content_msg_data msg_data;
	GError *err = NULL;

	if (rsvg_handle_write(d->rsvgh, (const guchar *)data, (gsize)size,
				&err) == FALSE) {
		LOG(("rsvg_handle_write returned an error: %s", err->message));
		msg_data.error = err->message;
		content_broadcast(c, CONTENT_MSG_ERROR, msg_data);
		return false;
	}

	return true;
}

/** Convert Cairo's ARGB output to NetSurf's favoured ABGR format.  It converts
 * the data in-place. 
 *
 * \param pixels	Pixel data, in the form of ARGB.  This will
 *			be overwritten with new data in the form of ABGR.
 * \param width		Width of the bitmap
 * \param height	Height of the bitmap
 * \param rowstride	Number of bytes to skip after each row (this
 *			implementation requires this to be a multiple of 4.)
 */
static inline void rsvg_argb_to_abgr(uint8_t *pixels, 
		int width, int height, size_t rowstride)
{
	uint8_t *p = pixels;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			/* Swap R and B */
			const uint8_t r = p[x+3];

			p[x+3] = p[x];

			p[x] = r;
		}

		p += rowstride;
	}
}

bool rsvg_convert(struct content *c)
{
	struct content_rsvg_data *d = &c->data.rsvg;
	union content_msg_data msg_data;
	RsvgDimensionData rsvgsize;
	GError *err = NULL;

	if (rsvg_handle_close(d->rsvgh, &err) == FALSE) {
		LOG(("rsvg_handle_close returned an error: %s", err->message));
		msg_data.error = err->message;
		content_broadcast(c, CONTENT_MSG_ERROR, msg_data);
		return false;
	}

	assert(err == NULL);

	/* we should now be able to query librsvg for the natural size of the
	 * graphic, so we can create our bitmap.
	 */

	rsvg_handle_get_dimensions(d->rsvgh, &rsvgsize);
	c->width = rsvgsize.width;
	c->height = rsvgsize.height;

	if ((d->bitmap = bitmap_create(c->width, c->height,
			BITMAP_NEW)) == NULL) {
		LOG(("Failed to create bitmap for rsvg render."));
		msg_data.error = messages_get("NoMemory");
		content_broadcast(c, CONTENT_MSG_ERROR, msg_data);
		return false;
	}

	if ((d->cs = cairo_image_surface_create_for_data(
			(unsigned char *)bitmap_get_buffer(d->bitmap),
			CAIRO_FORMAT_ARGB32,
			c->width, c->height,
			bitmap_get_rowstride(d->bitmap))) == NULL) {
		LOG(("Failed to create Cairo image surface for rsvg render."));
		msg_data.error = messages_get("NoMemory");
		content_broadcast(c, CONTENT_MSG_ERROR, msg_data);
		return false;
	}

	if ((d->ct = cairo_create(d->cs)) == NULL) {
		LOG(("Failed to create Cairo drawing context for rsvg render."));
		msg_data.error = messages_get("NoMemory");
		content_broadcast(c, CONTENT_MSG_ERROR, msg_data);
		return false;
	}

	rsvg_handle_render_cairo(d->rsvgh, d->ct);
	rsvg_argb_to_abgr(bitmap_get_buffer(d->bitmap),
				c->width, c->height,
				bitmap_get_rowstride(d->bitmap));

	c->bitmap = d->bitmap;
	bitmap_modified(c->bitmap);
	c->status = CONTENT_STATUS_DONE;
	/* Done: update status bar */
	content_set_status(c, "");

	return true;
}

bool rsvg_redraw(struct content *c, int x, int y,
		int width, int height, struct rect *clip,
		float scale, colour background_colour)
{
	plot.bitmap(x, y, width, height, c->bitmap, background_colour, BITMAPF_NONE);
	return true;
}

void rsvg_destroy(struct content *c)
{
	struct content_rsvg_data *d = &c->data.rsvg;

	if (d->bitmap != NULL) bitmap_destroy(d->bitmap);
	if (d->rsvgh != NULL) rsvg_handle_free(d->rsvgh);
	if (d->ct != NULL) cairo_destroy(d->ct);
	if (d->cs != NULL) cairo_surface_destroy(d->cs);

	return;
}

bool rsvg_clone(const struct content *old, struct content *new_content)
{
	const char *data;
	unsigned long size;

	/* Simply replay create/process/convert */
	if (rsvg_create(new_content, NULL) == false)
		return false;

	data = content__get_source_data(new_content, &size);
	if (size > 0) {
		if (rsvg_process_data(new_content, data, size) == false)
			return false;
	}

	if (old->status == CONTENT_STATUS_READY ||
			old->status == CONTENT_STATUS_DONE) {
		if (rsvg_convert(new_content) == false)
			return false;
	}

	return true;
}

#endif /* WITH_RSVG */
