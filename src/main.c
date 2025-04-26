#include <gio/gio.h>
#include "mu_update.h"

static gboolean
on_handle_trigger_update(MuUpdateOrgMuUpdate *skeleton,
                         GDBusMethodInvocation *invocation,
                         const gchar *verified_hash)
{
    g_print("Received TriggerUpdate with hash: %s\n", verified_hash);

    // TODO: Validate hash and perform update logic here
    gboolean success = TRUE;
    const gchar *status_message = "Update applied successfully.";

    mu_update_org_mu_update_complete_trigger_update(skeleton, invocation, success, status_message);
    return TRUE;
}

int main(int argc, char *argv[])
{
    GMainLoop *loop;
    GDBusConnection *connection;
    GError *error = NULL;
    MuUpdateOrgMuUpdate *skeleton;

    // Connect to the system bus
    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        g_printerr("Failed to connect to system bus: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    // Create skeleton object
    skeleton = mu_update_org_mu_update_skeleton_new();

    // Attach handler
    g_signal_connect(skeleton, "handle-trigger-update", G_CALLBACK(on_handle_trigger_update), NULL);

    // Export the object at /org/mu/Update
    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton), connection, "/org/mu/Update", &error);
    if (error) {
        g_printerr("Failed to export object: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    g_print("mu-update-daemon is running...\n");

    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Cleanup
    g_object_unref(skeleton);
    g_main_loop_unref(loop);
    return 0;
}

