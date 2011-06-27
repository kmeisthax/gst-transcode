/*
 * Copyright (C) 2011 David Wendt <dcrkid@yahoo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gst/gst.h>

#include "gsttranscodebin.h"
#include "config.h"

static gboolean plugin_init (GstPlugin* plugin) {
    return gst_element_register (plugin, "transcodebin", GST_RANK_NONE, GST_TYPE_TRANSCODE_BIN);
};

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "gst-transcode",
    "Transcoding helpers for GStreamer",
    plugin_init,
    VERSION,
    "LGPL",
    "libgst-transcode",
    "http://gstreamer.net/" //FIXME: Get an actual url
)
