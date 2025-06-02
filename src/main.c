#include <gio/gio.h>
#include <systemd/sd-daemon.h>
#include "mu_update.h"

// Forward declaration
static gboolean on_handle_trigger_update(MuUpdateOrgMuUpdateSkeleton *skeleton,
					 GDBusMethodInvocation *invocation,
					 const gchar *inbox_path);

// Callback when the bus is acquired
static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	MuUpdateOrgMuUpdate *skeleton = mu_update_org_mu_update_skeleton_new();

	g_signal_connect(skeleton, "handle-trigger-update", G_CALLBACK(on_handle_trigger_update), NULL);

	if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton), connection, "/org/mu/Update", NULL))
	{
		g_printerr("Failed to export D-Bus interface\n");
		exit(1);
	}

	//Notify systemd that the service is ready
	sd_notify(0, "READY=1");

	g_print("mu-update-daemon: D-Bus interface exported and ready.\n");
}

// Handler for TriggerUpdate method call
static gboolean on_handle_trigger_update(MuUpdateOrgMuUpdateSkeleton *skeleton,
			 GDBusMethodInvocation *invocation,
			 const gchar *inbox_path)
{
	g_print("Received TriggerUpdate with inbox: %s\n", inbox_path);

	gchar *manifest_path = g_build_filename(inbox_path, "manifest.json", NULL);
	gchar *sig_path = g_build_filename(inbox_path, "manifest.json.minisig", NULL);

	const gchar *trusted_dir = "/etc/mu/trusted.d";
	GDir *dir = g_dir_open(trusted_dir, 0, NULL);
	const gchar *filename;
	gboolean verified = FALSE;
	gboolean success = FALSE;
	gchar *verified_key = NULL;

	while ((filename = g_dir_read_name(dir)) && !verified)
	{
		if (!g_str_has_suffix(filename, ".pub")) {
			continue;
		}				

		gchar *full_path = g_build_filename(trusted_dir, filename, NULL);
		gchar *cmd = g_strdup_printf("minisign -Vm %s -x %s -p %s",
					     manifest_path, sig_path, full_path);
		if (system(cmd) == 0) {
			verified = TRUE;
			verified_key = g_strdup(filename);
		}
		
		g_free(cmd);
		g_free(full_path);
	}

	g_dir_close(dir);

	const gchar *status_message;
	
	if (verified)
	{
		status_message = "Signature verification successful!";


		g_print("mu-update-daemon: Signature verified with key: %s\n", verified_key);
		g_print("mu-update-daemon: Signature valid. Proceeding with update...\n");

		// Here you would typically call the update process, e.g., applying the update.
		// For this example, we will just simulate success.
		// In a real application, you would handle the update logic here.


		success = TRUE;
	}
	else
	{
		status_message = "Signature verification failed! Update aborted.";
		g_printerr("mu-update-daemon: Signature invalid. Aborting update.\n");
	}

	// No cast needed here!
	mu_update_org_mu_update_complete_trigger_update(
	    (MuUpdateOrgMuUpdate *)skeleton,
	    invocation,
	    success,
	    status_message);
	return TRUE;
}

int main(int argc, char *argv[])
{
	guint owner_id = g_bus_own_name(
	    G_BUS_TYPE_SYSTEM,
	    "org.mu.Update",
	    G_BUS_NAME_OWNER_FLAGS_NONE,
	    on_bus_acquired,
	    NULL,
	    NULL,
	    NULL,
	    NULL);

	g_print("mu-update-daemon: Running...\n");

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	g_bus_unown_name(owner_id);
	g_main_loop_unref(loop);
	return 0;
}
