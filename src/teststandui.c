#include <gtk/gtk.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include "load_cell_cli.h"

#define PRIMARY_TRACE_COUNT 1
#define TOTAL_TRACE_COUNT 1
#define DATA_POINTS_TO_KEEP 500
#define M_PI 3.14159265358979323846 

enum {
    COL_TIME,
    COL_PRIMARY,
    NUM_COLS
};

typedef struct {
    double x_time;
    double y_value;
} DataPoint;

typedef struct {
    GList *data;
    char name[32];
    GdkRGBA color;
    gboolean visible;
    GtkWidget *check_button;
    char css_class[16];
} TraceData;

typedef struct {
    GtkWidget *drawing_area;
    GtkWidget *status_bar;
    GtkWidget *start_stop_button;
    GtkListStore *list_store; 
    TraceData traces[TOTAL_TRACE_COUNT];
    double max_x;
    double max_y;
    gboolean is_recording;
    gboolean has_data_input;
    gboolean test_mode;
    guint test_timer_id;
} GraphData;

static GraphData g_graph_data = {0};
static guint g_timer_source_id = 0; /* used for network client if needed (kept 0) */
static gdouble g_current_time = 0.0;
static const GdkRGBA g_colors[] = {
    {1.0, 0.0, 0.0, 1.0},
    {0.0, 0.0, 1.0, 1.0}
};

static void on_start_stop_clicked(GtkButton *button, gpointer user_data);
static void on_reset_clicked(GtkButton *button, gpointer user_data);
static void log_new_data(GraphData *gd, double primary_val);

static void update_table(GraphData *gd, double time, double primary_val) {
    GtkTreeIter iter;
    gtk_list_store_append(gd->list_store, &iter);
    gtk_list_store_set(gd->list_store, &iter, COL_TIME, time, COL_PRIMARY, primary_val, -1);
}

static void log_new_data(GraphData *gd, double primary_val) {
    

    g_current_time += 0.1; 
    
    TraceData *primary_trace = &gd->traces[0];
    DataPoint *new_point = g_malloc(sizeof(DataPoint));
    new_point->x_time = g_current_time;
    new_point->y_value = primary_val;

    primary_trace->data = g_list_append(primary_trace->data, new_point);
    
    if (g_list_length(primary_trace->data) > DATA_POINTS_TO_KEEP) {
        GList *old_node = primary_trace->data;
        g_free(old_node->data);
        primary_trace->data = g_list_delete_link(primary_trace->data, old_node);
    }
    
    update_table(gd, g_current_time, primary_val);

    gd->max_x = g_current_time;

    /* recompute max_y dynamically from both traces (find peak and add 10% headroom) */
    double maxy = 0.0;
    for (int ti = 0; ti < TOTAL_TRACE_COUNT; ++ti) {
        for (GList *l = gd->traces[ti].data; l != NULL; l = l->next) {
            DataPoint *p = (DataPoint *)l->data;
            if (p->y_value > maxy) maxy = p->y_value;
        }
    }
    if (maxy <= 0.0) maxy = 1.0;
    gd->max_y = maxy * 1.1; /* 10% headroom */

    gtk_widget_queue_draw(gd->drawing_area);
}


// timer and random data generation
/*
 * on_tick_timer: callback used by the network client when packets arrive.
 * Signature matches run_load_cell_cli callback: void (*)(output_data *, void *)
 */
/* Idle marshaling structure for passing data from worker threads to main loop */
typedef struct {
    output_data data;
    GraphData *gd;
} IdleOutput;

static gboolean idle_handle_output(gpointer user_data) {
    IdleOutput *io = (IdleOutput *)user_data;
    if (io && io->gd) {
        io->gd->has_data_input = TRUE;
        /* only log when recording is active */
        if (io->gd->is_recording) {
            log_new_data(io->gd, io->data.value_tare);
        }
#if FAST_AS_POSSIBLE
#else
static void on_tick_timer(output_data * data, gpointer user_data) {
    GraphData *gd = (GraphData *)user_data;
    if (!gd->is_recording) {
       return;
        //return G_SOURCE_CONTINUE; 
    }
    free(io);
    return G_SOURCE_REMOVE;
}

/* Network callback — invoked in worker thread: marshal to main loop */
static void on_tick_timer(output_data * data, gpointer user_data) {
    if (!data || !user_data) return;
    GraphData *gd = (GraphData *)user_data;
    /* if UI not ready yet, ignore incoming ticks */
    if (!gd || !gd->drawing_area || !gd->list_store) return;

    IdleOutput *io = malloc(sizeof(IdleOutput));
    if (!io) return;
    io->data = *data; /* copy */
    io->gd = gd;

    /* schedule on main loop to safely update GTK widgets */
    g_idle_add(idle_handle_output, io);
}

/* ui_timer_cb: GSourceFunc-compatible timer used for UI-only random data generation */
static gboolean ui_timer_cb(gpointer user_data) {
    if (!user_data) return TRUE;
    GraphData *gd = (GraphData *)user_data;
    if (!gd || !gd->drawing_area || !gd->list_store) return TRUE;
    if (!gd->is_recording) return TRUE; /* keep the timer running */

    double primary_val = (double)(rand() % 1000) / 10.0;
    log_new_data(gd, primary_val);
    return TRUE; /* continue calling */
}
#endif

static void on_test_toggled(GtkToggleButton *button, gpointer user_data);

static void on_start_stop_clicked(GtkButton *button, gpointer user_data) {
    GraphData *gd = (GraphData *)user_data;
    /* Only toggle recording if there is a data source (network or test) */
    if (!gd->has_data_input && !gd->test_mode) {
        gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, "No data source available. Click Test to simulate data.");
        return;
    }

    gd->is_recording = !gd->is_recording;
    
    if (gd->is_recording) {
        gtk_button_set_label(button, "Stop Recording");
        gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, "Recording data...");
    } else {
        gtk_button_set_label(button, "Start Recording");
        gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, "Recording Paused. Click Reset to clear data.");
    }
}

static gboolean test_timer_cb(gpointer user_data) {
    if (!user_data) return TRUE;
    GraphData *gd = (GraphData *)user_data;
    if (!gd || !gd->drawing_area || !gd->list_store) return TRUE;
    if (!gd->is_recording) return TRUE;
    double primary_val = (double)(rand() % 1000) / 10.0;
    log_new_data(gd, primary_val);
    return TRUE;
}

static void on_test_toggled(GtkToggleButton *button, gpointer user_data) {
    GraphData *gd = (GraphData *)user_data;
    gboolean active = gtk_toggle_button_get_active(button);

    if (active) {
        gd->test_mode = TRUE;
        gd->has_data_input = TRUE; /* simulate data source present */
        /* start test timer */
        gd->test_timer_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, test_timer_cb, gd, NULL);
        gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, "Test mode: generating simulated data.");
    } else {
        gd->test_mode = FALSE;
        gd->has_data_input = FALSE;
        if (gd->test_timer_id) {
            g_source_remove(gd->test_timer_id);
            gd->test_timer_id = 0;
        }
        gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, "Test mode stopped.");
    }
}

static void reset_graph_data(GraphData *gd) {
    for (int i = 0; i < TOTAL_TRACE_COUNT; i++) {
        g_list_free_full(gd->traces[i].data, g_free);
        gd->traces[i].data = NULL;
    }
    
    if (gd->list_store) {
        gtk_list_store_clear(gd->list_store);
    }
    
    g_current_time = 0.0;
    gd->max_x = 0.0;
    gd->max_y = 150.0; 
    
    gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, "Graph data cleared. Ready to record.");
    gtk_widget_queue_draw(gd->drawing_area);
}

static void on_reset_clicked(GtkButton *button, gpointer user_data) {
    GraphData *gd = (GraphData *)user_data;
    
    if (gd->is_recording) {
        on_start_stop_clicked(GTK_BUTTON(NULL), gd); 
    }
    
    reset_graph_data(gd);
}

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GraphData *gd = (GraphData *)user_data;

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    
    const int margin_left = 60; 
    const int margin_right = 20;
    const int margin_top = 20;
    const int margin_bottom = 40; 
    
    int graph_width = width - margin_left - margin_right;
    int graph_height = height - margin_top - margin_bottom;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); 
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); 

    double scale_x = gd->max_x > 0 ? (double)graph_width / gd->max_x : 0.0;
    double scale_y = gd->max_y > 0 ? (double)graph_height / gd->max_y : 0.0;

    /* 1. Draw Grid Lines */
    
    // Set a lighter color and dashed line style for the grid
    cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 1.0); // Light gray for grid
    cairo_set_line_width(cr, 0.5);
    double dashes[] = {2.0, 2.0};
    cairo_set_dash(cr, dashes, 2, 0);

    // Horizontal Grid Lines (aligned with Y-axis ticks)
    int y_ticks = 5;
    for (int i = 0; i <= y_ticks; ++i) {
        double y_pos = margin_top + graph_height - (double)graph_height * i / y_ticks;
        
        cairo_move_to(cr, margin_left, y_pos);
        cairo_line_to(cr, margin_left + graph_width, y_pos);
        cairo_stroke(cr);
    }

    // Vertical Grid Lines (aligned with Time-axis ticks)
    int x_ticks = 4; // 25%, 50%, 75%, 100% (already defined in your label loop)
    for (int q = 1; q <= x_ticks; ++q) { // Start at 1 to skip the Y-axis (q=0)
        double x_pos = margin_left + (double)graph_width * q / x_ticks;

        cairo_move_to(cr, x_pos, margin_top);
        cairo_line_to(cr, x_pos, margin_top + graph_height);
        cairo_stroke(cr);
    }

    /* 2. Draw Zero-Level Line (Solid and Darker) */

    // Reset line style to solid
    cairo_set_dash(cr, NULL, 0, 0); 
    
    // Check if 0 is within the visible Y-range (0 to max_y)
    if (gd->max_y > 0) {
        // Calculate the screen Y position for a data value of 0.0
        double y_pos_zero = margin_top + graph_height - (0.0 * scale_y);

        // Set line style for the bolded zero line
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3); // Darker gray
        cairo_set_line_width(cr, 2.0); // Thicker line

        // Draw the line across the graph area
        cairo_move_to(cr, margin_left, y_pos_zero);
        cairo_line_to(cr, margin_left + graph_width, y_pos_zero);
        cairo_stroke(cr);
    }

    // Reset color to black for axes and labels
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    // Reset line width for axes
    cairo_set_line_width(cr, 1.0);

    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, margin_left, margin_top + graph_height);
    cairo_line_to(cr, margin_left + graph_width, margin_top + graph_height); 
    cairo_move_to(cr, margin_left, margin_top);
    cairo_line_to(cr, margin_left, margin_top + graph_height); 
    cairo_stroke(cr);

    cairo_set_font_size(cr, 12.0);
    
    cairo_save(cr);
    cairo_translate(cr, 15, margin_top + graph_height / 2);
    cairo_rotate(cr, -M_PI / 2.0);
    cairo_move_to(cr, 0, 0);
    cairo_show_text(cr, "Value");
    cairo_restore(cr);

    /* Y axis ticks: 5 ticks including 0 and max_y */
    for (int i = 0; i <= y_ticks; ++i) {
        double val = (gd->max_y * i) / (double)y_ticks;
        double y_pos = margin_top + graph_height - val * scale_y;

        cairo_move_to(cr, margin_left - 5, y_pos);
        cairo_line_to(cr, margin_left, y_pos);
        cairo_stroke(cr);

        char label[32];
        snprintf(label, sizeof(label), "%.1f", val);
        cairo_move_to(cr, margin_left - 40, y_pos + 4);
        cairo_show_text(cr, label);
    }
    
    /* Time axis label */
    cairo_move_to(cr, margin_left + graph_width / 2 - 30, height - 5);
    cairo_show_text(cr, "Time (s)");

    /* Quarter tick marks on time axis (0%,25%,50%,75%,100%) */
    for (int q = 0; q <= 4; ++q) {
        double frac = q / 4.0;
        double tval = gd->max_x * frac;
        double x_pos = margin_left + frac * graph_width;

        /* small tick */
        cairo_move_to(cr, x_pos, margin_top + graph_height);
        cairo_line_to(cr, x_pos, margin_top + graph_height + 6);
        cairo_stroke(cr);

        /* label */
        char label[32];
        snprintf(label, sizeof(label), "%.1f", tval);
        cairo_move_to(cr, x_pos - 10, margin_top + graph_height + 20);
        cairo_show_text(cr, label);
    }
    
    TraceData *trace = &gd->traces[0];

    if (g_list_length(trace->data) > 0) {

        cairo_set_source_rgba(cr, trace->color.red, trace->color.green, trace->color.blue, trace->color.alpha);
        const double line_radius = 1.0;
        const double point_radius = 4.0;

        gboolean first_point = TRUE;
        double prev_x, prev_y;

        for (GList *l = trace->data; l != NULL; l = l->next) {
            DataPoint *p = (DataPoint *)l->data;

            double screen_x = margin_left + (p->x_time * scale_x);
            double screen_y = margin_top + graph_height - (p->y_value * scale_y);
            
            if (!first_point) {
                cairo_move_to(cr, prev_x, prev_y);
                cairo_line_to(cr, screen_x, screen_y);
                cairo_set_line_width(cr, 2 * line_radius);
                cairo_stroke(cr);
            } 

            prev_x = screen_x;
            prev_y = screen_y;
            first_point = FALSE; 
        }
        
        for (GList *l = trace->data; l != NULL; l = l->next) {
            DataPoint *p = (DataPoint *)l->data;

            double screen_x = margin_left + (p->x_time * scale_x);
            double screen_y = margin_top + graph_height - (p->y_value * scale_y);

            cairo_arc(cr, screen_x, screen_y, 3.0, 0, 2 * M_PI); 
            cairo_fill(cr); 
        }
    }

    return FALSE;
}

static gboolean on_motion_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    GraphData *gd = (GraphData *)user_data;
    double mouse_x = event->x;
    double mouse_y = event->y;
    const int margin_left = 60;
    const int margin_top = 20;
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    const int margin_right = 20;
    const int margin_bottom = 40; 
    int graph_width = width - margin_left - margin_right;
    int graph_height = height - margin_top - margin_bottom;
    
    double scale_x = gd->max_x > 0 ? (double)graph_width / gd->max_x : 0.0;
    double scale_y = gd->max_y > 0 ? (double)graph_height / gd->max_y : 0.0;
    
    char status_text[256] = "Hover over data points to see values";
    gdouble closest_distance = 10.0; 

    gtk_statusbar_pop(GTK_STATUSBAR(gd->status_bar), 0);

    for (int i = 0; i < TOTAL_TRACE_COUNT; i++) {
        TraceData *trace = &gd->traces[i];
        if (!trace->visible) continue;

        for (GList *l = trace->data; l != NULL; l = l->next) {
            DataPoint *p = (DataPoint *)l->data;
            
            double screen_x = margin_left + (p->x_time * scale_x);
            double screen_y = margin_top + graph_height - (p->y_value * scale_y);
            
            double distance = sqrt(pow(mouse_x - screen_x, 2) + pow(mouse_y - screen_y, 2));

            if (distance < closest_distance) {
                closest_distance = distance;
                snprintf(status_text, sizeof(status_text), 
                            "%s: Time=%.1fs, Value=%.2f", 
                            trace->name, p->x_time, p->y_value);
                break; 
            }
        }
    }
    
    gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, status_text);

    return FALSE;
}

static GtkWidget *create_data_table(GraphData *gd) {
    GtkWidget *tree_view;
    
    gd->list_store = gtk_list_store_new(NUM_COLS, 
                                        G_TYPE_DOUBLE,
                                        G_TYPE_DOUBLE
                                        );
    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(gd->list_store));
    
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_widget_set_size_request(scrolled_window, 300, -1); 

    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_GRID_LINES_BOTH);

    const char *headers[] = {"Time (s)", "Trace 1"};
    for (int i = 0; i < NUM_COLS; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        
        g_object_set(renderer, "xalign", 0.5f, NULL); 
        
        g_object_set(renderer, "xpad", 6, NULL); 

        GtkTreeViewColumn *column = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(column, headers[i]);
        
        gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width(column, 150); 

        gtk_tree_view_column_pack_start(column, renderer, TRUE);
        gtk_tree_view_column_add_attribute(column, renderer, "text", i);
        
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    }
    
    return scrolled_window;
}

static void activate (GtkApplication* app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *main_vbox;
    GtkWidget *hbox_controls_graph;
    GtkWidget *graph_area_vbox;
    GtkWidget *graph_hbox;

    GtkCssProvider *css_provider = gtk_css_provider_new();
    GdkScreen *screen = gdk_screen_get_default();
    
    srand(time(NULL));
    
    /* primary trace */
    g_graph_data.traces[0].color = g_colors[0];
    snprintf(g_graph_data.traces[0].name, 32, "Trace 1");
    snprintf(g_graph_data.traces[0].css_class, 16, "trace-1");
    g_graph_data.traces[0].visible = TRUE;

    g_graph_data.is_recording = FALSE; 

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Interactive Logger");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 650); 

    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    graph_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), graph_hbox, TRUE, TRUE, 0);

    GtkWidget *data_table = create_data_table(&g_graph_data);
    gtk_box_pack_start(GTK_BOX(graph_hbox), data_table, FALSE, TRUE, 0);

    graph_area_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(graph_hbox), graph_area_vbox, TRUE, TRUE, 0);

    hbox_controls_graph = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(graph_area_vbox), hbox_controls_graph, FALSE, FALSE, 5);
    
    g_graph_data.start_stop_button = gtk_button_new_with_label("Stop Recording");
    g_signal_connect(g_graph_data.start_stop_button, "clicked", G_CALLBACK(on_start_stop_clicked), &g_graph_data);
    gtk_box_pack_start(GTK_BOX(hbox_controls_graph), g_graph_data.start_stop_button, FALSE, FALSE, 0);

    GtkWidget *reset_button = gtk_button_new_with_label("Reset Data");
    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_clicked), &g_graph_data);
    gtk_box_pack_start(GTK_BOX(hbox_controls_graph), reset_button, FALSE, FALSE, 0);

    GtkWidget *test_toggle = gtk_check_button_new_with_label("Test Mode");
    g_signal_connect(test_toggle, "toggled", G_CALLBACK(on_test_toggled), &g_graph_data);
    gtk_box_pack_start(GTK_BOX(hbox_controls_graph), test_toggle, FALSE, FALSE, 0);
    
    g_graph_data.drawing_area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(graph_area_vbox), g_graph_data.drawing_area, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(g_graph_data.drawing_area), "draw", G_CALLBACK(on_draw_event), &g_graph_data);
    gtk_widget_set_events(g_graph_data.drawing_area, gtk_widget_get_events(g_graph_data.drawing_area) | GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(g_graph_data.drawing_area), "motion-notify-event", G_CALLBACK(on_motion_event), &g_graph_data);
    
    g_graph_data.status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(main_vbox), g_graph_data.status_bar, FALSE, FALSE, 0);

    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(g_graph_data.status_bar), "status");
    gtk_statusbar_push(GTK_STATUSBAR(g_graph_data.status_bar), context_id, "Idle. Click Start Recording to begin or enable Test Mode to simulate data.");

    gchar *css_data = g_strdup_printf(
        ".trace-1 label { color: rgb(255, 0, 0); }"
    );

    gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
    g_free(css_data);
    
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);
    /* Try to start the network client; if it fails (returns RUN_FAILURE / (pthread_t)-1)
     * fall back to the UI timer which generates simulated data. This makes the
     * network-based data collection the default behavior when available.
     */
    
    pthread_t cli_thread = run_load_cell_cli(on_tick_timer, (void *)&g_graph_data, LC_CLI_DETACH);
    if (cli_thread == (pthread_t)-1) {
        /* network client didn't start — remain idle. Use Test Mode to simulate data when desired. */
        g_timer_source_id = 0;
    } else {
        
        /* client is running in a background thread; no UI timer used */
        g_timer_source_id = 0;
    }
    
    gtk_widget_show_all(window);
}

int main (int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new ("org.full.logger", G_APPLICATION_DEFAULT_FLAGS);
    
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    
    if (g_timer_source_id != 0) {
        g_source_remove(g_timer_source_id);
    }

    return status;
}
