#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>
#include <NetworkManager.h>
#include <cstring>
#include "translations.h"

/* Declare resource functions */
extern "C" {
    GResource *resources_get_resource (void);
}

/* ---------- App state ---------- */
typedef struct {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *page_indicators;
    GtkWidget *content_stack;
    GtkWidget *navigation_box;
    GtkWidget *back_arrow;
    GtkWidget *next_arrow;
    GtkWidget *skip_button;

    // Wi-Fi page widgets
    GtkWidget *wifi_switch;
    GtkWidget *wifi_list_box;
    GtkWidget *wifi_refresh_btn;

    NMClient  *nm_client;
    int        current_page;
    gchar     *selected_theme;
    GPtrArray *page_dots;
    
    // Add flag to prevent recursive calls
    gboolean   updating_wifi_switch;
    
    // Theme state
    gboolean   is_dark_theme;
    GtkCssProvider *theme_provider;
    gchar     *current_gtk_theme;
    guint      theme_check_id;
    
    // Network state tracking
    gboolean   networking_enabled;
    gboolean   has_ethernet_connection;
    guint      update_timeout_id;
} WelcomeApp;

/* ---------- Forward declarations ---------- */
/* navigation / pages */
static void update_page_indicators(WelcomeApp *app);
static void update_navigation(WelcomeApp *app);
static GtkWidget* create_welcome_page(WelcomeApp *app);
static GtkWidget* create_theme_page(WelcomeApp *app);
static GtkWidget* create_network_page(WelcomeApp *app);
static GtkWidget* create_keybinds_page(void);
static GtkWidget* create_updater_page(void);
static GtkWidget* create_settings_page(void);
static GtkWidget* create_store_page(void);
static GtkWidget* create_complete_page(WelcomeApp *app);
static void setup_css(WelcomeApp *app);
static void update_theme_css(WelcomeApp *app);
static void update_logo_images(WelcomeApp *app);
static void on_back_clicked(GtkButton *button, WelcomeApp *app);
static void on_next_clicked(GtkButton *button, WelcomeApp *app);
static void on_skip_clicked(GtkButton *button, WelcomeApp *app);
static void on_theme_selected(GtkButton *button, WelcomeApp *app);
static void on_finish_clicked(GtkButton *button, WelcomeApp *app);

/* Wi-Fi helpers */
static NMDeviceWifi* get_primary_wifi_device(NMClient *client);
static gchar* ssid_from_bytes(GBytes *ssid_bytes);
static gboolean ap_is_secured(NMAccessPoint *ap);
static NMRemoteConnection* find_saved_connection_for_ssid(NMClient *client, const gchar *ssid);
static gboolean ssid_equal(const gchar *a, const gchar *b);

static void scan_wifi_networks(WelcomeApp *app);
static void populate_wifi_list_now(WelcomeApp *app);
static gboolean populate_wifi_list_timeout(gpointer user_data);
static void on_wifi_refresh_clicked(GtkButton *button, WelcomeApp *app);
static gboolean on_wifi_switch_state_set(GtkSwitch *sw, gboolean state, WelcomeApp *app);
static void reflect_wifi_switch_state(WelcomeApp *app);

/* Network state helpers */
static gboolean check_networking_enabled(NMClient *client);
static gboolean check_ethernet_connection(NMClient *client);
static void update_network_state(WelcomeApp *app);
static gboolean update_network_state_timeout(gpointer user_data);
static void enable_networking(WelcomeApp *app);
static void on_enable_networking_clicked(GtkButton *button, WelcomeApp *app);

/* Theme helpers */
static gchar* get_current_gtk_theme(void);
static void detect_and_apply_theme(WelcomeApp *app);
static gboolean check_theme_changes(WelcomeApp *app);
static gboolean theme_check_timeout(gpointer user_data);

static void on_wifi_connect_clicked(GtkButton *button, gpointer user_data);
static void on_connect_button_clicked(GtkButton *button, gpointer user_data);
static void on_password_dialog_destroy(GtkWidget *dialog, gpointer user_data);
static void add_and_activate_psk(NMClient *client, NMDeviceWifi *wifi_dev, NMAccessPoint *ap, const gchar *ssid, const gchar *psk);
static void activate_saved_connection(NMClient *client, NMRemoteConnection *conn, NMDevice *dev);

static void on_nm_notify_wireless_enabled(GObject *gobj, GParamSpec *pspec, gpointer user_data);
static void on_nm_client_changed(NMClient *client, gpointer user_data);
static void on_wifi_device_props_changed(GObject *gobj, GParamSpec *pspec, gpointer user_data);

/* lifecycle */
static void on_window_destroy(GtkWidget *w, gpointer user_data);
static void activate(GtkApplication *app_gtk, gpointer user_data);

/* ---------- Small UI helpers ---------- */
static GtkWidget* make_icon_image(const char *icon_name, int pixel_size) {
    GtkWidget *img = gtk_image_new_from_icon_name(icon_name);
    if (pixel_size > 0) gtk_image_set_pixel_size(GTK_IMAGE(img), pixel_size);
    return img;
}
static GtkWidget* make_icon_button(const char *icon_name, int pixel_size) {
    GtkWidget *btn = gtk_button_new();
    GtkWidget *img = make_icon_image(icon_name, pixel_size);
    gtk_button_set_child(GTK_BUTTON(btn), img);
    return btn;
}

static GtkWidget* make_resource_image(const char *resource_path, int pixel_size) {
    GError *error = NULL;
    GdkTexture *texture = gdk_texture_new_from_resource(resource_path);
    if (texture == NULL) {
        g_warning("Failed to load resource %s: %s", resource_path, error->message);
        g_error_free(error);
        return make_icon_image("image-missing", pixel_size);
    }
    
    GtkWidget *image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
    if (pixel_size > 0) {
        gtk_image_set_pixel_size(GTK_IMAGE(image), pixel_size);
    }
    // For theme images, we want them to scale to fill the available space
    gtk_widget_set_hexpand(GTK_WIDGET(image), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(image), TRUE);
    gtk_widget_set_halign(GTK_WIDGET(image), GTK_ALIGN_CENTER);
    gtk_widget_set_valign(GTK_WIDGET(image), GTK_ALIGN_CENTER);
    g_object_unref(texture);
    return image;
}

static GtkWidget* create_theme_button(const char *resource_path, const char *label_text, gboolean is_selected) {
    // Create a button with a much smaller fixed size
    GtkWidget *button = gtk_button_new();
    gtk_widget_set_size_request(button, 150, 100);  // Much smaller: 150x100
    gtk_widget_add_css_class(button, "theme-card");
    if (is_selected) {
        gtk_widget_add_css_class(button, "theme-selected");
    }
    
    // Create an overlay to stack the image and label
    GtkWidget *overlay = gtk_overlay_new();
    
    // Create the background image
    GError *error = NULL;
    GdkTexture *texture = gdk_texture_new_from_resource(resource_path);
    if (texture == NULL) {
        g_warning("Failed to load resource %s: %s", resource_path, error->message);
        g_error_free(error);
        GtkWidget *placeholder = make_icon_image("image-missing", -1);
        gtk_overlay_set_child(GTK_OVERLAY(overlay), placeholder);
    } else {
        GtkWidget *picture = gtk_picture_new_for_paintable(GDK_PAINTABLE(texture));
        gtk_widget_set_hexpand(picture, FALSE);  // Don't expand
        gtk_widget_set_vexpand(picture, FALSE);  // Don't expand
        gtk_picture_set_content_fit(GTK_PICTURE(picture), GTK_CONTENT_FIT_COVER);
        gtk_picture_set_content_fit(GTK_PICTURE(picture), GTK_CONTENT_FIT_CONTAIN);
        gtk_widget_set_size_request(picture, 130, 70);  // Set picture size directly
        gtk_overlay_set_child(GTK_OVERLAY(overlay), picture);
        g_object_unref(texture);
    }
    
    // Create a box to hold the label
    GtkWidget *label_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(label_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label_box, GTK_ALIGN_END);
    gtk_widget_set_size_request(label_box, -1, 25);
    
    // Create the label
    GtkWidget *label = gtk_label_new(label_text);
    gtk_widget_add_css_class(label, "title-2");
    gtk_widget_add_css_class(label, "theme-label");
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 10);
    gtk_widget_set_size_request(label, -1, 20);  // Set label size directly
    gtk_box_append(GTK_BOX(label_box), label);
    
    // Add the label box as an overlay
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), label_box);
    
    // Set the overlay as the button's child
    gtk_button_set_child(GTK_BUTTON(button), overlay);
    
    return button;
}

/* ---------- Utility implementations ---------- */

static NMDeviceWifi* get_primary_wifi_device(NMClient *client) {
    if (!client) return NULL;
    const GPtrArray *devices = nm_client_get_devices(client);
    if (!devices) return NULL;
    for (guint i = 0; i < devices->len; ++i) {
        NMDevice *dev = reinterpret_cast<NMDevice*>(g_ptr_array_index(devices, i));
        if (NM_IS_DEVICE_WIFI(dev)) return NM_DEVICE_WIFI(dev);
    }
    return NULL;
}

static gchar* ssid_from_bytes(GBytes *ssid_bytes) {
    if (!ssid_bytes) return NULL;
    gsize len = 0;
    const guint8 *data = reinterpret_cast<const guint8*>(g_bytes_get_data(ssid_bytes, &len));
    if (!data || len == 0) return NULL;
    return g_strndup(reinterpret_cast<const char*>(data), len);
}

static gboolean ap_is_secured(NMAccessPoint *ap) {
    if (!ap) return FALSE;
    return (nm_access_point_get_flags(ap)     != NM_802_11_AP_FLAGS_NONE) ||
           (nm_access_point_get_wpa_flags(ap) != NM_802_11_AP_SEC_NONE)   ||
           (nm_access_point_get_rsn_flags(ap) != NM_802_11_AP_SEC_NONE);
}

static gboolean ssid_equal(const gchar *a, const gchar *b) {
    if (!a || !b) return FALSE;
    return g_strcmp0(a, b) == 0;
}

/* Search saved remote (system) connections for matching SSID
   Returns a referenced NMRemoteConnection* (or NULL). Caller must g_object_unref() */
static NMRemoteConnection* find_saved_connection_for_ssid(NMClient *client, const gchar *ssid) {
    if (!client || !ssid) return NULL;

    // iterate through nm_client_get_connections()
    const GPtrArray *conns = nm_client_get_connections(client);
    if (!conns) return NULL;

    for (guint i = 0; i < conns->len; ++i) {
        NMRemoteConnection *rc = reinterpret_cast<NMRemoteConnection*>(g_ptr_array_index(conns, i));
        if (!rc) continue;
        NMConnection *c = NM_CONNECTION(rc);
        NMSettingWireless *s_wifi = nm_connection_get_setting_wireless(c);
        if (!s_wifi) continue;
        GBytes *bytes = nm_setting_wireless_get_ssid(s_wifi);
        gchar *conn_ssid = ssid_from_bytes(bytes);
        gboolean match = ssid_equal(conn_ssid, ssid);
        g_free(conn_ssid);
        if (match) {
            // return a reference to the remote connection
            return NM_REMOTE_CONNECTION(g_object_ref(rc));
        }
    }
    return NULL;
}

/* ---------- Wi-Fi UI building ---------- */

static void build_ap_row(WelcomeApp *app, NMDeviceWifi *wifi_dev, NMAccessPoint *ap) {
    const Translations* tr = get_translations();
    
    gchar *ssid = ssid_from_bytes(nm_access_point_get_ssid(ap));
    if (!ssid) ssid = g_strdup("<hidden>");

    gboolean secured = ap_is_secured(ap);
    guint8 strength = nm_access_point_get_strength(ap);

    GtkWidget *row = gtk_list_box_row_new();
    gtk_widget_set_size_request(row, -1, 60);

    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_widget_set_margin_top(row_box, 12);
    gtk_widget_set_margin_bottom(row_box, 12);
    gtk_widget_set_margin_start(row_box, 20);
    gtk_widget_set_margin_end(row_box, 20);

    const char *icon_name =
        (strength > 75) ? "network-wireless-signal-excellent-symbolic" :
        (strength > 50) ? "network-wireless-signal-good-symbolic" :
        (strength > 25) ? "network-wireless-signal-ok-symbolic" :
                          "network-wireless-signal-weak-symbolic";
    GtkWidget *signal_icon = make_icon_image(icon_name, 20);
    gtk_box_append(GTK_BOX(row_box), signal_icon);

    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *name_label = gtk_label_new(ssid);
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(name_label, "heading");
    gtk_box_append(GTK_BOX(name_box), name_label);

    /* Status: Connected / Saved / Secured */
    NMAccessPoint *active_ap = nm_device_wifi_get_active_access_point(wifi_dev);
    gboolean is_active = FALSE;
    if (active_ap) {
        const char *p1 = nm_object_get_path(NM_OBJECT(active_ap));
        const char *p2 = nm_object_get_path(NM_OBJECT(ap));
        if (p1 && p2 && g_strcmp0(p1, p2) == 0) is_active = TRUE;
    }

    NMRemoteConnection *saved = find_saved_connection_for_ssid(app->nm_client, ssid);

    if (is_active) {
        GtkWidget *status = gtk_label_new(tr->connected_status);
        gtk_widget_add_css_class(status, "caption");
        gtk_widget_add_css_class(status, "accent");
        gtk_widget_set_halign(status, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(name_box), status);
    } else if (saved) {
        GtkWidget *status = gtk_label_new(tr->saved_status);
        gtk_widget_add_css_class(status, "caption");
        gtk_widget_set_halign(status, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(name_box), status);
    } else if (secured) {
        GtkWidget *status = gtk_label_new(tr->secured_status);
        gtk_widget_add_css_class(status, "caption");
        gtk_widget_set_halign(status, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(name_box), status);
    }

    if (saved) g_object_unref(saved);

    gtk_widget_set_hexpand(name_box, TRUE);
    gtk_box_append(GTK_BOX(row_box), name_box);

    if (secured) {
        GtkWidget *lock_icon = make_icon_image("network-wireless-encrypted-symbolic", 16);
        gtk_box_append(GTK_BOX(row_box), lock_icon);
    }

    GtkWidget *connect_btn = gtk_button_new();
    gtk_widget_add_css_class(connect_btn, "flat");
    gtk_widget_add_css_class(connect_btn, "circular");
    gtk_button_set_child(GTK_BUTTON(connect_btn), make_icon_image("go-next-symbolic", 16));

    /* attach data for handler; keep refs so AP stays valid while row exists */
    g_object_set_data_full(G_OBJECT(connect_btn), "wifi-dev", g_object_ref(wifi_dev), (GDestroyNotify)g_object_unref);
    g_object_set_data_full(G_OBJECT(connect_btn), "ap",       g_object_ref(ap),       (GDestroyNotify)g_object_unref);
    g_object_set_data_full(G_OBJECT(connect_btn), "ssid",     g_strdup(ssid),         g_free);

    g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_wifi_connect_clicked), app);

    gtk_box_append(GTK_BOX(row_box), connect_btn);

    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
    gtk_list_box_append(GTK_LIST_BOX(app->wifi_list_box), row);

    g_free(ssid);
}

/* ---------- Theme helpers ---------- */

static gchar* get_current_gtk_theme(void) {
    GtkSettings *settings = gtk_settings_get_default();
    gchar *theme = NULL;
    g_object_get(settings, "gtk-theme-name", &theme, NULL);
    return theme;
}

static void detect_and_apply_theme(WelcomeApp *app) {
    gchar *current_theme = get_current_gtk_theme();
    
    if (current_theme) {
        g_print("Current GTK theme: %s\n", current_theme);
        
        gboolean should_be_dark = FALSE;
        if (g_strcmp0(current_theme, "ElysiaOS-HoC") == 0) {
            should_be_dark = TRUE;
        } else if (g_strcmp0(current_theme, "ElysiaOS") == 0) {
            should_be_dark = FALSE;
        }
        
        g_print("Theme detection: current_theme=%s, is_dark_theme=%s, should_be_dark=%s\n", 
                current_theme, 
                app->is_dark_theme ? "true" : "false",
                should_be_dark ? "true" : "false");
        
        if (app->is_dark_theme != should_be_dark) {
            g_print("Theme changed from %s to %s\n", 
                    app->is_dark_theme ? "dark" : "light",
                    should_be_dark ? "dark" : "light");
            app->is_dark_theme = should_be_dark;
            update_theme_css(app);
        } else {
            // Even if theme hasn't changed, we still need to update logo images
            // in case they haven't been set yet
            g_print("Calling update_logo_images (no theme change)\n");
            update_logo_images(app);
        }
        
        g_free(current_theme);
    }
}

static gboolean check_theme_changes(WelcomeApp *app) {
    detect_and_apply_theme(app);
    return G_SOURCE_CONTINUE; // Continue checking
}

static gboolean theme_check_timeout(gpointer user_data) {
    WelcomeApp *app = (WelcomeApp*) user_data;
    return check_theme_changes(app);
}

static void enable_networking(WelcomeApp *app) {
    if (!app->nm_client) return;
    
    g_print("Enabling networking...\n");
    GError *error = NULL;
    if (!nm_client_networking_set_enabled(app->nm_client, TRUE, &error)) {
        g_warning("Failed to enable networking: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
    }
}

static void on_enable_networking_clicked(GtkButton *button, WelcomeApp *app) {
    (void)button;
    enable_networking(app);
}

static void populate_wifi_list_now(WelcomeApp *app) {
    const Translations* tr = get_translations();
    
    if (!app->nm_client || !app->wifi_list_box) return;

    /* Clear existing list */
    for (GtkWidget *child = gtk_widget_get_first_child(app->wifi_list_box); child; ) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(app->wifi_list_box), child);
        child = next;
    }

    /* Check if networking is enabled */
    if (!app->networking_enabled) {
        GtkWidget *row = gtk_list_box_row_new();
        
        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_top(row_box, 20);
        gtk_widget_set_margin_bottom(row_box, 20);
        gtk_widget_set_margin_start(row_box, 20);
        gtk_widget_set_margin_end(row_box, 20);
        
        GtkWidget *lbl = gtk_label_new(tr->networking_disabled_message);
        gtk_widget_add_css_class(lbl, "dim-label");
        gtk_widget_set_halign(lbl, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(row_box), lbl);
        
        GtkWidget *enable_btn = gtk_button_new_with_label(tr->enable_networking_button);
        gtk_widget_add_css_class(enable_btn, "suggested-action");
        gtk_widget_set_halign(enable_btn, GTK_ALIGN_CENTER);
        g_signal_connect(enable_btn, "clicked", G_CALLBACK(on_enable_networking_clicked), app);
        gtk_box_append(GTK_BOX(row_box), enable_btn);
        
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
        gtk_list_box_append(GTK_LIST_BOX(app->wifi_list_box), row);
        return;
    }

    /* Check if already connected via ethernet */
    if (app->has_ethernet_connection) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new(tr->ethernet_connected_message);
        gtk_widget_add_css_class(lbl, "dim-label");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), lbl);
        gtk_list_box_append(GTK_LIST_BOX(app->wifi_list_box), row);
        return;
    }

    /* Check if Wi-Fi is enabled */
    gboolean hw_enabled = nm_client_wireless_hardware_get_enabled(app->nm_client);
    gboolean sw_enabled = nm_client_wireless_get_enabled(app->nm_client);
    
    if (!hw_enabled) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new(tr->wifi_hardware_disabled_message);
        gtk_widget_add_css_class(lbl, "dim-label");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), lbl);
        gtk_list_box_append(GTK_LIST_BOX(app->wifi_list_box), row);
        return;
    }
    
    if (!sw_enabled) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new(tr->wifi_disabled_message);
        gtk_widget_add_css_class(lbl, "dim-label");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), lbl);
        gtk_list_box_append(GTK_LIST_BOX(app->wifi_list_box), row);
        return;
    }

    NMDeviceWifi *wifi = get_primary_wifi_device(app->nm_client);
    if (!wifi) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new(tr->no_wifi_device_message);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), lbl);
        gtk_list_box_append(GTK_LIST_BOX(app->wifi_list_box), row);
        return;
    }

    const GPtrArray *aps = nm_device_wifi_get_access_points(wifi);
    if (!aps || aps->len == 0) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new(tr->no_networks_found_message);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), lbl);
        gtk_list_box_append(GTK_LIST_BOX(app->wifi_list_box), row);
        return;
    }

    g_print("Found %d Wi-Fi networks\n", aps->len);
    for (guint j = 0; j < aps->len; ++j) {
        NMAccessPoint *ap = reinterpret_cast<NMAccessPoint*>(g_ptr_array_index(aps, j));
        build_ap_row(app, wifi, ap);
    }
}

static gboolean populate_wifi_list_timeout(gpointer user_data) {
    WelcomeApp *app = (WelcomeApp*) user_data;
    populate_wifi_list_now(app);
    return G_SOURCE_REMOVE;
}

static void scan_wifi_networks(WelcomeApp *app) {
    if (!app->nm_client) return;
    NMDeviceWifi *wifi = get_primary_wifi_device(app->nm_client);
    if (!wifi) return;
    nm_device_wifi_request_scan_async(wifi, NULL, NULL, NULL);
    g_timeout_add(2000, populate_wifi_list_timeout, app);
}

static void on_wifi_refresh_clicked(GtkButton *button, WelcomeApp *app) {
    (void)button;
    scan_wifi_networks(app);
}

/* reflect wifi-enabled/hardware state into the switch widget */
static void reflect_wifi_switch_state(WelcomeApp *app) {
    if (!app->nm_client || !app->wifi_switch || app->updating_wifi_switch) return;
    
    app->updating_wifi_switch = TRUE;
    
    gboolean hw_enabled = nm_client_wireless_hardware_get_enabled(app->nm_client);
    gboolean sw_enabled = nm_client_wireless_get_enabled(app->nm_client);
    
    g_print("Wi-Fi Hardware enabled: %s, Software enabled: %s, Networking enabled: %s\n", 
            hw_enabled ? "YES" : "NO", sw_enabled ? "YES" : "NO", 
            app->networking_enabled ? "YES" : "NO");
    
    /* Set switch state to reflect software Wi-Fi state only if hardware is enabled and networking is enabled */
    if (hw_enabled && app->networking_enabled) {
        gtk_switch_set_active(GTK_SWITCH(app->wifi_switch), sw_enabled);
        gtk_widget_set_sensitive(app->wifi_switch, TRUE);
    } else {
        /* Hardware disabled or networking disabled - switch should be off and disabled */
        gtk_switch_set_active(GTK_SWITCH(app->wifi_switch), FALSE);
        gtk_widget_set_sensitive(app->wifi_switch, FALSE);
    }
    
    app->updating_wifi_switch = FALSE;
    
    /* Update the refresh button sensitivity based on Wi-Fi state */
    if (app->wifi_refresh_btn) {
        gtk_widget_set_sensitive(app->wifi_refresh_btn, hw_enabled && sw_enabled && app->networking_enabled);
    }
}
/* handler for user toggling the switch in UI
   returns TRUE to stop default handler (we manually reflect the state) */
static gboolean on_wifi_switch_state_set(GtkSwitch *sw, gboolean state, WelcomeApp *app) {
    if (!app->nm_client || app->updating_wifi_switch) return TRUE;

    gboolean hw_enabled = nm_client_wireless_hardware_get_enabled(app->nm_client);
    gboolean current_sw_enabled = nm_client_wireless_get_enabled(app->nm_client);
    
    g_print("Switch toggle requested: %s (HW: %s, Current SW: %s, Networking: %s)\n", 
            state ? "ON" : "OFF", 
            hw_enabled ? "enabled" : "disabled",
            current_sw_enabled ? "enabled" : "disabled",
            app->networking_enabled ? "enabled" : "disabled");
    
    if (!hw_enabled) {
        g_print("Cannot enable Wi-Fi: Hardware is disabled\n");
        /* Hardware is disabled, prevent any state change */
        reflect_wifi_switch_state(app);
        return TRUE;
    }
    
    if (!app->networking_enabled) {
        g_print("Cannot enable Wi-Fi: Networking is disabled\n");
        /* Networking is disabled, prevent any state change */
        reflect_wifi_switch_state(app);
        return TRUE;
    }
    
    /* Only proceed if the requested state is different from current state */
    if (state != current_sw_enabled) {
        g_print("Setting Wi-Fi software state to: %s\n", state ? "enabled" : "disabled");
        
        /* Set software Wi-Fi state */
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        nm_client_wireless_set_enabled(app->nm_client, state);
        #pragma GCC diagnostic pop
        
        /* Give some time for the change to take effect before updating UI */
        if (state) {
            /* Enabling Wi-Fi - scan after a delay */
            g_timeout_add(1000, (GSourceFunc)scan_wifi_networks, app);
        } else {
            /* Disabling Wi-Fi - update list immediately */
            g_timeout_add(500, (GSourceFunc)populate_wifi_list_now, app);
        }
        
        /* Reflect the state change after a short delay */
        g_timeout_add(300, (GSourceFunc)reflect_wifi_switch_state, app);
    }

    return TRUE; /* Prevent default toggle handling */
}

/* NM signals - made more robust to prevent recursive calls */
/* ---------- Network state helpers ---------- */

static gboolean check_networking_enabled(NMClient *client) {
    if (!client) return FALSE;
    return nm_client_networking_get_enabled(client);
}

static gboolean check_ethernet_connection(NMClient *client) {
    if (!client) return FALSE;
    
    const GPtrArray *devices = nm_client_get_devices(client);
    if (!devices) return FALSE;
    
    for (guint i = 0; i < devices->len; ++i) {
        NMDevice *dev = reinterpret_cast<NMDevice*>(g_ptr_array_index(devices, i));
        if (NM_IS_DEVICE_ETHERNET(dev)) {
            NMDeviceState state = nm_device_get_state(dev);
            if (state == NM_DEVICE_STATE_ACTIVATED) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void update_network_state(WelcomeApp *app) {
    if (!app->nm_client) return;
    
    gboolean new_networking_enabled = check_networking_enabled(app->nm_client);
    gboolean new_ethernet_connection = check_ethernet_connection(app->nm_client);
    
    gboolean state_changed = (app->networking_enabled != new_networking_enabled) ||
                            (app->has_ethernet_connection != new_ethernet_connection);
    
    app->networking_enabled = new_networking_enabled;
    app->has_ethernet_connection = new_ethernet_connection;
    
    if (state_changed) {
        g_print("Network state changed: networking=%s, ethernet=%s\n", 
                new_networking_enabled ? "enabled" : "disabled",
                new_ethernet_connection ? "connected" : "disconnected");
        
        // Update UI with debounced timeout
        if (app->update_timeout_id > 0) {
            g_source_remove(app->update_timeout_id);
        }
        app->update_timeout_id = g_timeout_add(300, update_network_state_timeout, app);
    }
}

static gboolean update_network_state_timeout(gpointer user_data) {
    WelcomeApp *app = (WelcomeApp*) user_data;
    app->update_timeout_id = 0;
    
    if (!app->updating_wifi_switch) {
        reflect_wifi_switch_state(app);
        populate_wifi_list_now(app);
    }
    
    return G_SOURCE_REMOVE;
}

static void on_nm_notify_wireless_enabled(GObject *gobj, GParamSpec *pspec, gpointer user_data) {
    WelcomeApp *app = (WelcomeApp*) user_data;
    update_network_state(app);
}

static void on_nm_client_changed(NMClient *client, gpointer user_data) {
    WelcomeApp *app = (WelcomeApp*) user_data;
    update_network_state(app);
}

static void on_wifi_device_props_changed(GObject *gobj, GParamSpec *pspec, gpointer user_data) {
    (void)gobj; (void)pspec;
    WelcomeApp *app = (WelcomeApp*) user_data;
    update_network_state(app);
}

/* ---------- Connect flow ---------- */

/* Activate an already-saved connection (async) */
static void activate_saved_connection(NMClient *client, NMRemoteConnection *conn, NMDevice *dev) {
    if (!client || !conn || !dev) return;
    nm_client_activate_connection_async(client, NM_CONNECTION(conn), dev, NULL, NULL, NULL, NULL);
}

/* Build a WPA-PSK connection and add+activate it */
static void add_and_activate_psk(NMClient *client, NMDeviceWifi *wifi_dev, NMAccessPoint *ap, const gchar *ssid, const gchar *psk) {
    if (!client || !wifi_dev || !ap || !ssid || !psk) return;

    NMConnection *c = nm_simple_connection_new();

    /* Connection setting */
    NMSettingConnection *s_con = NM_SETTING_CONNECTION(nm_setting_connection_new());
    g_object_set(G_OBJECT(s_con),
                 NM_SETTING_CONNECTION_ID, ssid,
                 NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                 NM_SETTING_CONNECTION_AUTOCONNECT, TRUE,
                 NULL);
    nm_connection_add_setting(c, NM_SETTING(s_con));

    /* Wireless setting */
    NMSettingWireless *s_wifi = NM_SETTING_WIRELESS(nm_setting_wireless_new());
    GBytes *ssid_bytes = g_bytes_new(ssid, strlen(ssid));
    g_object_set(G_OBJECT(s_wifi),
                 NM_SETTING_WIRELESS_SSID, ssid_bytes,
                 NM_SETTING_WIRELESS_MODE, "infrastructure",
                 NULL);
    g_bytes_unref(ssid_bytes);
    nm_connection_add_setting(c, NM_SETTING(s_wifi));

    /* Security (WPA-PSK) */
    NMSettingWirelessSecurity *s_wsec = NM_SETTING_WIRELESS_SECURITY(nm_setting_wireless_security_new());
    g_object_set(G_OBJECT(s_wsec),
                 NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
                 NM_SETTING_WIRELESS_SECURITY_PSK, psk,
                 NULL);
    nm_connection_add_setting(c, NM_SETTING(s_wsec));

    /* AP object path string as specific_object */
    const char *ap_path = nm_object_get_path(NM_OBJECT(ap));

    nm_client_add_and_activate_connection_async(
        client,
        c,
        NM_DEVICE(wifi_dev),
        ap_path,
        NULL, NULL, NULL);
}

/* Dialog data structure */
typedef struct {
    WelcomeApp *app;
    NMDeviceWifi *wifi_dev;
    NMAccessPoint *ap;
    gchar *ssid;
    GtkWidget *entry;
    GtkWidget *dialog;
} DialogData;

/* New callback functions for modern GTK4 dialog */
static void on_connect_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    DialogData *d = (DialogData*) user_data;
    if (d && d->ssid && d->entry && d->app && d->app->nm_client) {
        const gchar *psk = gtk_editable_get_text(GTK_EDITABLE(d->entry));
        if (psk && *psk) {
            add_and_activate_psk(d->app->nm_client, d->wifi_dev, d->ap, d->ssid, psk);
        }
    }
    
    if (d && d->dialog) {
        gtk_window_destroy(GTK_WINDOW(d->dialog));
    }
}

static void on_password_dialog_destroy(GtkWidget *dialog, gpointer user_data) {
    (void)dialog;
    DialogData *d = (DialogData*) user_data;
    if (d) {
        if (d->ap) g_object_unref(d->ap);
        if (d->wifi_dev) g_object_unref(d->wifi_dev);
        g_free(d->ssid);
        g_free(d);
    }
}

/* When user clicks connect on a row */
static void on_wifi_connect_clicked(GtkButton *button, gpointer user_data) {
    const Translations* tr = get_translations();
    
    WelcomeApp *app = (WelcomeApp*) user_data;
    if (!app || !app->nm_client) return;

    NMDeviceWifi *wifi_dev = reinterpret_cast<NMDeviceWifi*>(g_object_get_data(G_OBJECT(button), "wifi-dev"));
    NMAccessPoint *ap      = reinterpret_cast<NMAccessPoint*>(g_object_get_data(G_OBJECT(button), "ap"));
    const gchar   *ssid    = reinterpret_cast<const gchar*>(g_object_get_data(G_OBJECT(button), "ssid"));

    if (!wifi_dev || !ap || !ssid) return;

    /* If saved connection exists — activate it */
    NMRemoteConnection *saved = find_saved_connection_for_ssid(app->nm_client, ssid);
    if (saved) {
        activate_saved_connection(app->nm_client, saved, NM_DEVICE(wifi_dev));
        g_object_unref(saved);
        return;
    }

    /* If open network — add & activate immediately (no password) */
    if (!ap_is_secured(ap)) {
        NMConnection *c = nm_simple_connection_new();

        NMSettingConnection *s_con = NM_SETTING_CONNECTION(nm_setting_connection_new());
        g_object_set(G_OBJECT(s_con),
                     NM_SETTING_CONNECTION_ID, ssid,
                     NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                     NM_SETTING_CONNECTION_AUTOCONNECT, TRUE,
                     NULL);
        nm_connection_add_setting(c, NM_SETTING(s_con));

        NMSettingWireless *s_wifi = NM_SETTING_WIRELESS(nm_setting_wireless_new());
        GBytes *ssid_bytes = g_bytes_new(ssid, strlen(ssid));
        g_object_set(G_OBJECT(s_wifi),
                     NM_SETTING_WIRELESS_SSID, ssid_bytes,
                     NM_SETTING_WIRELESS_MODE, "infrastructure",
                     NULL);
        g_bytes_unref(ssid_bytes);
        nm_connection_add_setting(c, NM_SETTING(s_wifi));

        const char *ap_path = nm_object_get_path(NM_OBJECT(ap));
        nm_client_add_and_activate_connection_async(app->nm_client, c, NM_DEVICE(wifi_dev), ap_path, NULL, NULL, NULL);
        return;
    }

    /* Otherwise secured & not saved: prompt for PSK */
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), tr->password_dialog_title);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button))));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 200);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);
    gtk_widget_set_margin_start(main_box, 20);
    gtk_widget_set_margin_end(main_box, 20);

    gchar *prompt = g_strdup_printf(tr->password_dialog_prompt, ssid);
    GtkWidget *label = gtk_label_new(prompt);
    g_free(prompt);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(entry), GTK_INPUT_PURPOSE_PASSWORD);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label(tr->cancel_button);
    GtkWidget *connect_btn = gtk_button_new_with_label(tr->connect_button);
    gtk_widget_add_css_class(connect_btn, "suggested-action");

    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    gtk_box_append(GTK_BOX(button_box), connect_btn);

    gtk_box_append(GTK_BOX(main_box), label);
    gtk_box_append(GTK_BOX(main_box), entry);
    gtk_box_append(GTK_BOX(main_box), button_box);

    gtk_window_set_child(GTK_WINDOW(dialog), main_box);

    /* Package data for dialog response handler */
    DialogData *d = (DialogData*)g_malloc0(sizeof(DialogData));
    d->app = app;
    d->wifi_dev = NM_DEVICE_WIFI(g_object_ref(wifi_dev));
    d->ap = NM_ACCESS_POINT(g_object_ref(ap));
    d->ssid = g_strdup(ssid);
    d->entry = entry;
    d->dialog = dialog;

    /* Connect button handlers */
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_connect_button_clicked), d);
    g_signal_connect(dialog, "destroy", G_CALLBACK(on_password_dialog_destroy), d);

    gtk_window_present(GTK_WINDOW(dialog));
}

/* ---------- External link handler ---------- */
static void open_url(const gchar *url) {
    gchar *command = g_strdup_printf("xdg-open '%s'", url);
    GError *error = NULL;
    if (!g_spawn_command_line_async(command, &error)) {
        g_warning("Failed to open URL %s: %s", url, error->message);
        g_error_free(error);
    }
    g_free(command);
}

static void on_support_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    open_url("https://ko-fi.com/matsuko3");
}

static void on_discord_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    open_url("https://discord.gg/tbRy63xdWD");
}

static void on_website_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    open_url("https://www.elysiaos.live/");
}

/* ---------- Pages: welcome / theme / network / complete ---------- */

static GtkWidget* create_welcome_page(WelcomeApp *app) {
    const Translations* tr = get_translations();
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_vexpand(box, TRUE);

    GtkWidget *welcome_label = gtk_label_new(tr->welcome_title);
    gtk_widget_add_css_class(welcome_label, "display-1");
    gtk_widget_add_css_class(welcome_label, "accent");
    gtk_widget_set_halign(welcome_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), welcome_label);

    const char *logo_path = app->is_dark_theme ? 
        "/org/elysiaos/welcome/elyoslogo1.png" : 
        "/org/elysiaos/welcome/elyoslogo2.png";
    GtkWidget *logo = make_resource_image(logo_path, 200);
    gtk_widget_set_halign(logo, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(logo, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), logo);

    return box;
}

static void on_theme_selected(GtkButton *button, WelcomeApp *app) {
    const gchar *theme_script = (const gchar*) g_object_get_data(G_OBJECT(button), "theme-script");
    const gchar *theme_name = (const gchar*) g_object_get_data(G_OBJECT(button), "theme-name");
    if (!theme_script) return;

    // Find the parent container and update selection styling
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
    if (parent) {
        for (GtkWidget *sib = gtk_widget_get_first_child(parent); sib; sib = gtk_widget_get_next_sibling(sib)) {
            if (GTK_IS_BUTTON(sib)) gtk_widget_remove_css_class(sib, "theme-selected");
        }
    }

    gtk_widget_add_css_class(GTK_WIDGET(button), "theme-selected");

    /* Update app theme state */
    if (theme_name && g_strcmp0(theme_name, "dark") == 0) {
        app->is_dark_theme = TRUE;
    } else {
        app->is_dark_theme = FALSE;
    }
    
    update_theme_css(app);

    /* Execute the theme script */
    gchar *command = g_strdup_printf("bash %s", theme_script);
    GError *error = NULL;
    if (!g_spawn_command_line_async(command, &error)) {
        g_warning("Failed to execute theme script %s: %s", theme_script, error->message);
        g_error_free(error);
    }
    g_free(command);

    g_print("Applied theme script: %s\n", theme_script);
}

static GtkWidget* create_theme_page(WelcomeApp *app) {
    const Translations* tr = get_translations();
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(main_box, TRUE);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(title_box, GTK_ALIGN_CENTER);

    GtkWidget *title = gtk_label_new(tr->theme_title);
    gtk_widget_add_css_class(title, "display-2");
    gtk_box_append(GTK_BOX(title_box), title);

    GtkWidget *subtitle = gtk_label_new(tr->theme_subtitle);
    gtk_widget_add_css_class(subtitle, "title-3");
    gtk_widget_add_css_class(subtitle, "dim-label");
    gtk_box_append(GTK_BOX(title_box), subtitle);

    gtk_box_append(GTK_BOX(main_box), title_box);

    GtkWidget *buttons_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(buttons_box, GTK_ALIGN_CENTER);

    /* Light theme button */
    GtkWidget *light_button = create_theme_button("/org/elysiaos/welcome/light.png", "Light", TRUE);
    gtk_widget_set_size_request(light_button, 180, 120);  // Explicitly set size
    const gchar *home_dir = g_get_home_dir();
    gchar *light_script = g_build_filename(home_dir, ".config", "Elysia", "LightTheme.sh", NULL);
    g_object_set_data_full(G_OBJECT(light_button), "theme-script", light_script, g_free);
    g_object_set_data(G_OBJECT(light_button), "theme-name", (gpointer)"light");
    g_signal_connect(light_button, "clicked", G_CALLBACK(on_theme_selected), app);
    gtk_box_append(GTK_BOX(buttons_box), light_button);

    /* Dark theme button */
    GtkWidget *dark_button = create_theme_button("/org/elysiaos/welcome/dark.png", "Dark", FALSE);
    gtk_widget_set_size_request(dark_button, 180, 120);  // Explicitly set size
    gchar *dark_script = g_build_filename(home_dir, ".config", "Elysia", "DarkTheme.sh", NULL);
    g_object_set_data_full(G_OBJECT(dark_button), "theme-script", dark_script, g_free);
    g_object_set_data(G_OBJECT(dark_button), "theme-name", (gpointer)"dark");
    g_signal_connect(dark_button, "clicked", G_CALLBACK(on_theme_selected), app);
    gtk_box_append(GTK_BOX(buttons_box), dark_button);

    gtk_box_append(GTK_BOX(main_box), buttons_box);

    return main_box;
}

static GtkWidget* create_network_page(WelcomeApp *app) {
    const Translations* tr = get_translations();
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(main_box, 30);
    gtk_widget_set_margin_bottom(main_box, 30);
    gtk_widget_set_margin_start(main_box, 40);
    gtk_widget_set_margin_end(main_box, 40);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(title_box, GTK_ALIGN_CENTER);

    GtkWidget *title = gtk_label_new(tr->network_title);
    gtk_widget_add_css_class(title, "display-2");
    gtk_box_append(GTK_BOX(title_box), title);

    GtkWidget *subtitle = gtk_label_new(tr->network_subtitle);
    gtk_widget_add_css_class(subtitle, "title-3");
    gtk_widget_add_css_class(subtitle, "dim-label");
    gtk_box_append(GTK_BOX(title_box), subtitle);

    gtk_box_append(GTK_BOX(main_box), title_box);

    GtkWidget *wifi_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_widget_set_halign(wifi_header, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(wifi_header, 20);

    GtkWidget *wifi_icon = make_icon_image("network-wireless-symbolic", 20);
    gtk_box_append(GTK_BOX(wifi_header), wifi_icon);

    GtkWidget *wifi_label = gtk_label_new(tr->wifi_label);
    gtk_widget_add_css_class(wifi_label, "title-3");
    gtk_box_append(GTK_BOX(wifi_header), wifi_label);

    app->wifi_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->wifi_switch), TRUE);
    gtk_box_append(GTK_BOX(wifi_header), app->wifi_switch);

    app->wifi_refresh_btn = make_icon_button("view-refresh-symbolic", 16);
    gtk_widget_add_css_class(app->wifi_refresh_btn, "flat");
    gtk_widget_add_css_class(app->wifi_refresh_btn, "circular");
    gtk_widget_set_tooltip_text(app->wifi_refresh_btn, tr->refresh_button_tooltip);
    g_signal_connect(app->wifi_refresh_btn, "clicked", G_CALLBACK(on_wifi_refresh_clicked), app);
    gtk_box_append(GTK_BOX(wifi_header), app->wifi_refresh_btn);

    gtk_box_append(GTK_BOX(main_box), wifi_header);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scrolled, 600, 320);
    gtk_widget_set_halign(scrolled, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    app->wifi_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(app->wifi_list_box, "wifi-list");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), app->wifi_list_box);
    gtk_box_append(GTK_BOX(main_box), scrolled);

    app->nm_client = nm_client_new(NULL, NULL);
    if (app->nm_client) {
        g_print("NetworkManager client initialized successfully\n");
        
        /* Initialize network state */
        app->networking_enabled = check_networking_enabled(app->nm_client);
        app->has_ethernet_connection = check_ethernet_connection(app->nm_client);
        app->update_timeout_id = 0;
        
        g_print("Initial network state: networking=%s, ethernet=%s\n", 
                app->networking_enabled ? "enabled" : "disabled",
                app->has_ethernet_connection ? "connected" : "disconnected");
        
        /* Connect to NetworkManager state change signals */
        g_signal_connect(app->nm_client, "notify::wireless-enabled", 
                        G_CALLBACK(on_nm_notify_wireless_enabled), app);
        g_signal_connect(app->nm_client, "notify::wireless-hardware-enabled", 
                        G_CALLBACK(on_nm_notify_wireless_enabled), app);
        g_signal_connect(app->nm_client, "notify::networking-enabled", 
                        G_CALLBACK(on_nm_client_changed), app);
        g_signal_connect(app->nm_client, "changed", 
                        G_CALLBACK(on_nm_client_changed), app);

        /* Connect to all device signals for state changes */
        const GPtrArray *devices = nm_client_get_devices(app->nm_client);
        if (devices) {
            for (guint i = 0; i < devices->len; ++i) {
                NMDevice *dev = reinterpret_cast<NMDevice*>(g_ptr_array_index(devices, i));
                if (dev) {
                    g_signal_connect(dev, "notify::state", 
                                   G_CALLBACK(on_wifi_device_props_changed), app);
                }
            }
        }

        /* Connect to Wi-Fi device signals */
        NMDeviceWifi *wifi = get_primary_wifi_device(app->nm_client);
        if (wifi) {
            g_print("Found Wi-Fi device: %s\n", nm_device_get_iface(NM_DEVICE(wifi)));
            g_signal_connect(wifi, "notify::active-access-point", 
                           G_CALLBACK(on_wifi_device_props_changed), app);
            g_signal_connect(wifi, "notify::access-points", 
                           G_CALLBACK(on_wifi_device_props_changed), app);
        } else {
            g_print("No Wi-Fi device found\n");
        }

        /* Initial state setup */
        reflect_wifi_switch_state(app);
        populate_wifi_list_now(app);
        
        /* Auto-enable networking if disabled */
        if (!app->networking_enabled) {
            g_print("Networking is disabled, auto-enabling...\n");
            enable_networking(app);
        }
        
        /* Only scan if Wi-Fi is enabled and networking is enabled */
        gboolean hw_enabled = nm_client_wireless_hardware_get_enabled(app->nm_client);
        gboolean sw_enabled = nm_client_wireless_get_enabled(app->nm_client);
        if (hw_enabled && sw_enabled && app->networking_enabled) {
            scan_wifi_networks(app);
        }
    } else {
        g_print("Failed to initialize NetworkManager client\n");
        
        /* Show error in the list */
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new(tr->nm_not_available_message);
        gtk_widget_add_css_class(lbl, "error-label");
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), lbl);
        gtk_list_box_append(GTK_LIST_BOX(app->wifi_list_box), row);
        
        /* Disable Wi-Fi controls */
        gtk_widget_set_sensitive(app->wifi_switch, FALSE);
        gtk_widget_set_sensitive(app->wifi_refresh_btn, FALSE);
    }

    g_signal_connect(app->wifi_switch, "state-set", G_CALLBACK(on_wifi_switch_state_set), app);

    return main_box;
}

static GtkWidget* create_keybinds_page(void) {
    const Translations* tr = get_translations();
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);
    gtk_widget_set_margin_start(main_box, 40);
    gtk_widget_set_margin_end(main_box, 40);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(title_box, GTK_ALIGN_CENTER);

    GtkWidget *title = gtk_label_new(tr->keybinds_title);
    gtk_widget_add_css_class(title, "display-2");
    gtk_box_append(GTK_BOX(title_box), title);

    GtkWidget *subtitle = gtk_label_new(tr->keybinds_subtitle);
    gtk_widget_add_css_class(subtitle, "title-3");
    gtk_widget_add_css_class(subtitle, "dim-label");
    gtk_box_append(GTK_BOX(title_box), subtitle);

    gtk_box_append(GTK_BOX(main_box), title_box);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scrolled, 600, 350);
    gtk_widget_set_halign(scrolled, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_add_css_class(scrolled, "scrolled-window");

    GtkWidget *keybinds_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(keybinds_grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(keybinds_grid), 12);
    gtk_widget_set_halign(keybinds_grid, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(keybinds_grid, "keybinds-grid");

    // Keybind data with translated descriptions
    const struct {
        const char *shortcut;
        const char *description_key; // Key to look up translation
    } keybinds[] = {
        {"SUPER + Q", tr->keybind_close_window},
        {"SUPER + SPACE", tr->keybind_app_manager},
        {"SUPER + T", tr->keybind_terminal},
        {"ALT + TAB", tr->keybind_workspace_switcher},
        {"CTRL + SPACE", tr->keybind_change_language},
        {"SUPER + L", tr->keybind_lock_screen},
        {"SUPER + M", tr->keybind_powermenu},
        {"SUPER + [0-9]", tr->keybind_switch_workspaces},
        {"SUPER + SHIFT + S", tr->keybind_workspaces_viewer},
        {"SUPER + W", tr->keybind_notification},
        {"SUPER + TAB", tr->keybind_system_info},
        {"SUPER + SHIFT + W", tr->keybind_wallpapers_menu},
        {"SUPER + SHIFT + M", tr->keybind_exit_hyprland},
        {"SUPER + V", tr->keybind_toggle_float},
        {"SUPER + D", tr->keybind_launch_editor},
        {"SUPER + E", tr->keybind_launch_file_manager},
        {"SUPER + O", tr->keybind_launch_browser},
        {"PRINTSC", tr->keybind_full_screenshot},
        {"SUPER + S", tr->keybind_region_screenshot},
        {"F1", tr->keybind_mute_volume},
        {"F6", tr->keybind_lower_brightness},
        {"F7", tr->keybind_higher_brightness},
        {"Fn + F2", tr->keybind_lower_volume},
        {"Fn + F3", tr->keybind_higher_volume},
        {"Fn + F4", tr->keybind_mute_microphone},
        {NULL, NULL}
    };

    // Add keybinds to grid
    for (int i = 0; keybinds[i].shortcut != NULL; i++) {
        GtkWidget *shortcut_label = gtk_label_new(keybinds[i].shortcut);
        gtk_widget_add_css_class(shortcut_label, "keybind-shortcut");
        gtk_widget_set_halign(shortcut_label, GTK_ALIGN_END);
        
        GtkWidget *description_label = gtk_label_new(keybinds[i].description_key);
        gtk_widget_add_css_class(description_label, "keybind-description");
        gtk_widget_set_halign(description_label, GTK_ALIGN_START);
        
        gtk_grid_attach(GTK_GRID(keybinds_grid), shortcut_label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(keybinds_grid), description_label, 1, i, 1, 1);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), keybinds_grid);
    gtk_box_append(GTK_BOX(main_box), scrolled);

    return main_box;
}

static GtkWidget* create_updater_page(void) {
    const Translations* tr = get_translations();
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);
    gtk_widget_set_margin_start(main_box, 40);
    gtk_widget_set_margin_end(main_box, 40);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(title_box, GTK_ALIGN_CENTER);

    GtkWidget *title = gtk_label_new(tr->updater_title);
    gtk_widget_add_css_class(title, "display-2");
    gtk_box_append(GTK_BOX(title_box), title);

    gtk_box_append(GTK_BOX(main_box), title_box);

    // Add updater image
    GtkWidget *image = gtk_picture_new_for_resource("/org/elysiaos/welcome/updater.png");
    gtk_widget_set_size_request(image, 300, 200);
    gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), image);

    // Add description text
    GtkWidget *description = gtk_label_new(tr->updater_subtitle);
    gtk_widget_add_css_class(description, "title-3");
    gtk_widget_add_css_class(description, "dim-label");
    gtk_widget_set_halign(description, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), description);

    return main_box;
}

static GtkWidget* create_settings_page(void) {
    const Translations* tr = get_translations();
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);
    gtk_widget_set_margin_start(main_box, 40);
    gtk_widget_set_margin_end(main_box, 40);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(title_box, GTK_ALIGN_CENTER);

    GtkWidget *title = gtk_label_new(tr->settings_title);
    gtk_widget_add_css_class(title, "display-2");
    gtk_box_append(GTK_BOX(title_box), title);

    gtk_box_append(GTK_BOX(main_box), title_box);

    // Add settings image
    GtkWidget *image = gtk_picture_new_for_resource("/org/elysiaos/welcome/settings.png");
    gtk_widget_set_size_request(image, 300, 200);
    gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), image);

    // Add description text
    GtkWidget *description = gtk_label_new(tr->settings_subtitle);
    gtk_widget_add_css_class(description, "title-3");
    gtk_widget_add_css_class(description, "dim-label");
    gtk_widget_set_halign(description, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), description);

    return main_box;
}

static GtkWidget* create_store_page(void) {
    const Translations* tr = get_translations();
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);
    gtk_widget_set_margin_start(main_box, 40);
    gtk_widget_set_margin_end(main_box, 40);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(title_box, GTK_ALIGN_CENTER);

    GtkWidget *title = gtk_label_new(tr->store_title);
    gtk_widget_add_css_class(title, "display-2");
    gtk_box_append(GTK_BOX(title_box), title);

    gtk_box_append(GTK_BOX(main_box), title_box);

    // Add store image
    GtkWidget *image = gtk_picture_new_for_resource("/org/elysiaos/welcome/store.png");
    gtk_widget_set_size_request(image, 300, 200);
    gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), image);

    // Add description text
    GtkWidget *description = gtk_label_new(tr->store_subtitle);
    gtk_widget_add_css_class(description, "title-3");
    gtk_widget_add_css_class(description, "dim-label");
    gtk_widget_set_halign(description, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), description);

    return main_box;
}

static GtkWidget* create_complete_page(WelcomeApp *app) {
    const Translations* tr = get_translations();
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 40);
    gtk_widget_set_halign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(main_box, TRUE);
    gtk_widget_set_vexpand(main_box, TRUE);

    /* Top section with logo and text */
    GtkWidget *top_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 40);
    gtk_widget_set_halign(top_section, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(top_section, GTK_ALIGN_CENTER);

    const char *logo_path = app->is_dark_theme ? 
        "/org/elysiaos/welcome/elyoslogo1.png" : 
        "/org/elysiaos/welcome/elyoslogo2.png";
    GtkWidget *logo = make_resource_image(logo_path, 200);
    gtk_widget_set_halign(logo, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(logo, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(top_section), logo);

    GtkWidget *complete_label = gtk_label_new(tr->complete_title);
    gtk_widget_add_css_class(complete_label, "display-1");
    gtk_widget_add_css_class(complete_label, "accent");
    gtk_widget_set_halign(complete_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(top_section), complete_label);

    gtk_box_append(GTK_BOX(main_box), top_section);

    /* Buttons section */
    GtkWidget *buttons_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(buttons_section, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(buttons_section, GTK_ALIGN_CENTER);

    /* Support button */
    GtkWidget *support_btn = gtk_button_new_with_label(tr->support_button);
    gtk_widget_set_size_request(support_btn, 120, 40);
    gtk_widget_add_css_class(support_btn, "glass-button");
    gtk_widget_set_halign(support_btn, GTK_ALIGN_CENTER);
    g_signal_connect(support_btn, "clicked", G_CALLBACK(on_support_clicked), NULL);
    gtk_box_append(GTK_BOX(buttons_section), support_btn);

    /* Discord button */
    GtkWidget *discord_btn = gtk_button_new_with_label(tr->discord_button);
    gtk_widget_set_size_request(discord_btn, 120, 40);
    gtk_widget_add_css_class(discord_btn, "glass-button");
    gtk_widget_set_halign(discord_btn, GTK_ALIGN_CENTER);
    g_signal_connect(discord_btn, "clicked", G_CALLBACK(on_discord_clicked), NULL);
    gtk_box_append(GTK_BOX(buttons_section), discord_btn);

    /* Website button */
    GtkWidget *website_btn = gtk_button_new_with_label(tr->website_button);
    gtk_widget_set_size_request(website_btn, 120, 40);
    gtk_widget_add_css_class(website_btn, "glass-button");
    gtk_widget_set_halign(website_btn, GTK_ALIGN_CENTER);
    g_signal_connect(website_btn, "clicked", G_CALLBACK(on_website_clicked), NULL);
    gtk_box_append(GTK_BOX(buttons_section), website_btn);

    gtk_box_append(GTK_BOX(main_box), buttons_section);

    /* Close button at the bottom */
    GtkWidget *close_section = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(close_section, GTK_ALIGN_CENTER);

    GtkWidget *close_btn = gtk_button_new_with_label(tr->close_button);
    gtk_widget_set_size_request(close_btn, 120, 40);
    gtk_widget_add_css_class(close_btn, "close-button");
    gtk_widget_set_halign(close_btn, GTK_ALIGN_CENTER);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_finish_clicked), NULL);
    gtk_box_append(GTK_BOX(close_section), close_btn);

    gtk_box_append(GTK_BOX(main_box), close_section);

    return main_box;
}

/* ---------- Navigation and CSS ---------- */

static void update_logo_images(WelcomeApp *app) {
    g_print("Updating logo images, dark theme: %s\n", app->is_dark_theme ? "true" : "false");
    
    // Update welcome page logo
    GtkWidget *welcome_page = gtk_stack_get_child_by_name(GTK_STACK(app->content_stack), "welcome");
    if (welcome_page) {
        g_print("Found welcome page\n");
        GtkWidget *welcome_box = gtk_widget_get_first_child(welcome_page);
        if (welcome_box) {
            g_print("Found welcome box\n");
            GtkWidget *welcome_logo = gtk_widget_get_next_sibling(gtk_widget_get_first_child(welcome_box));
            if (welcome_logo) {
                g_print("Found welcome logo\n");
                const char *logo_path = app->is_dark_theme ? 
                    "/org/elysiaos/welcome/elyoslogo1.png" : 
                    "/org/elysiaos/welcome/elyoslogo2.png";
                
                g_print("Setting welcome logo to: %s\n", logo_path);
                
                // Remove the old image and add a new one
                gtk_box_remove(GTK_BOX(welcome_box), welcome_logo);
                
                GtkWidget *new_logo = make_resource_image(logo_path, 200);
                gtk_box_insert_child_after(GTK_BOX(welcome_box), new_logo, gtk_widget_get_first_child(welcome_box));
            } else {
                g_print("Did not find welcome logo\n");
            }
        } else {
            g_print("Did not find welcome box\n");
        }
    } else {
        g_print("Did not find welcome page\n");
    }
    
    // Update complete page logo
    GtkWidget *complete_page = gtk_stack_get_child_by_name(GTK_STACK(app->content_stack), "complete");
    if (complete_page) {
        g_print("Found complete page\n");
        GtkWidget *complete_box = gtk_widget_get_first_child(complete_page);
        if (complete_box) {
            g_print("Found complete box\n");
            GtkWidget *complete_top_section = gtk_widget_get_first_child(complete_box);
            if (complete_top_section) {
                g_print("Found complete top section\n");
                GtkWidget *complete_logo = gtk_widget_get_first_child(complete_top_section);
                if (complete_logo) {
                    g_print("Found complete logo\n");
                    const char *logo_path = app->is_dark_theme ? 
                        "/org/elysiaos/welcome/elyoslogo1.png" : 
                        "/org/elysiaos/welcome/elyoslogo2.png";
                    
                    g_print("Setting complete logo to: %s\n", logo_path);
                    
                    // Remove the old image and add a new one
                    gtk_box_remove(GTK_BOX(complete_top_section), complete_logo);
                    
                    GtkWidget *new_logo = make_resource_image(logo_path, 200);
                    gtk_box_append(GTK_BOX(complete_top_section), new_logo);
                } else {
                    g_print("Did not find complete logo\n");
                }
            } else {
                g_print("Did not find complete top section\n");
            }
        } else {
            g_print("Did not find complete box\n");
        }
    } else {
        g_print("Did not find complete page\n");
    }
}

static void update_theme_css(WelcomeApp *app) {
    if (!app->theme_provider) return;
    
    const char *css;
    if (app->is_dark_theme) {
        css = "window { background-color: #333; color: #ffffff;}"
              "window {font-family: ElysiaOSNew12;} "
              ".display-1 {font-size: 34px; }"
              ".display-2 {font-size: 28px; font-weight: bold; }"
              ".page-indicators { margin: 20px; }"
              ".page-dot { min-width:12px; min-height:12px; border-radius:6px; margin:0 4px; }"
              ".active-dot { background-color: #fc77d9; }"
              ".inactive-dot { background-color: #666; }"
              ".theme-card { border-radius:16px; border:2px solid #555; background:#444; padding:8px; color: #ffffff; background-size: cover; background-position: center; width: 180px; height: 120px; }"  // Fixed size
              ".theme-card:hover { border-color:#fc77d9; }"
              ".theme-selected { border-color:#fc77d9 !important; background:#555 !important; }"
              ".theme-card image { -gtk-icon-style: regular; }"
              ".theme-label { background: rgba(0, 0, 0, 0.7); color: white; padding: 4px 8px; border-radius: 6px; font-size: 14px; }"
              ".theme-card picture { min-width: 160px; min-height: 80px; max-width: 160px; max-height: 80px; }"
              "#light-theme-button { background-image: url('/org/elysiaos/welcome/light.png'); }"
              "#dark-theme-button { background-image: url('/org/elysiaos/welcome/dark.png'); }"
              ".keybind-shortcut {"
              "  font-family: ElysiaOSNew12;"
              "  font-size: 11px;"
              "  color: #ffffff;"
              "  margin: 2px 8px 2px 0px;"
              "  font-weight: 600;"
              "  background: linear-gradient(to right, rgba(112, 119, 189, 0.2) 0%, rgba(177, 201, 236, 0.3) 100%);"
              "  border: 1px solid rgba(112, 119, 189, 0.4);"
              "  border-radius: 4px;"
              "  padding: 4px 8px;"
              "}"
              ".keybind-description {"
              "  font-family: ElysiaOSNew12;"
              "  font-size: 11px;"
              "  color: #cccccc;"
              "  margin: 2px 0px 2px 8px;"
              "  font-weight: 400;"
              "}"
              ".scrolled-window {"
              "  background: transparent;"
              "  border: none;"
              "}"
              ".scrolled-window scrollbar {"
              "  background: transparent;"
              "}"
              ".scrolled-window scrollbar slider {"
              "  background: rgba(255, 255, 255, 0.3);"
              "  border-radius: 6px;"
              "  min-width: 8px;"
              "}"
              ".scrolled-window scrollbar slider:hover {"
              "  background: rgba(255, 255, 255, 0.5);"
              "}"
              ".tip-label {"
              "  font-family: ElysiaOSNew12;"
              "  font-size: 10px;"
              "  color: #8e8e93;"
              "  margin: 8px 0px;"
              "  font-style: italic;"
              "}";
    } else {
        css = "window { background-color: #ffedfa; color: #333;}"
              "window {font-family: ElysiaOSNew12;} "
              ".display-1 {font-size: 34px; }"
              ".display-2 {font-size: 28px; font-weight: bold; }"
              ".page-indicators { margin: 20px; }"
              ".page-dot { min-width:12px; min-height:12px; border-radius:6px; margin:0 4px; }"
              ".active-dot { background-color: #fc77d9; }"
              ".inactive-dot { background-color: #c0c0c0; }"
              ".theme-card { border-radius:16px; border:2px solid #e0e0e0; background:#fafafa; padding:8px; color: #333; background-size: cover; background-position: center; width: 180px; height: 120px; }"  // Fixed size
              ".theme-card:hover { border-color:#fc77d9; }"
              ".theme-selected { border-color:#fc77d9 !important; background:#f0f7ff !important; }"
              ".theme-card image { -gtk-icon-style: regular; }"
              ".theme-label { background: rgba(255, 255, 255, 0.7); color: black; padding: 4px 8px; border-radius: 6px; font-size: 14px; }"
              ".theme-card picture { min-width: 160px; min-height: 80px; max-width: 160px; max-height: 80px; }"
              "#light-theme-button { background-image: url('/org/elysiaos/welcome/light.png'); }"
              "#dark-theme-button { background-image: url('/org/elysiaos/welcome/dark.png'); }"
              ".keybind-shortcut {"
              "  font-family: ElysiaOSNew12;"
              "  font-size: 11px;"
              "  color: #1d1d1f;"
              "  margin: 2px 8px 2px 0px;"
              "  font-weight: 600;"
              "  background: linear-gradient(to right, rgba(229, 167, 198, 0.2) 0%, rgba(237, 206, 227, 0.3) 100%);"
              "  border: 1px solid rgba(229, 167, 198, 0.4);"
              "  border-radius: 4px;"
              "  padding: 4px 8px;"
              "}"
              ".keybind-description {"
              "  font-family: ElysiaOSNew12;"
              "  font-size: 11px;"
              "  color: #6d6d70;"
              "  margin: 2px 0px 2px 8px;"
              "  font-weight: 400;"
              "}"
              ".scrolled-window {"
              "  background: transparent;"
              "  border: none;"
              "}"
              ".scrolled-window scrollbar {"
              "  background: transparent;"
              "}"
              ".scrolled-window scrollbar slider {"
              "  background: rgba(0, 0, 0, 0.3);"
              "  border-radius: 6px;"
              "  min-width: 8px;"
              "}"
              ".scrolled-window scrollbar slider:hover {"
              "  background: rgba(0, 0, 0, 0.5);"
              "}"
              ".tip-label {"
              "  font-family: ElysiaOSNew12;"
              "  font-size: 10px;"
              "  color: #8e8e93;"
              "  margin: 8px 0px;"
              "  font-style: italic;"
              "}";
    }
    
    gtk_css_provider_load_from_string(app->theme_provider, css);
    
    // Update logo images based on theme
    update_logo_images(app);
}

static void update_page_indicators(WelcomeApp *app) {
    for (int i = 0; i < (int)app->page_dots->len; ++i) {
        GtkWidget *dot = reinterpret_cast<GtkWidget*>(g_ptr_array_index(app->page_dots, i));
        if (i == app->current_page) {
            gtk_widget_add_css_class(dot, "active-dot");
            gtk_widget_remove_css_class(dot, "inactive-dot");
        } else {
            gtk_widget_remove_css_class(dot, "active-dot");
            gtk_widget_add_css_class(dot, "inactive-dot");
        }
    }
}

static void update_navigation(WelcomeApp *app) {
    switch (app->current_page) {
        case 0:
            gtk_widget_set_visible(app->back_arrow, FALSE);
            gtk_widget_set_visible(app->next_arrow, TRUE);
            gtk_widget_set_visible(app->skip_button, TRUE);
            break;
        case 1:
            gtk_widget_set_visible(app->back_arrow, TRUE);
            gtk_widget_set_visible(app->next_arrow, TRUE);
            gtk_widget_set_visible(app->skip_button, TRUE);
            break;
        case 2:
            gtk_widget_set_visible(app->back_arrow, TRUE);
            gtk_widget_set_visible(app->next_arrow, TRUE);
            gtk_widget_set_visible(app->skip_button, TRUE);
            break;
        case 3:
            gtk_widget_set_visible(app->back_arrow, TRUE);
            gtk_widget_set_visible(app->next_arrow, TRUE);
            gtk_widget_set_visible(app->skip_button, TRUE);
            break;
        case 4:
            gtk_widget_set_visible(app->back_arrow, TRUE);
            gtk_widget_set_visible(app->next_arrow, TRUE);
            gtk_widget_set_visible(app->skip_button, TRUE);
            break;
        case 5:
            gtk_widget_set_visible(app->back_arrow, TRUE);
            gtk_widget_set_visible(app->next_arrow, TRUE);
            gtk_widget_set_visible(app->skip_button, TRUE);
            break;
        case 6:
            gtk_widget_set_visible(app->back_arrow, TRUE);
            gtk_widget_set_visible(app->next_arrow, TRUE);
            gtk_widget_set_visible(app->skip_button, TRUE);
            break;
        case 7:
            gtk_widget_set_visible(app->back_arrow, TRUE);
            gtk_widget_set_visible(app->next_arrow, FALSE);
            gtk_widget_set_visible(app->skip_button, FALSE);
            break;
    }
    update_page_indicators(app);
}

static void on_back_clicked(GtkButton *button, WelcomeApp *app) {
    (void)button;
    if (app->current_page > 0) {
        app->current_page--;
        const char* page_names[] = {"welcome", "theme", "network", "keybinds", "updater", "settings", "store", "complete"};
        gtk_stack_set_visible_child_name(GTK_STACK(app->content_stack), page_names[app->current_page]);
        update_navigation(app);
    }
}

static void on_next_clicked(GtkButton *button, WelcomeApp *app) {
    (void)button;
    if (app->current_page < 7) {
        app->current_page++;
        const char* page_names[] = {"welcome", "theme", "network", "keybinds", "updater", "settings", "store", "complete"};
        gtk_stack_set_visible_child_name(GTK_STACK(app->content_stack), page_names[app->current_page]);
        update_navigation(app);
    }
}

static void on_skip_clicked(GtkButton *button, WelcomeApp *app) {
    (void)button;
    g_print("Setup completed (skipped)\n");
    gtk_window_destroy(GTK_WINDOW(app->window));
}

static void on_finish_clicked(GtkButton *button, WelcomeApp *app) {
    (void)button;
    g_print("Setup completed\n");
    gtk_window_destroy(GTK_WINDOW(app->window));
}

static void setup_css(WelcomeApp *app) {
    app->theme_provider = gtk_css_provider_new();
    app->is_dark_theme = FALSE; // Start with light theme
    
    const char *css =
        "window { background-color: #ffedfa; color: #333;}"
        "window {font-family: ElysiaOSNew12;} "
        ".display-1 {font-size: 34px; }"
        ".display-2 {font-size: 28px; font-weight: bold; }"
        ".page-indicators { margin: 20px; }"
        ".page-dot { min-width:12px; min-height:12px; border-radius:6px; margin:0 4px; }"
        ".active-dot { background-color: #fc77d9; }"
        ".inactive-dot { background-color: #c0c0c0; }"
        ".theme-card { border-radius:16px; border:2px solid #e0e0e0; background:#fafafa; padding:8px; color: #333; background-size: cover; background-position: center; width: 180px; height: 120px; }"  // Fixed size
        ".theme-card:hover { border-color:#fc77d9; }"
        ".theme-selected { border-color:#fc77d9 !important; background:#f0f7ff !important; }"
        ".theme-card image { -gtk-icon-style: regular; }"
        ".theme-label { background: rgba(255, 255, 255, 0.7); color: black; padding: 4px 8px; border-radius: 6px; font-size: 14px; }"
        ".theme-card picture { min-width: 160px; min-height: 80px; max-width: 160px; max-height: 80px; }"
        "#light-theme-button { background-image: url('/org/elysiaos/welcome/light.png'); }"
        "#dark-theme-button { background-image: url('/org/elysiaos/welcome/dark.png'); }"
        ".keybind-shortcut {"
        "  font-family: ElysiaOSNew12;"
        "  font-size: 11px;"
        "  color: #1d1d1f;"
        "  margin: 2px 8px 2px 0px;"
        "  font-weight: 600;"
        "  background: linear-gradient(to right, rgba(229, 167, 198, 0.2) 0%, rgba(237, 206, 227, 0.3) 100%);"
        "  border: 1px solid rgba(229, 167, 198, 0.4);"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "}"
        ".keybind-description {"
        "  font-family: ElysiaOSNew12;"
        "  font-size: 11px;"
        "  color: #6d6d70;"
        "  margin: 2px 0px 2px 8px;"
        "  font-weight: 400;"
        "}"
        ".scrolled-window {"
        "  background: transparent;"
        "  border: none;"
        "}"
        ".scrolled-window scrollbar {"
        "  background: transparent;"
        "}"
        ".scrolled-window scrollbar slider {"
        "  background: rgba(0, 0, 0, 0.3);"
        "  border-radius: 6px;"
        "  min-width: 8px;"
        "}"
        ".scrolled-window scrollbar slider:hover {"
        "  background: rgba(0, 0, 0, 0.5);"
        "}"
        ".tip-label {"
        "  font-family: ElysiaOSNew12;"
        "  font-size: 10px;"
        "  color: #8e8e93;"
        "  margin: 8px 0px;"
        "  font-style: italic;"
        "}"
        ".title-3 {"
        "  font-size: 18px;"
        "  font-weight: normal;"
        "}"
        ".dim-label {"
        "  color: #666;"
        "}";
        
    gtk_css_provider_load_from_string(app->theme_provider, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(app->theme_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

/* ---------- Lifecycle ---------- */

static void on_window_destroy(GtkWidget *w, gpointer user_data) {
    WelcomeApp *app = (WelcomeApp*) user_data;
    if (!app) return;
    
    /* Clean up timeouts if active */
    if (app->update_timeout_id > 0) {
        g_source_remove(app->update_timeout_id);
        app->update_timeout_id = 0;
    }
    
    if (app->theme_check_id > 0) {
        g_source_remove(app->theme_check_id);
        app->theme_check_id = 0;
    }
    
    if (app->nm_client) g_object_unref(app->nm_client);
    if (app->page_dots) g_ptr_array_unref(app->page_dots);
    if (app->theme_provider) g_object_unref(app->theme_provider);
    g_clear_pointer(&app->selected_theme, g_free);
    g_clear_pointer(&app->current_gtk_theme, g_free);
    g_free(app);
    (void)w;
}

static void activate(GtkApplication *app_gtk, gpointer user_data) {
    const Translations* tr = get_translations();
    
    (void)user_data;

    WelcomeApp *app = g_new0(WelcomeApp, 1);
    app->current_page = 0;
    app->page_dots = g_ptr_array_new();
    app->updating_wifi_switch = FALSE;
    app->is_dark_theme = FALSE;
    app->current_gtk_theme = NULL;
    app->theme_check_id = 0;
    app->networking_enabled = FALSE;
    app->has_ethernet_connection = FALSE;
    app->update_timeout_id = 0;

    setup_css(app);

    /* Detect and apply initial theme */
    detect_and_apply_theme(app);
    
    /* Start theme monitoring */
    app->theme_check_id = g_timeout_add(2000, theme_check_timeout, app);

    app->window = gtk_application_window_new(app_gtk);
    gtk_window_set_title(GTK_WINDOW(app->window), tr->welcome_subtitle);
    gtk_window_set_default_size(GTK_WINDOW(app->window), 900, 700);
    gtk_window_set_resizable(GTK_WINDOW(app->window), FALSE);

    app->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(app->window), app->main_box);

    /* Page indicators */
    app->page_indicators = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(app->page_indicators, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(app->page_indicators, "page-indicators");

    for (int i = 0; i < 8; ++i) {
        GtkWidget *dot = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_add_css_class(dot, "page-dot");
        if (i == 0) gtk_widget_add_css_class(dot, "active-dot");
        else        gtk_widget_add_css_class(dot, "inactive-dot");
        gtk_box_append(GTK_BOX(app->page_indicators), dot);
        g_ptr_array_add(app->page_dots, dot);
    }
    gtk_box_append(GTK_BOX(app->main_box), app->page_indicators);

    /* Create overlay for content and navigation arrows */
    GtkWidget *overlay = gtk_overlay_new();
    gtk_widget_set_vexpand(overlay, TRUE);
    gtk_box_append(GTK_BOX(app->main_box), overlay);

    /* Content stack */
    app->content_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->content_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(app->content_stack), 200);
    gtk_widget_set_hexpand(app->content_stack, TRUE);
    gtk_widget_set_vexpand(app->content_stack, TRUE);

    gtk_stack_add_named(GTK_STACK(app->content_stack), create_welcome_page(app), "welcome");
    gtk_stack_add_named(GTK_STACK(app->content_stack), create_theme_page(app), "theme");
    gtk_stack_add_named(GTK_STACK(app->content_stack), create_network_page(app), "network");
    gtk_stack_add_named(GTK_STACK(app->content_stack), create_keybinds_page(), "keybinds");
    gtk_stack_add_named(GTK_STACK(app->content_stack), create_updater_page(), "updater");
    gtk_stack_add_named(GTK_STACK(app->content_stack), create_settings_page(), "settings");
    gtk_stack_add_named(GTK_STACK(app->content_stack), create_store_page(), "store");
    gtk_stack_add_named(GTK_STACK(app->content_stack), create_complete_page(app), "complete");

    gtk_overlay_set_child(GTK_OVERLAY(overlay), app->content_stack);

    /* Back arrow on the left */
    app->back_arrow = make_icon_button("go-previous-symbolic", 16);
    gtk_widget_add_css_class(app->back_arrow, "nav-arrow");
    gtk_widget_set_margin_start(app->back_arrow, 20);
    gtk_widget_set_margin_end(app->back_arrow, 20);
    gtk_widget_set_valign(app->back_arrow, GTK_ALIGN_CENTER);
    g_signal_connect(app->back_arrow, "clicked", G_CALLBACK(on_back_clicked), app);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), app->back_arrow);
    gtk_widget_set_halign(app->back_arrow, GTK_ALIGN_START);

    /* Next arrow on the right */
    app->next_arrow = make_icon_button("go-next-symbolic", 16);
    gtk_widget_add_css_class(app->next_arrow, "nav-arrow");
    gtk_widget_set_margin_start(app->next_arrow, 20);
    gtk_widget_set_margin_end(app->next_arrow, 20);
    gtk_widget_set_valign(app->next_arrow, GTK_ALIGN_CENTER);
    g_signal_connect(app->next_arrow, "clicked", G_CALLBACK(on_next_clicked), app);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), app->next_arrow);
    gtk_widget_set_halign(app->next_arrow, GTK_ALIGN_END);

    /* Skip button at the bottom */
    app->navigation_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(app->navigation_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(app->navigation_box, 10);
    gtk_widget_set_margin_bottom(app->navigation_box, 20);

    app->skip_button = gtk_button_new_with_label(tr->skip_button);
    gtk_widget_add_css_class(app->skip_button, "skip-btn");
    g_signal_connect(app->skip_button, "clicked", G_CALLBACK(on_skip_clicked), app);
    gtk_box_append(GTK_BOX(app->navigation_box), app->skip_button);

    gtk_box_append(GTK_BOX(app->main_box), app->navigation_box);

    gtk_stack_set_visible_child_name(GTK_STACK(app->content_stack), "welcome");
    update_navigation(app);

    g_signal_connect(app->window, "destroy", G_CALLBACK(on_window_destroy), app);

    gtk_window_present(GTK_WINDOW(app->window));
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    // Register resources
    GResource *resource = resources_get_resource();
    g_resources_register(resource);
    
    GtkApplication *app = gtk_application_new("org.elysiaos.welcome", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}