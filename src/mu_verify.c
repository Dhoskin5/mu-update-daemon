#include "mu_verify.h"
#include <gio/gio.h>
#include <glib.h>
#include <openssl/sha.h>
#include <stdio.h>

/* ------------------------------------------------------------------------ */
gboolean verify_manifest_signature(const gchar *manifest_path,
				   const gchar *sig_path,
				   const gchar *trusted_dir,
				   gchar **verified_key_out) {
	GDir *dir = g_dir_open(trusted_dir, 0, NULL);
	if (!dir)
		return FALSE;

	const gchar *filename;
	while ((filename = g_dir_read_name(dir))) {
		if (!g_str_has_suffix(filename, ".pub"))
			continue;

		gchar *full_path = g_build_filename(trusted_dir, filename,
						    NULL);
		gchar *cmd = g_strdup_printf("minisign -Vm %s -x %s -p %s",
					     manifest_path, sig_path,
					     full_path);

		if (system(cmd) == 0) {
			if (verified_key_out)
				*verified_key_out = g_strdup(filename);
			g_free(cmd);
			g_free(full_path);
			g_dir_close(dir);
			return TRUE;
		}

		g_free(cmd);
		g_free(full_path);
	}

	g_dir_close(dir);
	return FALSE;
}

/* ------------------------------------------------------------------------ */
gboolean verify_file_sha256(const gchar *file_path,
			    const gchar *expected_sha256) {
	guchar hash[SHA256_DIGEST_LENGTH];
	gchar hash_string[SHA256_DIGEST_LENGTH * 2 + 1];
	hash_string[sizeof(hash_string) - 1] = '\0';

	FILE *file = fopen(file_path, "rb");
	if (!file) {
		g_printerr("verify_file_sha256: Failed to open file: %s\n",
			   file_path);
		return FALSE;
	}

	SHA256_CTX sha256;
	SHA256_Init(&sha256);

	guchar buffer[4096];
	size_t bytes_read = 0;
	while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
		SHA256_Update(&sha256, buffer, bytes_read);
	}

	fclose(file);
	SHA256_Final(hash, &sha256);

	for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
		sprintf(hash_string + i * 2, "%02x", hash[i]);
	}

	if (g_strcmp0(hash_string, expected_sha256) != 0) {
		g_printerr("verify_file_sha256: Hash mismatch for %s\n",
			   file_path);
		g_printerr("Expected: %s\n", expected_sha256);
		g_printerr("Actual:   %s\n", hash_string);
		return FALSE;
	}

	return TRUE;
}
