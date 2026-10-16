/* save to sf3i
 *
 * 16/10/26 Yukari Hafner
 *    - from sf3isave
 */

/*

	This file is part of VIPS.

	VIPS is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
	02110-1301  USA

 */

/*

	These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
#define DEBUG_VERBOSE
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vips/vips.h>
#include <vips/internal.h>

#include "pforeign.h"

#ifdef HAVE_SF3I

#define SF3I_IMPLEMENTATION
#define SF3I_NO_STDIO

typedef struct _VipsForeignSaveSf3i {
	VipsForeignSave parent_object;

	VipsTarget *target;
} VipsForeignSaveSf3i;

typedef VipsForeignSaveClass VipsForeignSaveSf3iClass;

G_DEFINE_ABSTRACT_TYPE(VipsForeignSaveSf3i, vips_foreign_save_sf3i,
	VIPS_TYPE_FOREIGN_SAVE);

static void
vips_foreign_save_sf3i_dispose(GObject *gobject)
{
	VipsForeignSaveSf3i *sf3i = (VipsForeignSaveSf3i *) gobject;

	VIPS_UNREF(sf3i->target);

	G_OBJECT_CLASS(vips_foreign_save_sf3i_parent_class)->dispose(gobject);
}

static int
vips_foreign_save_sf3i_build(VipsObject *object)
{
	VipsObjectClass *class = VIPS_OBJECT_GET_CLASS(object);
	VipsForeignSave *save = (VipsForeignSave *) object;
	VipsForeignSaveSf3i *sf3i = (VipsForeignSaveSf3i *) object;

	VipsImage *memory;
	sf3i_desc desc;
	int size;
	void *encoded;

	if (VIPS_OBJECT_CLASS(vips_foreign_save_sf3i_parent_class)->build(object))
		return -1;

	if (vips_target_write(sf3i->target, encoded, size)) {
		SF3I_FREE(encoded);
		return -1;
	}
	SF3I_FREE(encoded);

	if (vips_target_end(sf3i->target))
		return -1;

	return 0;
}

/* Save a bit of typing.
 */
#define UC VIPS_FORMAT_UCHAR

static VipsBandFormat bandfmt_sf3i[10] = {
	/* Band format:  UC  C   US  S   UI  I   F   X   D   DX */
	/* Promotion: */ UC, UC, UC, UC, UC, UC, UC, UC, UC, UC
};

static void
vips_foreign_save_sf3i_class_init(VipsForeignSaveSf3iClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS(class);
	VipsForeignSaveClass *save_class = (VipsForeignSaveClass *) class;

	gobject_class->dispose = vips_foreign_save_sf3i_dispose;

	object_class->nickname = "sf3isave_base";
	object_class->description = _("save as SF3I");
	object_class->build = vips_foreign_save_sf3i_build;

	operation_class->flags = VIPS_OPERATION_UNTRUSTED;

	save_class->saveable =
		VIPS_FOREIGN_SAVEABLE_RGB | VIPS_FOREIGN_SAVEABLE_ALPHA;
	save_class->format_table = bandfmt_sf3i;
}

static void
vips_foreign_save_sf3i_init(VipsForeignSaveSf3i *sf3i)
{
}

typedef struct _VipsForeignSaveSf3iFile {
	VipsForeignSaveSf3i parent_object;

	char *filename;
} VipsForeignSaveSf3iFile;

typedef VipsForeignSaveSf3iClass VipsForeignSaveSf3iFileClass;

G_DEFINE_TYPE(VipsForeignSaveSf3iFile, vips_foreign_save_sf3i_file,
	vips_foreign_save_sf3i_get_type());

static int
vips_foreign_save_sf3i_file_build(VipsObject *object)
{
	VipsForeignSaveSf3i *sf3i = (VipsForeignSaveSf3i *) object;
	VipsForeignSaveSf3iFile *file = (VipsForeignSaveSf3iFile *) object;

	if (file->filename &&
		!(sf3i->target = vips_target_new_to_file(file->filename)))
		return -1;

	return VIPS_OBJECT_CLASS(vips_foreign_save_sf3i_file_parent_class)
		->build(object);
}

const char *vips__sf3i_suffs[] = { ".sf3i", NULL };

static void
vips_foreign_save_sf3i_file_class_init(VipsForeignSaveSf3iFileClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "sf3isave";
	object_class->description = _("save image to file as SF3I");
	object_class->build = vips_foreign_save_sf3i_file_build;

	foreign_class->suffs = vips__sf3i_suffs;

	VIPS_ARG_STRING(class, "filename", 1,
		_("Filename"),
		_("Filename to save to"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveSf3iFile, filename),
		NULL);
}

static void
vips_foreign_save_sf3i_file_init(VipsForeignSaveSf3iFile *file)
{
}

typedef struct _VipsForeignSaveSf3iBuffer {
	VipsForeignSaveSf3i parent_object;

	/* Save to a buffer.
	 */
	VipsArea *buf;

} VipsForeignSaveSf3iBuffer;

typedef VipsForeignSaveSf3iClass VipsForeignSaveSf3iBufferClass;

G_DEFINE_TYPE(VipsForeignSaveSf3iBuffer, vips_foreign_save_sf3i_buffer,
	vips_foreign_save_sf3i_get_type());

static int
vips_foreign_save_sf3i_buffer_build(VipsObject *object)
{
	VipsForeignSaveSf3i *sf3i = (VipsForeignSaveSf3i *) object;
	VipsForeignSaveSf3iBuffer *buffer =
		(VipsForeignSaveSf3iBuffer *) object;

	VipsBlob *blob;

	if (!(sf3i->target = vips_target_new_to_memory()))
		return -1;

	if (VIPS_OBJECT_CLASS(vips_foreign_save_sf3i_buffer_parent_class)
			->build(object))
		return -1;

	g_object_get(sf3i->target, "blob", &blob, NULL);
	g_object_set(buffer, "buffer", blob, NULL);
	vips_area_unref(VIPS_AREA(blob));

	return 0;
}

static void
vips_foreign_save_sf3i_buffer_class_init(
	VipsForeignSaveSf3iBufferClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "sf3isave_buffer";
	object_class->description = _("save image to buffer as SF3I");
	object_class->build = vips_foreign_save_sf3i_buffer_build;

	foreign_class->suffs = vips__sf3i_suffs;

	VIPS_ARG_BOXED(class, "buffer", 1,
		_("Buffer"),
		_("Buffer to save to"),
		VIPS_ARGUMENT_REQUIRED_OUTPUT,
		G_STRUCT_OFFSET(VipsForeignSaveSf3iBuffer, buf),
		VIPS_TYPE_BLOB);
}

static void
vips_foreign_save_sf3i_buffer_init(VipsForeignSaveSf3iBuffer *buffer)
{
}

typedef struct _VipsForeignSaveSf3iTarget {
	VipsForeignSaveSf3i parent_object;

	VipsTarget *target;
} VipsForeignSaveSf3iTarget;

typedef VipsForeignSaveSf3iClass VipsForeignSaveSf3iTargetClass;

G_DEFINE_TYPE(VipsForeignSaveSf3iTarget, vips_foreign_save_sf3i_target,
	vips_foreign_save_sf3i_get_type());

static int
vips_foreign_save_sf3i_target_build(VipsObject *object)
{
	VipsForeignSaveSf3i *sf3i = (VipsForeignSaveSf3i *) object;
	VipsForeignSaveSf3iTarget *target =
		(VipsForeignSaveSf3iTarget *) object;

	if (target->target) {
		sf3i->target = target->target;
		g_object_ref(sf3i->target);
	}

	return VIPS_OBJECT_CLASS(vips_foreign_save_sf3i_target_parent_class)
		->build(object);
}

static void
vips_foreign_save_sf3i_target_class_init(
	VipsForeignSaveSf3iTargetClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "sf3isave_target";
	object_class->description = _("save image to target as SF3I");
	object_class->build = vips_foreign_save_sf3i_target_build;

	foreign_class->suffs = vips__sf3i_suffs;

	VIPS_ARG_OBJECT(class, "target", 1,
		_("Target"),
		_("Target to save to"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsForeignSaveSf3iTarget, target),
		VIPS_TYPE_TARGET);
}

static void
vips_foreign_save_sf3i_target_init(VipsForeignSaveSf3iTarget *target)
{
}

#endif /*HAVE_SF3I*/

/**
 * vips_sf3isave: (method)
 * @in: image to save
 * @filename: file to write to
 * @...: `NULL`-terminated list of optional named arguments
 *
 * Write to a file as SF3I. Images are saved as 8-bit RGB or RGBA.
 *
 * ::: seealso
 *   [ctor@Image.sf3iload], [method@Image.write_to_file].
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_sf3isave(VipsImage *in, const char *filename, ...)
{
	va_list ap;
	int result;

	va_start(ap, filename);
	result = vips_call_split("sf3isave", ap, in, filename);
	va_end(ap);

	return result;
}

/**
 * vips_sf3isave_buffer: (method)
 * @in: image to save
 * @buf: (array length=len) (element-type guint8): return output buffer here
 * @len: (type gsize): return output length here
 * @...: `NULL`-terminated list of optional named arguments
 *
 * As [method@Image.sf3isave], but save to a memory buffer.
 *
 * The address of the buffer is returned in @buf, the length of the buffer in
 * @len. You are responsible for freeing the buffer with [func@GLib.free]
 * when you are done with it.
 *
 * ::: seealso
 *     [method@Image.sf3isave].
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_sf3isave_buffer(VipsImage *in, void **buf, size_t *len, ...)
{
	va_list ap;
	VipsArea *area;
	int result;

	area = NULL;

	va_start(ap, len);
	result = vips_call_split("sf3isave_buffer", ap, in, &area);
	va_end(ap);

	if (!result &&
		area) {
		if (buf) {
			*buf = area->data;
			area->free_fn = NULL;
		}
		if (len)
			*len = area->length;

		vips_area_unref(area);
	}

	return result;
}

/**
 * vips_sf3isave_target: (method)
 * @in: image to save
 * @target: save image to this target
 * @...: `NULL`-terminated list of optional named arguments
 *
 * As [method@Image.sf3isave], but save to a target.
 *
 * ::: seealso
 *     [method@Image.sf3isave].
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_sf3isave_target(VipsImage *in, VipsTarget *target, ...)
{
	va_list ap;
	int result;

	va_start(ap, target);
	result = vips_call_split("sf3isave_target", ap, in, target);
	va_end(ap);

	return result;
}
