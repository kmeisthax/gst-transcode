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

#ifndef __GST_TRANSCODE_BIN_H__
#define __GST_TRANSCODE_BIN_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/**
 * GstTranscodeBin:
 *
 * A convenience element that converts any input media into output media
 * conformant to a particular #GstEncodingProfile. Internally, it's an autoplug
 * wrapper around #GstDecodeBin2 and #GstEncodeBin, providing similar pads
 * to said bins.
 */

#define GST_TYPE_TRANSCODE_BIN              (gst_transcode_bin_get_type ())
#define GST_TRANSCODE_BIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_TRANSCODE_BIN, GstTranscodeBin))
#define GST_IS_TRANSCODE_BIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_TRANSCODE_BIN))
#define GST_TRANSCODE_BIN_CLASS(kls)        (G_TYPE_CHECK_CLASS_CAST ((kls), GST_TYPE_TRANSCODE_BIN, GstTranscodeBinClass))
#define GST_IS_TRANSCODE_BIN_CLASS(kls)     (G_TYPE_CHECK_CLASS_TYPE ((kls), GST_TYPE_TRANSCODE_BIN))
#define GST_TRANSCODE_BIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_TRANSCODE_BIN, GstTranscodeBinClass))

typedef struct _GstTranscodeBin
{
    GstBin parent_instance;

    /* private elements
     * do not use, will move to private struct later */
    GstElement* ebin;
    GstElement* dbin;
    
    GstGhostPad* srcpad;
    GstGhostPad* sinkpad;
    
    GstCaps* dcaps;
    GList* reqpads;
} GstTranscodeBin;

typedef GstBinClass GstTranscodeBinClass;

GType gst_transcode_bin_get_type(void);

/* Properties */

/** GstTranscodeBin:profile:
 *
 * Encoding profile to target. Like #GstEncodeBin, must be set before going to
 * %GST_STATE_PAUSED or higher.
 */

#define GST_TRANSCODE_BIN_PROP_PROFILE           "profile"

G_END_DECLS

#endif
