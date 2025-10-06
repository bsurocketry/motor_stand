#include <gtk/gtk.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 

#define PRIMARY_TRACE_COUNT 1
#define TOTAL_TRACE_COUNT PRIMARY_TRACE_COUNT
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
} GraphData;

static GraphData g_graph_data = {0};
static guint g_timer_source_id = 0;
static gdouble g_current_time = 0.0;
static const GdkRGBA g_colors[] = {
    {1.0, 0.0, 0.0, 1.0} 
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
    gd->max_y = 150.0; 

    gtk_widget_queue_draw(gd->drawing_area);
}


// timer and random data generation
static gboolean on_tick_timer(gpointer user_data) {
    GraphData *gd = (GraphData *)user_data;
    if (!gd->is_recording) {
        return G_SOURCE_CONTINUE; 
    }
    

    double primary_val = (double)(rand() % 1000) / 10.0; 

    log_new_data(gd, primary_val);

    return G_SOURCE_CONTINUE; 
}

static void trace_toggle_cb(GtkToggleButton *button, gpointer user_data) {
    TraceData *trace = (TraceData *)user_data;
    trace->visible = gtk_toggle_button_get_active(button);
    gtk_widget_queue_draw(g_graph_data.drawing_area); 
}

static void on_start_stop_clicked(GtkButton *button, gpointer user_data) {
    GraphData *gd = (GraphData *)user_data;
    gd->is_recording = !gd->is_recording;
    
    if (gd->is_recording) {
        gtk_button_set_label(button, "Stop Recording");
        gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, "Recording data...");
    } else {
        gtk_button_set_label(button, "Start Recording");
        gtk_statusbar_push(GTK_STATUSBAR(gd->status_bar), 0, "Recording Paused. Click Reset to clear data.");
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

    for (int i = 0; i <= (int)gd->max_y; i += 50) {
        double y_pos = margin_top + graph_height - (double)i * scale_y;
        
        cairo_move_to(cr, margin_left - 5, y_pos);
        cairo_line_to(cr, margin_left, y_pos);
        cairo_stroke(cr);

        char label[10];
        snprintf(label, 10, "%d", i);
        cairo_move_to(cr, margin_left - 30, y_pos + 4);
        cairo_show_text(cr, label);
    }
    
    cairo_move_to(cr, margin_left + graph_width / 2 - 30, height - 5);
    cairo_show_text(cr, "Time (s)");

    char time_label[10];
    snprintf(time_label, 10, "%.1f", gd->max_x);
    cairo_move_to(cr, margin_left + graph_width - 15, margin_top + graph_height + 15);
    cairo_show_text(cr, time_label);
    
    for (int i = 0; i < TOTAL_TRACE_COUNT; i++) {
        TraceData *trace = &gd->traces[i];
        if (!trace->visible || g_list_length(trace->data) == 0) {
            continue;
        }

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

    const char *headers[] = {"Time (s)", "Trace 1"};
    for (int i = 0; i < NUM_COLS; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),
                                                     -1,
                                                     headers[i],
                                                     renderer,
                                                     "text", i,
                                                     NULL);
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
    
    g_graph_data.traces[0].color = g_colors[0];
    snprintf(g_graph_data.traces[0].name, 32, "Trace 1");
    snprintf(g_graph_data.traces[0].css_class, 16, "trace-1");
    g_graph_data.traces[0].visible = TRUE;

    g_graph_data.is_recording = TRUE; 

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

    for (int i = 0; i < TOTAL_TRACE_COUNT; i++) {
        TraceData *trace = &g_graph_data.traces[i];
        GtkWidget *check = gtk_check_button_new_with_label(trace->name);
        
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
        
        GtkStyleContext *context = gtk_widget_get_style_context(check);
        gtk_style_context_add_class(context, trace->css_class);

        g_signal_connect(check, "toggled", G_CALLBACK(trace_toggle_cb), trace);
        gtk_box_pack_start(GTK_BOX(hbox_controls_graph), check, FALSE, FALSE, 0);
        trace->check_button = check;
    }
    
    g_graph_data.drawing_area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(graph_area_vbox), g_graph_data.drawing_area, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(g_graph_data.drawing_area), "draw", G_CALLBACK(on_draw_event), &g_graph_data);
    gtk_widget_set_events(g_graph_data.drawing_area, gtk_widget_get_events(g_graph_data.drawing_area) | GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(g_graph_data.drawing_area), "motion-notify-event", G_CALLBACK(on_motion_event), &g_graph_data);
    
    g_graph_data.status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(main_vbox), g_graph_data.status_bar, FALSE, FALSE, 0);

    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(g_graph_data.status_bar), "status");
    gtk_statusbar_push(GTK_STATUSBAR(g_graph_data.status_bar), context_id, "Recording data...");

    gchar *css_data = g_strdup_printf(
        ".trace-1 label { color: rgb(255, 0, 0); }"
    );

    gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
    g_free(css_data);
    
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);


    g_timer_source_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, on_tick_timer, &g_graph_data, NULL);

    
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
//gcc -o test teststandui.c $(pkg-config --cflags --libs gtk+-3.0)
