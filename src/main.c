#include <gio/gio.h>
#include "mu_update.h"

// Forward declaration
static gboolean on_handle_trigger_update(MuUpdateOrgMuUpdateSkeleton *skeleton,
                                         GDBusMethodInvocation *invocation);

// Callback when the bus is acquired
static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    MuUpdateOrgMuUpdate *skeleton = mu_update_org_mu_update_skeleton_new();

    g_signal_connect(skeleton, "handle-trigger-update", G_CALLBACK(on_handle_trigger_update), NULL);

    if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton), connection, "/org/mu/Update", NULL)) {
        g_printerr("Failed to export D-Bus interface\n");
        exit(1);
    }

    g_print("mu-update-daemon: D-Bus interface exported and ready.\n");
}

// Handler for TriggerUpdate method call
static gboolean
on_handle_trigger_update(MuUpdateOrgMuUpdateSkeleton *skeleton,
                         GDBusMethodInvocation *invocation)
{
    g_print("Received TriggerUpdate with hash\n");

    gboolean success = TRUE;
    const gchar *status_message = "Update applied successfully.";

    // No cast needed here!
    mu_update_org_mu_update_complete_trigger_update(
        (MuUpdateOrgMuUpdate *)skeleton,
        invocation,
        success,
        status_message
    );
    return TRUE;
}

int main(int argc, char *argv[]) {
    guint owner_id = g_bus_own_name(
        G_BUS_TYPE_SYSTEM,
        "org.mu.Update",
        G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired,
        NULL,
        NULL,
        NULL,
        NULL
    );

    g_print("mu-update-daemon: Running...\n");

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    g_bus_unown_name(owner_id);
    g_main_loop_unref(loop);
    return 0;
}
