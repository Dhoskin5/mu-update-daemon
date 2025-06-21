#include "mu_apply_update.h"
#include "mu_manifest.h"
#include "mu_update.h"
#include "mu_verify.h"
#include <gio/gio.h>
#include <systemd/sd-daemon.h>

static gboolean on_handle_trigger_update(MuUpdateOrgMuUpdateSkeleton *skeleton,
					 GDBusMethodInvocation *invocation,
					 const gchar *inbox_path);

/* ------------------------------------------------------------------------ */
static void on_bus_acquired(GDBusConnection *connection, const gchar *name,
			    gpointer user_data) {
	MuUpdateOrgMuUpdate *skeleton = mu_update_org_mu_update_skeleton_new();

	g_signal_connect(skeleton, "handle-trigger-update",
			 G_CALLBACK(on_handle_trigger_update), NULL);

	if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(
						  skeleton),
					      connection, "/org/mu/Update",
					      NULL)) {
		g_printerr("Failed to export D-Bus interface\n");
		exit(1);
	}

	// Notify systemd that the service is ready
	sd_notify(0, "READY=1");

	g_print("mu-update-daemon: D-Bus interface exported and ready.\n");
}

/* ------------------------------------------------------------------------ */
static gboolean on_handle_trigger_update(MuUpdateOrgMuUpdateSkeleton *skeleton,
					 GDBusMethodInvocation *invocation,
					 const gchar *inbox_path) {
	g_print("Received TriggerUpdate with inbox: %s\n", inbox_path);

	gchar *manifest_path = g_build_filename(inbox_path, "manifest.json",
						NULL);
	gchar *sig_path = g_build_filename(inbox_path, "manifest.json.minisig",
					   NULL);
	const gchar *trusted_dir = "/etc/mu/trusted.d";

	gboolean success = FALSE;
	gchar *verified_key = NULL;
	const gchar *status_message = "Unknown error";

	if (!verify_manifest_signature(manifest_path, sig_path, trusted_dir,
				       &verified_key)) {
		g_printerr(
		    "mu-update-daemon: Failed to verify manifest signature.\n");
		status_message = "Signature verification failed!";
		goto cleanup;
	}

	g_print("mu-update-daemon: Signature verified with key: %s\n",
		verified_key);

	// Parse the manifest
	struct manifest m;
	int iret = parse_manifest(manifest_path, &m);
	if (iret < 0) {
		status_message = "Failed to parse manifest!";
	} else {
		g_print("mu-update-daemon: Manifest parsed "
			"successfully.\n");
		g_print("Version: %s\n", m.version);
		g_print("Timestamp: %s\n", m.timestamp);
		g_print("Description: %s\n", m.description);
		g_print("Key ID: %s\n", m.key_id);
		g_print("File Count: %d\n", m.file_count);

		for (int i = 0; i < m.file_count; ++i) {
			g_print("File %d of %d:\n", i + 1, m.file_count);
			g_print("  Path: %s\n", m.files[i].path);
			g_print("  SHA256: %s\n", m.files[i].sha256);
			g_print("  Payload Name: %s\n",
				m.files[i].payload_name);

			const gchar *name = m.files[i].payload_name;
			gchar *full = g_build_filename(inbox_path, name, NULL);

			if (!verify_file_sha256(full, m.files[i].sha256)) {
				status_message = "Payload hash mismatch!";
				g_free(full);
				goto cleanup;
			}
			g_free(full);

			if (!apply_payload(inbox_path, m.files[i].payload_name,
					   m.files[i].path)) {
				status_message = "Failed to apply payload!";
				goto cleanup;
			}
		}

		if (m.script_present) {
			g_print("Script Name: %s\n", m.script.script_name);
			g_print("Script SHA256: %s\n", m.script.sha256);

			gchar *script_full = g_build_filename(
			    inbox_path, m.script.script_name, NULL);
			if (!verify_file_sha256(script_full, m.script.sha256)) {
				status_message = "Script hash mismatch!";
				g_free(script_full);
				goto cleanup;
			}

			if (!run_post_script(script_full)) {
				status_message = "Failed to run post script!";
				g_free(script_full);
				goto cleanup;
			}

			g_free(script_full);

		} else {
			g_print("Script is not present.\n");
		}

		success = TRUE;
	}

cleanup:
	// Clean up resources
	if (manifest_path) {
		g_free(manifest_path);
	}
	if (sig_path) {
		g_free(sig_path);
	}
	if (verified_key) {
		g_free(verified_key);
	}

	// No cast needed here!
	mu_update_org_mu_update_complete_trigger_update(
	    (MuUpdateOrgMuUpdate *)skeleton, invocation, success,
	    status_message);
	return TRUE;
}

/* ------------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
	guint owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.mu.Update",
					G_BUS_NAME_OWNER_FLAGS_NONE,
					on_bus_acquired, NULL, NULL, NULL,
					NULL);

	g_print("mu-update-daemon: Running...\n");

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	g_bus_unown_name(owner_id);
	g_main_loop_unref(loop);
	return 0;
}
