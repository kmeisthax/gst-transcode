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

#include <glib-object.h>
#include <gst/gst.h>
#include <gst/pbutils/encoding-profile.h>
#include <libgupnp-dlna/gupnp-dlna-discoverer.h>

#include <stdio.h>
#include <unistd.h>

int create_transcode_pipeline(const char* source_filename, const char* dest_filename, GstEncodingProfile* prof, GstPipeline** pipe) {
    *pipe = (GstPipeline*)gst_pipeline_new("tcode");
    if (*pipe == NULL) {
        printf("Could not construct Pipeline\n");
        return -1;
    }
    
    GstElementFactory* xcodefact = gst_element_factory_find("transcodebin");
    if (xcodefact == NULL) {
        printf("Could not construct Transcode bin factory\n");
        gst_object_unref(*pipe);
        return -2;
    }
    
    GstElement* xcode = gst_element_factory_create(xcodefact, "tcode-bin");
    if (xcode == NULL) {
        printf("Could not construct Transcode bin element\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        return -3;
    }

    GstElementFactory* fsrcfact = gst_element_factory_find("filesrc");
    if (fsrcfact == NULL) {
        printf("Could not construct File Source factory\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        return -4;
    }
    
    GstElement* filesrc = gst_element_factory_create(fsrcfact, "file-src");
    if (filesrc == NULL) {
        printf("Could not construct File Source element\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        return -5;
    }

    GstElementFactory* fsinkfact = gst_element_factory_find("filesink");
    if (fsinkfact == NULL) {
        printf("Could not construct File Sink factory\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        return -6;
    }
    
    GstElement* filesink = gst_element_factory_create(fsinkfact, "file-sink");
    if (filesink == NULL) {
        printf("Could not construct File Sink element\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        gst_object_unref(fsinkfact);
        return -7;
    }
    
    g_object_set(G_OBJECT (filesrc), "location", source_filename, NULL);
    g_object_set(G_OBJECT (filesink), "location", dest_filename, NULL);
    g_object_set(G_OBJECT (xcode), "profile", prof, NULL);

    GstPad* fsrc_srcpad = gst_element_get_static_pad(filesrc, "src");
    if (fsrc_srcpad == NULL) {
        printf("WTF, filesrc has no srcpad\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        return -8;
    }
    
    GstPad* xcod_sinkpad = gst_element_get_static_pad(xcode, "sink");
    if (xcod_sinkpad == NULL) {
        printf("WTF, transcodebin has no sinkpad\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        gst_object_unref(fsrc_srcpad);
        return -9;
    }
    
    GstPadLinkReturn linkret = gst_pad_link(fsrc_srcpad, xcod_sinkpad);
    gst_object_unref(fsrc_srcpad);
    gst_object_unref(xcod_sinkpad);
    
    if (linkret != GST_PAD_LINK_OK) {
        printf("Error when linking filesrc and transcodebin pads; error code %d\n", linkret);
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        return -10;
    }

    GstPad* xcod_srcpad = gst_element_get_static_pad(xcode, "src");
    if (xcod_srcpad == NULL) {
        printf("WTF, transcodebin has no srcpad\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        return -11;
    }
    
    GstPad* fsink_sinkpad = gst_element_get_static_pad(filesink, "sink");
    if (fsink_sinkpad == NULL) {
        printf("WTF, filesink has no sinkpad\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        gst_object_unref(xcod_srcpad);
        return -12;
    }
    
    GstPadLinkReturn linkret2 = gst_pad_link(xcod_srcpad, fsink_sinkpad);
    gst_object_unref(xcod_srcpad);
    gst_object_unref(fsink_sinkpad);
    if (linkret2 != GST_PAD_LINK_OK) {
        printf("Error when linking transcodebin and filesink pads; error code %d\n", linkret);
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        return -13;
    }
    
    if (gst_bin_add(GST_BIN (pipe), filesrc) == FALSE) {
        printf("FAILED to add filesrc to pipeline\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(filesrc);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        return -14;
    }

    if (gst_bin_add(GST_BIN (pipe), xcode) == FALSE) {
        printf("FAILED to add transcodebin to pipeline\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(xcode);
        gst_object_unref(fsrcfact);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        return -15;
    }

    if (gst_bin_add(GST_BIN (pipe), filesink) == FALSE) {
        printf("FAILED to add filesink to pipeline\n");
        gst_object_unref(*pipe);
        gst_object_unref(xcodefact);
        gst_object_unref(fsrcfact);
        gst_object_unref(fsinkfact);
        gst_object_unref(filesink);
        return -16;
    }

    gst_object_unref(xcodefact);
    gst_object_unref(fsrcfact);
    gst_object_unref(fsinkfact);
    return 0;
};

int main (int argc, char** argv) {
    /* Transcode standard input into all available DLNA video profiles.
     */
    g_type_init();
    g_thread_init(NULL);
    gst_init(&argc, &argv);
    
    GUPnPDLNADiscoverer* profsrc = gupnp_dlna_discoverer_new((GstClockTime)10 * GST_SECOND, FALSE, FALSE);
    if (profsrc == NULL) {
        printf("Could not construct GUPnP-DLNA Discoverer\n");
        exit(-1);
    }

    const GList* profiles = gupnp_dlna_discoverer_list_profiles(profsrc);
    int num_profs = -1;
    
    char source_filename[81];
    printf("Please enter source filename (80 chars max): ");
    scanf("%s", source_filename);
    
    char dest_filename[91];

    while (profiles != NULL) {
        GstEncodingProfile* prof = gupnp_dlna_profile_get_encoding_profile(profiles->data);
        profiles = profiles->next;
        num_profs++;
        
        sprintf(dest_filename, "%s-%d", source_filename, num_profs);

        printf("Transcoding %s to %s\n", source_filename, dest_filename);
        
        GstPipeline* pipe = NULL;
        int succ = create_transcode_pipeline(source_filename, dest_filename, prof, &pipe);
        if (succ < 0) {
            printf("For some reason, the pipeline didn't get created for profile %d (error code %d)\n", num_profs, succ);
            if (pipe != NULL) {
                gst_object_unref(pipe);
            }
            continue;
        }

        gboolean die = FALSE;
        GstStateChangeReturn gelemnst = gst_element_set_state((GstElement*)(pipe), GST_STATE_PLAYING);
        switch (gelemnst) {
            case GST_STATE_CHANGE_FAILURE:
                printf("Attempting to bring the pipeline to playing FAILED. (profile %d)\n", num_profs);
                gst_object_unref(pipe);
                die = TRUE;
                break;
            case GST_STATE_CHANGE_ASYNC:
                printf("State change asynchronous, waiting for completion...");
                gelemnst = gst_element_get_state((GstElement*)(pipe), NULL, NULL, GST_CLOCK_TIME_NONE);
                switch (gelemnst) {
                    case GST_STATE_CHANGE_FAILURE:
                        printf("Attempting to bring the pipeline to playing FAILED. (profile %d)\n", num_profs);
                        gst_object_unref(pipe);
                        die = TRUE;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }

        if (die) continue;

        while (TRUE) {
            sleep(5);
            
            GstQuery *duration, *position;
            gboolean dursuc, possuc;

            duration = gst_query_new_duration (GST_FORMAT_TIME);
            if (duration == NULL) continue;
            
            dursuc = gst_element_query((GstElement*)(pipe), duration);

            position = gst_query_new_position (GST_FORMAT_TIME);
            if (position == NULL) {
                gst_query_unref(duration);
            };
            
            possuc = gst_element_query((GstElement*)(pipe), position);

            if (dursuc && possuc) {
                gfloat progress = 0.0f;
                gint64 durcnt, poscnt;
                
                gst_query_parse_duration(duration, NULL, &durcnt);
                gst_query_parse_position(position, NULL, &poscnt);

                progress = (gfloat)durcnt / (gfloat)poscnt;

                printf("Encoding progress %.2f%%\n", progress * 100.0f);

                if (progress >= 1.0f) {
                    gst_query_unref(duration);
                    gst_query_unref(position);
                    break;
                }
            }

            gst_query_unref(duration);
            gst_query_unref(position);
        }

        gst_object_unref(pipe);
    }

    printf("Made %d test encodes\n", num_profs + 1);
    return 0;
};
