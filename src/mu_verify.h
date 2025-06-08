#ifndef MU_VERIFY_H
#define MU_VERIFY_H
#include <glib.h>
#include <stdlib.h>

gboolean verify_manifest_signature(const gchar *manifest_path,
				   const gchar *sig_path,
				   const gchar *trusted_dir,
				   gchar **verified_key_out);

gboolean verify_file_sha256(const gchar *file_path,
			    const gchar *expected_sha256);

#endif // MU_VERIFY_H
