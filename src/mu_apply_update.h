#ifndef MU_APPLY_UPDATE_H
#define MU_APPLY_UPDATE_H
#include <glib.h>

gboolean apply_payload(const gchar *inbox_path, const gchar *payload_name,
		       const gchar *destination_path);
gboolean run_post_script(const gchar *script_path);

#endif // MU_APPLY_UPDATE_H
