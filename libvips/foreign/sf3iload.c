/* save sf3 images
 *
 * 16/10/26 Yukari Hafner
 * 	- from sf3isave.c
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
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <glib/gi18n-lib.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <vips/vips.h>
#include <vips/internal.h>

#include "pforeign.h"

#ifdef HAVE_SF3I

typedef struct _VipsForeignLoadSf3i {
	VipsForeignLoad parent_object;

	VipsSource *source;

	/* Properties of this sf3i, from the header.
	 */
	int width;
	int height;
	int bands;

} VipsForeignLoadSf3i;

static const char *sf3i_magic = "sf3if";

typedef VipsForeignLoadClass VipsForeignLoadSf3iClass;

G_DEFINE_ABSTRACT_TYPE(VipsForeignLoadSf3i, vips_foreign_load_sf3i,
	VIPS_TYPE_FOREIGN_LOAD);

static gboolean
vips_foreign_load_sf3i_is_a_source(VipsSource *source)
{
	const unsigned char *data;

	return (data = vips_source_sniff(source, 4)) &&
		memcmp(data, sf3i_magic, 4) == 0;
}

static void
vips_foreign_load_sf3i_dispose(GObject *gobject)
{
	VipsForeignLoadSf3i *sf3i = (VipsForeignLoadSf3i *) gobject;

#ifdef DEBUG
	printf("vips_foreign_load_sf3i_dispose: %p\n", sf3i);
#endif /*DEBUG*/

	VIPS_UNREF(sf3i->source);

	G_OBJECT_CLASS(vips_foreign_load_sf3i_parent_class)->dispose(gobject);
}

/* Scan the header into our class.
 */
static int
vips_foreign_load_sf3i_parse_header(VipsForeignLoadSf3i *sf3i)
{
	VipsObjectClass *class = VIPS_OBJECT_GET_CLASS(sf3i);
	unsigned char header[14];

	if (vips_source_read(sf3i->source, header, 14) != 14)
		return -1;

	/* Check magic bytes.
	 */
	if (memcmp(header, sf3i_magic, 4) != 0) {
		vips_error(class->nickname, _("bad SF3I magic"));
		return -1;
	}

	/* Validate header values.
	 */
	if (sf3i->width <= 0 ||
		sf3i->width >= VIPS_MAX_COORD ||
		sf3i->height <= 0 ||
		sf3i->height >= VIPS_MAX_COORD) {
		vips_error(class->nickname,
			_("bad SF3I dimensions"));
		return -1;
	}

	if (sf3i->bands != 3 && sf3i->bands != 4) {
		vips_error(class->nickname,
			_("bad SF3I channels"));
		return -1;
	}

	return 0;
}

static VipsForeignFlags
vips_foreign_load_sf3i_get_flags(VipsForeignLoad *load)
{
	return 0;
}

static int
vips_foreign_load_sf3i_header(VipsForeignLoad *load)
{
	VipsForeignLoadSf3i *sf3i = (VipsForeignLoadSf3i *) load;

	if (vips_foreign_load_sf3i_parse_header(sf3i))
		return -1;

	vips_image_init_fields(load->out,
		sf3i->width, sf3i->height, sf3i->bands, VIPS_FORMAT_UCHAR,
		VIPS_CODING_NONE, VIPS_INTERPRETATION_sRGB, 1.0, 1.0);

	return 0;
}

static int
vips_foreign_load_sf3i_load(VipsForeignLoad *load)
{
	VipsObjectClass *class = VIPS_OBJECT_GET_CLASS(load);
	VipsForeignLoadSf3i *sf3i = (VipsForeignLoadSf3i *) load;

	VipsImage **t = (VipsImage **)
		vips_object_local_array((VipsObject *) load, 1);

	size_t length;
	const void *data = vips_source_map(sf3i->source, &length);
	if (!data)
		return -1;

	sf3i_desc desc;
	g_signal_connect(load->real, "close", G_CALLBACK(vips_image_free_buffer), decoded_data);

	t[0] = vips_image_new_from_memory(
		decoded_data,
		desc.width * desc.height * desc.channels,
		desc.width, desc.height,
		desc.channels,
		VIPS_FORMAT_UCHAR);

	if (!t[0])
		return -1;

	vips_image_init_fields(t[0],
		desc.width, desc.height,
		desc.channels,
		VIPS_FORMAT_UCHAR,
		VIPS_CODING_NONE,
		VIPS_INTERPRETATION_sRGB,
		1.0, 1.0);

	return vips_image_write(t[0], load->real);
}

static void
vips_foreign_load_sf3i_class_init(VipsForeignLoadSf3iClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS(class);
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->dispose = vips_foreign_load_sf3i_dispose;
	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "sf3iload_base";
	object_class->description = _("load sf3i base class");

	/* You're unlikely to want to use this on untrusted files.
	 */
	operation_class->flags |= VIPS_OPERATION_UNTRUSTED;

	foreign_class->suffs = vips__sf3i_suffs;

	/* We are fast at is_a(), so high priority.
	 */
	foreign_class->priority = 200;

	load_class->get_flags = vips_foreign_load_sf3i_get_flags;
	load_class->header = vips_foreign_load_sf3i_header;
	load_class->load = vips_foreign_load_sf3i_load;
}

static void
vips_foreign_load_sf3i_init(VipsForeignLoadSf3i *sf3i)
{
}

typedef struct _VipsForeignLoadSf3iFile {
	VipsForeignLoadSf3i parent_object;

	char *filename;

} VipsForeignLoadSf3iFile;

typedef VipsForeignLoadSf3iClass VipsForeignLoadSf3iFileClass;

G_DEFINE_TYPE(VipsForeignLoadSf3iFile, vips_foreign_load_sf3i_file,
	vips_foreign_load_sf3i_get_type());

static int
vips_foreign_load_sf3i_file_build(VipsObject *object)
{
	VipsForeignLoadSf3iFile *file = (VipsForeignLoadSf3iFile *) object;
	VipsForeignLoadSf3i *sf3i = (VipsForeignLoadSf3i *) object;

	if (file->filename &&
		!(sf3i->source = vips_source_new_from_file(file->filename)))
		return -1;

	return VIPS_OBJECT_CLASS(vips_foreign_load_sf3i_file_parent_class)
		->build(object);
}

static void
vips_foreign_load_sf3i_file_class_init(VipsForeignLoadSf3iClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "sf3iload";
	object_class->description = _("load sf3i from file");
	object_class->build = vips_foreign_load_sf3i_file_build;

	VIPS_ARG_STRING(class, "filename", 1,
		_("Filename"),
		_("Filename to load from"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsForeignLoadSf3iFile, filename),
		NULL);
}

static void
vips_foreign_load_sf3i_file_init(VipsForeignLoadSf3iFile *file)
{
}

typedef struct _VipsForeignLoadSf3iBuffer {
	VipsForeignLoadSf3i parent_object;

	VipsBlob *blob;

} VipsForeignLoadSf3iBuffer;

typedef VipsForeignLoadSf3iClass VipsForeignLoadSf3iBufferClass;

G_DEFINE_TYPE(VipsForeignLoadSf3iBuffer, vips_foreign_load_sf3i_buffer,
	vips_foreign_load_sf3i_get_type());

static int
vips_foreign_load_sf3i_buffer_build(VipsObject *object)
{
	VipsForeignLoadSf3i *sf3i = (VipsForeignLoadSf3i *) object;
	VipsForeignLoadSf3iBuffer *buffer = (VipsForeignLoadSf3iBuffer *) object;

	if (buffer->blob &&
		!(sf3i->source = vips_source_new_from_memory(
			  VIPS_AREA(buffer->blob)->data,
			  VIPS_AREA(buffer->blob)->length)))
		return -1;

	return VIPS_OBJECT_CLASS(vips_foreign_load_sf3i_buffer_parent_class)
		->build(object);
}

static gboolean
vips_foreign_load_sf3i_buffer_is_a_buffer(const void *buf, size_t len)
{
	VipsSource *source;
	gboolean result;

	if (!(source = vips_source_new_from_memory(buf, len)))
		return FALSE;
	result = vips_foreign_load_sf3i_is_a_source(source);
	VIPS_UNREF(source);

	return result;
}

static void
vips_foreign_load_sf3i_buffer_class_init(
	VipsForeignLoadSf3iBufferClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "sf3iload_buffer";
	object_class->description = _("load sf3i from buffer");
	object_class->build = vips_foreign_load_sf3i_buffer_build;

	load_class->is_a_buffer = vips_foreign_load_sf3i_buffer_is_a_buffer;

	VIPS_ARG_BOXED(class, "buffer", 1,
		_("Buffer"),
		_("Buffer to load from"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsForeignLoadSf3iBuffer, blob),
		VIPS_TYPE_BLOB);
}

static void
vips_foreign_load_sf3i_buffer_init(VipsForeignLoadSf3iBuffer *buffer)
{
}

typedef struct _VipsForeignLoadSf3iSource {
	VipsForeignLoadSf3i parent_object;

	VipsSource *source;

} VipsForeignLoadSf3iSource;

typedef VipsForeignLoadSf3iClass VipsForeignLoadSf3iSourceClass;

G_DEFINE_TYPE(VipsForeignLoadSf3iSource, vips_foreign_load_sf3i_source,
	vips_foreign_load_sf3i_get_type());

static int
vips_foreign_load_sf3i_source_build(VipsObject *object)
{
	VipsForeignLoadSf3i *sf3i = (VipsForeignLoadSf3i *) object;
	VipsForeignLoadSf3iSource *source = (VipsForeignLoadSf3iSource *) object;

	if (source->source) {
		sf3i->source = source->source;
		g_object_ref(sf3i->source);
	}

	return VIPS_OBJECT_CLASS(vips_foreign_load_sf3i_source_parent_class)
		->build(object);
}

static void
vips_foreign_load_sf3i_source_class_init(VipsForeignLoadSf3iFileClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS(class);
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "sf3iload_source";
	object_class->description = _("load sf3i from source");
	object_class->build = vips_foreign_load_sf3i_source_build;

	operation_class->flags |= VIPS_OPERATION_NOCACHE;

	load_class->is_a_source = vips_foreign_load_sf3i_is_a_source;

	VIPS_ARG_OBJECT(class, "source", 1,
		_("Source"),
		_("Source to load from"),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET(VipsForeignLoadSf3iSource, source),
		VIPS_TYPE_SOURCE);
}

static void
vips_foreign_load_sf3i_source_init(VipsForeignLoadSf3iSource *source)
{
}

#endif /*HAVE_SF3I*/

/**
 * vips_sf3iload:
 * @filename: file to load
 * @out: (out): output image
 * @...: `NULL`-terminated list of optional named arguments
 *
 * Read a SF3I image. Images are RGB or RGBA, 8 bits.
 *
 * ::: seealso
 *     [ctor@Image.new_from_file].
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_sf3iload(const char *filename, VipsImage **out, ...)
{
	va_list ap;
	int result;

	va_start(ap, out);
	result = vips_call_split("sf3iload", ap, filename, out);
	va_end(ap);

	return result;
}

/**
 * vips_sf3iload_buffer:
 * @buf: (array length=len) (element-type guint8): memory area to load
 * @len: (type gsize): size of memory area
 * @out: (out): image to write
 * @...: `NULL`-terminated list of optional named arguments
 *
 * Exactly as [ctor@Image.sf3iload], but read from a memory source.
 *
 * ::: seealso
 *     [ctor@Image.sf3iload].
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_sf3iload_buffer(void *buf, size_t len, VipsImage **out, ...)
{
	va_list ap;
	VipsBlob *blob;
	int result;

	/* We don't take a copy of the data or free it.
	 */
	blob = vips_blob_new(NULL, buf, len);

	va_start(ap, out);
	result = vips_call_split("sf3iload_buffer", ap, blob, out);
	va_end(ap);

	vips_area_unref(VIPS_AREA(blob));

	return result;
}

/**
 * vips_sf3iload_source:
 * @source: source to load
 * @out: (out): output image
 * @...: `NULL`-terminated list of optional named arguments
 *
 * Exactly as [ctor@Image.sf3iload], but read from a source.
 *
 * ::: seealso
 *     [ctor@Image.sf3iload].
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_sf3iload_source(VipsSource *source, VipsImage **out, ...)
{
	va_list ap;
	int result;

	va_start(ap, out);
	result = vips_call_split("sf3iload_source", ap, source, out);
	va_end(ap);

	return result;
}
