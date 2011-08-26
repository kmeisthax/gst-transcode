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

GST_BOILERPLATE (GstTranscodeBin, gst_transcode_bin, GstBin, GST_TYPE_BIN);

#define     ENCODE_BIN      "encodebin"
#define     DECODE_BIN      "decodebin2"

enum
{
  PROP_0,
  PROP_PROFILE,
  PROP_COUNT
};

static void gst_transcode_bin_base_init (gpointer gpkls);
static void gst_transcode_bin_class_init (GstTranscodeBinClass * kls);
static void gst_transcode_bin_init (GstTranscodeBin * self,
    GstTranscodeBinClass * kls);
static void gst_transcode_bin_set_property (GObject * goself, guint propid,
    const GValue * val, GParamSpec * pspec);
static void gst_transcode_bin_get_property (GObject * goself, guint propid,
    GValue * val, GParamSpec * pspec);
static void gst_transcode_bin_dispose (GObject * goself);

static gboolean _cast_autoplug_spell (GstTranscodeBin * self, GstPad * pad);
static gboolean _dbin_autoplug_continue (GstElement * bin, GstPad * pad,
    GstCaps * caps, gpointer user_data);
static void _dbin_pad_added (GstElement * bin, GstPad * pad,
    gpointer user_data);

static void
gst_transcode_bin_base_init (gpointer gpkls)
{
  GstElementClass *elemkls = GST_ELEMENT_CLASS (gpkls);

  gst_element_class_set_details_simple (elemkls,
      "Automatic Transcoder",
      "Generic/Bin/Transcoder",
      "Convenience transcoding element.\n\nElement capable of taking any input GStreamer can decode and encoding to any profile GStreamer supports without hassle.",
      "David Wendt <dcrkid@yahoo.com>");
};

static void
gst_transcode_bin_class_init (GstTranscodeBinClass * kls)
{
  GObjectClass *gokls = G_OBJECT_CLASS (kls);
  gokls->get_property = gst_transcode_bin_get_property;
  gokls->set_property = gst_transcode_bin_set_property;
  gokls->dispose = gst_transcode_bin_dispose;

  /* Properties */

  /** GstTranscodeBin:profile:
   *
   * Encoding profile to target. Like #GstEncodeBin, must be set before going
   * to %GST_STATE_PAUSED or higher.
   */
  g_object_class_install_property (gokls, PROP_PROFILE,
      gst_param_spec_mini_object ("profile", "profile",
          "The GstEncodingProfile to use", GST_TYPE_ENCODING_PROFILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
};

static void
gst_transcode_bin_init (GstTranscodeBin * self, GstTranscodeBinClass * kls)
{
  GstBin *gbself = (GstBin *) self;
  GstElement *geself = (GstElement *) self;
  GstPad *isrcpad, *isinkpad;

  self->ebin = gst_element_factory_make (ENCODE_BIN, ENCODE_BIN);
  self->dbin = gst_element_factory_make (DECODE_BIN, DECODE_BIN);

  gst_bin_add_many (gbself, self->dbin, self->ebin, NULL);

  g_signal_connect (self->dbin, "autoplug-continue",
      G_CALLBACK (_dbin_autoplug_continue), self);
  g_signal_connect (self->dbin, "pad-added",
      G_CALLBACK (_dbin_pad_added), self);

  isrcpad = gst_element_get_static_pad (self->ebin, "src");
  isinkpad = gst_element_get_static_pad (self->dbin, "sink");

  self->srcpad = gst_ghost_pad_new ("src", isrcpad);
  self->sinkpad = gst_ghost_pad_new ("sink", isinkpad);

  gst_element_add_pad (geself, self->sinkpad);
  gst_element_add_pad (geself, self->srcpad);

  self->dcaps = NULL;
  self->reqpads = NULL;
};

static void
gst_transcode_bin_set_property (GObject * goself, guint propid,
    const GValue * val, GParamSpec * pspec)
{
  GstTranscodeBin *self = (GstTranscodeBin *) goself;

  switch (propid) {
    case PROP_PROFILE:{
      GstEncodingProfile *prof = GST_ENCODING_PROFILE
          (gst_value_get_mini_object (val));
      g_object_set (G_OBJECT (self->ebin), "profile", prof, NULL);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (goself, propid, pspec);
      break;
  }
};

static void
gst_transcode_bin_get_property (GObject * goself, guint propid,
    GValue * val, GParamSpec * pspec)
{
  GstTranscodeBin *self = (GstTranscodeBin *) goself;

  switch (propid) {
    case PROP_PROFILE:
      g_object_get_property (G_OBJECT (self->ebin), "profile", val);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (goself, propid, pspec);
      break;
  }
};

static void
gst_transcode_bin_dispose (GObject * goself)
{
  GstTranscodeBin *self = GST_TRANSCODE_BIN (goself);

  while (self->reqpads != NULL) {
    GstPad *pad = GST_PAD (self->reqpads->data);

    gst_element_release_request_pad (self->ebin, pad);
    gst_object_unref (pad);

    self->reqpads = g_list_delete_link (self->reqpads, self->reqpads);
  }

  G_OBJECT_CLASS (parent_class)->dispose (goself);
};

static gboolean
_cast_autoplug_spell (GstTranscodeBin * self, GstPad * pad)
{
  GstPad *encode_sink;
  gboolean is_request_pad = FALSE;
  gboolean link_ok;
  gchar *padname, *decaps, *epadname;

  encode_sink = gst_element_get_compatible_pad (self->ebin, pad, NULL);

  if (encode_sink == NULL) {
    g_signal_emit_by_name (self->ebin, "request-pad", gst_pad_get_caps (pad),
        &encode_sink, NULL);
    is_request_pad = TRUE;

    if (encode_sink == NULL) {
      padname = gst_pad_get_name (pad);
      decaps = gst_caps_to_string (self->dcaps);
      GST_DEBUG
          ("No compatible encodebin pad found for pad '%s' with caps '%s', ignoring...",
          padname, decaps);

      g_free (padname);
      g_free (decaps);
      return FALSE;
    }
  }

  padname = gst_pad_get_name (pad);
  decaps = gst_caps_to_string (self->dcaps);
  epadname = gst_pad_get_name (encode_sink);

  GST_DEBUG ("pad '%s' with caps '%s' is compatible with '%s'", padname,
      decaps, epadname);

  link_ok = (gst_pad_link (pad, encode_sink) == GST_PAD_LINK_OK);

  if (!link_ok && is_request_pad) {
    gst_element_release_request_pad (self->ebin, encode_sink);
    gst_object_unref (encode_sink);
  } else if (is_request_pad) {
    self->reqpads = g_list_prepend (self->reqpads, encode_sink);
  }

  if (!link_ok) {
    GST_WARNING ("Failed to link pad '%s' to '%s'", padname, epadname);
  }

  g_free (padname);
  g_free (decaps);
  g_free (epadname);

  return link_ok;
};

static gboolean
_dbin_autoplug_continue (GstElement * bin, GstPad * pad, GstCaps *
    caps, gpointer user_data)
{
  GstTranscodeBin *self = GST_TRANSCODE_BIN (user_data);
  self->dcaps = caps;

  return !_cast_autoplug_spell (self, pad);
};

static void
_dbin_pad_added (GstElement * bin, GstPad * pad, gpointer user_data)
{
  GstTranscodeBin *self = GST_TRANSCODE_BIN (user_data);
  _cast_autoplug_spell (self, pad);
};
