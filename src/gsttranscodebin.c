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

#include "gsttranscodebin.h"

#include <gst/pbutils/encoding-profile.h>

//more GObject related boilerplate
GST_BOILERPLATE(GstTranscodeBin, gst_transcode_bin, GstBin, GST_TYPE_BIN);

enum {
    PROP_0,
    PROP_PROFILE,
    PROP_COUNT
};

static void gst_transcode_bin_set_property(GObject* goself, guint propid, const GValue* val, GParamSpec* pspec) {
    GstTranscodeBin *self = (GstTranscodeBin*) goself;
    
    switch (propid) {
        case PROP_PROFILE:
            g_object_set_property(G_OBJECT (self->ebin), "profile", val);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(goself, propid, pspec);
            break;
    }
};

static void gst_transcode_bin_get_property(GObject* goself, guint propid, GValue* val, GParamSpec* pspec) {
    GstTranscodeBin *self = (GstTranscodeBin*) goself;
    
    switch (propid) {
        case PROP_PROFILE:
            g_object_get_property(G_OBJECT (self->ebin), "profile", val);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(goself, propid, pspec);
            break;
    }
};

static void gst_transcode_bin_dispose (GObject *goself) {
    GstTranscodeBin* self = GST_TRANSCODE_BIN (goself);
    
    //Release and unref any remaining request pads
    while (self->reqpads != NULL) {
        GstPad* pad = GST_PAD (self->reqpads->data);
        
        gst_element_release_request_pad(self->ebin, pad);
        gst_object_unref(pad);
        
        self->reqpads = g_list_delete_link(self->reqpads, self->reqpads);
    }
    
    G_OBJECT_CLASS (parent_class)->dispose(goself);
};

static void gst_transcode_bin_class_init (GstTranscodeBinClass* kls) {
    GObjectClass *gokls = G_OBJECT_CLASS (kls);
    gokls->get_property = gst_transcode_bin_get_property;
    gokls->set_property = gst_transcode_bin_set_property;
    gokls->dispose = gst_transcode_bin_dispose;

    g_object_class_install_property(gokls,
                                    PROP_PROFILE,
                                    gst_param_spec_mini_object(GST_TRANSCODE_BIN_PROP_PROFILE,
                                                               "Profile",
                                                               "The GstEncodingProfile to use",
                                                               GST_TYPE_ENCODING_PROFILE,
                                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
};

static gboolean gst_transcode_bin_cast_autoplug_spell(GstTranscodeBin *self, GstPad *pad) {
    GstPad* encode_sink = NULL;
    encode_sink = gst_element_get_compatible_pad((GstElement*) self->ebin, pad, self->dcaps);
    
    gboolean is_request_pad = FALSE;
    if (encode_sink == NULL) {
        g_signal_emit_by_name(self->ebin, "request-pad", self->ebin, self->dcaps, &encode_sink);
        is_request_pad = TRUE;
    }
    
    if (encode_sink == NULL) {
        //TODO: Make this post some sort of error
        return FALSE;
    }
    
    gboolean link_ok = (gst_pad_link(pad, encode_sink) == GST_PAD_LINK_OK);
    
    if (!link_ok && is_request_pad) {
        gst_element_release_request_pad(self->ebin, encode_sink);
        gst_object_unref(encode_sink);
    } else if (is_request_pad) {
        self->reqpads = g_list_prepend(self->reqpads, encode_sink);
    }
    
    return link_ok;
};

static gboolean gst_transcode_bin_dbin_autoplug_continue(GstElement* bin, GstPad* pad, GstCaps* caps, gpointer user_data) {
    GstTranscodeBin* self = GST_TRANSCODE_BIN (user_data);
    self->dcaps = caps;
    
    return !gst_transcode_bin_cast_autoplug_spell(self, pad);
};

static void gst_transcode_bin_dbin_pad_added(GstElement* bin, GstPad* pad, gpointer user_data) {
    GstTranscodeBin* self = GST_TRANSCODE_BIN (user_data);
    gst_transcode_bin_cast_autoplug_spell(self, pad);
};

#define     ENCODE_BIN      "encodebin"
#define     DECODE_BIN      "decodebin2"

static void gst_transcode_bin_init (GstTranscodeBin* self, GstTranscodeBinClass* kls) {
    GstBin* gbself = (GstBin*) self;
    GstElement* geself = (GstElement*) self;
    
    self->ebin = gst_element_factory_make(ENCODE_BIN, ENCODE_BIN);
    self->dbin = gst_element_factory_make(DECODE_BIN, DECODE_BIN);
    
    gst_bin_add_many(gbself, self->dbin, self->ebin, NULL);
    
    g_signal_connect(self->dbin, "autoplug-continue", G_CALLBACK(gst_transcode_bin_dbin_autoplug_continue), self);
    g_signal_connect(self->dbin, "pad-added", G_CALLBACK(gst_transcode_bin_dbin_pad_added), self);
    
    GstPad* isrcpad = gst_element_get_static_pad(self->ebin, "src");
    GstPad* isinkpad = gst_element_get_static_pad(self->dbin, "sink");
    
    self->srcpad = gst_ghost_pad_new("src", isrcpad);
    self->sinkpad = gst_ghost_pad_new("sink", isinkpad);
    
    gst_element_add_pad(geself, GST_PAD (self->srcpad)  );
    gst_element_add_pad(geself, GST_PAD (self->sinkpad) );
    
    self->dcaps = NULL;
    self->reqpads = NULL;
};

static void gst_transcode_bin_base_init (gpointer gpkls) {
    GstElementClass* elemkls = GST_ELEMENT_CLASS (gpkls);

    gst_element_class_set_details_simple (elemkls,
                                          "Automatic Transcoder",
                                          "Generic/Bin/Transcoder",
                                          "Convenience transcoding element.\n\nElement capable of taking any input GStreamer can decode and encoding to any profile GStreamer supports without hassle.",
                                          "David Wendt <dcrkid@yahoo.com>");
};
