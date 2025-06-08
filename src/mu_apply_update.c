#include "mu_apply_update.h"
#include <gio/gio.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH_LEN 512

/* ------------------------------------------------------------------------ */
static gboolean is_path_safe(const gchar *path) {
	// Ensure path starts with allowed root
	char resolved[MAX_PATH_LEN];
	if (!realpath(path, resolved)) {
		g_printerr("mu-apply-update: Failed to resolve real path: %s\n",
			   path);
		return FALSE;
	}
	if (!(g_str_has_prefix(resolved, "/boot") ||
	      g_str_has_prefix(resolved, "/usr") ||
	      g_str_has_prefix(resolved, "/etc"))) {
		g_printerr(
		    "mu-apply-update: Resolved path not in allowed roots: %s\n",
		    resolved);
		return FALSE;
	}
}

/* ------------------------------------------------------------------------ */
gboolean apply_payload(const gchar *inbox_path, const gchar *payload_name,
		       const gchar *destination_path) {
	gchar full_src[MAX_PATH_LEN];
	snprintf(full_src, sizeof(full_src), "%s/%s", inbox_path, payload_name);

	// Check file exists
	if (access(full_src, R_OK) != 0) {
		g_printerr("mu-apply-update: Cannot access %s\n", full_src);
		return FALSE;
	}

	// Validate destination path safety
	if (!is_path_safe(destination_path)) {
		g_printerr("mu-apply-update: Unsafe target path: %s\n",
			   destination_path);
		return FALSE;
	}

	// Make sure destination path exists
	if (g_mkdir_with_parents(destination_path, 0755) != 0) {
		g_printerr("mu-apply-update: Failed to create path: %s\n",
			   destination_path);
		return FALSE;
	}

	// Assumes payload_name is a .tar archive (verified before calling)
	g_autoptr(GSubprocess)
	    proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_SILENCE |
					G_SUBPROCESS_FLAGS_STDERR_SILENCE,
				    NULL, "tar", "-xf", full_src, "-C",
				    destination_path, NULL);

	if (!proc || !g_subprocess_wait_check(proc, NULL, NULL)) {
		g_printerr("mu-apply-update: Tar extraction failed\n");
		return FALSE;
	}

	g_print("mu-apply-update: Applied %s to %s\n", payload_name,
		destination_path);
	return TRUE;
}

/* ------------------------------------------------------------------------ */
gboolean run_post_script(const gchar *script_path) {
	if (access(script_path, X_OK) != 0) {
		g_printerr("mu-apply-update: Post script is not executable: %s\n", script_path);
		return FALSE;
	}

	g_autoptr(GSubprocess) proc = g_subprocess_new(
		G_SUBPROCESS_FLAGS_NONE,
		NULL,
		script_path,
		NULL);

	if (!proc || !g_subprocess_wait_check(proc, NULL, NULL)) {
		g_printerr("mu-apply-update: Post script execution failed\n");
		return FALSE;
	}

	g_print("mu-apply-update: Post script executed successfully\n");
	return TRUE;
}
