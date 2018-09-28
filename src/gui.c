/*
 * vkrunner
 *
 * Copyright (C) 2018 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <stdlib.h>
#include <gtk/gtk.h>

#include "gui-worker.h"

struct gui {
        GtkWidget *window;
        GtkTextBuffer *text_buffer;
        guint timeout_source;

        struct gui_worker *worker;

        uint64_t serial_id;
};

static void
worker_cb(const struct gui_worker_data *data,
          void *user_data)
{
        g_print("Result: %s\nLog:\n%s\n",
                vr_result_to_string(data->result),
                data->log);
}

static gboolean
timeout_cb(void *user_data)
{
        struct gui *gui = user_data;

        gui->timeout_source = 0;

        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(gui->text_buffer, &start, &end);
        gchar *text = gtk_text_buffer_get_text(gui->text_buffer,
                                               &start, &end,
                                               FALSE /* include hidden */);
        gui_worker_set_source(gui->worker,
                              ++gui->serial_id,
                              text);
        g_free(text);

        return G_SOURCE_REMOVE;
}

static void
text_changed_cb(GtkTextBuffer *buffer,
                void *user_data)
{
        struct gui *gui = user_data;

        if (gui->timeout_source)
                g_source_remove(gui->timeout_source);

        gui->timeout_source = g_timeout_add(1000, timeout_cb, gui);
}

static struct gui *
gui_new(void)
{
        struct gui *gui = g_malloc0(sizeof *gui);

        gui->window = g_object_ref_sink(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        gtk_window_set_title(GTK_WINDOW(gui->window), "VkRunner");
        gtk_window_set_default_size(GTK_WINDOW(gui->window), 800, 600);
        g_signal_connect(G_OBJECT(gui->window),
                         "destroy",
                         G_CALLBACK(gtk_main_quit),
                         NULL /* user_data */);

        gui->worker = gui_worker_new(worker_cb, gui);

        GtkWidget *scrolled_view = gtk_scrolled_window_new(NULL, NULL);

        gui->text_buffer = gtk_text_buffer_new(NULL);
        g_signal_connect(G_OBJECT(gui->text_buffer),
                         "changed",
                         G_CALLBACK(text_changed_cb),
                         gui);
        GtkWidget *text_view = gtk_text_view_new_with_buffer(gui->text_buffer);

        gtk_container_add(GTK_CONTAINER(scrolled_view), text_view);

        gtk_container_add(GTK_CONTAINER(gui->window), scrolled_view);

        gtk_widget_show_all(gui->window);

        return gui;
}

static void
gui_free(struct gui *gui)
{
        g_object_unref(gui->window);
        g_object_unref(gui->text_buffer);

        gui_worker_free(gui->worker);

        g_free(gui);
}

int
main(int argc, char **argv)
{
        gtk_init(&argc, &argv);

        struct gui *gui = gui_new();

        gtk_main();

        gui_free(gui);

        return EXIT_SUCCESS;
}