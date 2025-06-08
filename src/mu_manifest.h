#ifndef MU_MANIFEST_H
#define MU_MANIFEST_H

#include <stdbool.h>

#define MAX_FILES 8

struct payload_file {
	char payload_name[256];
	char path[256];
	char sha256[65]; // SHA-256 = 64 hex chars + null
};

struct script_file {
	char script_name[256];
	char sha256[65]; // SHA-256 = 64 hex chars + null
};

struct manifest {
	char version[32];
	char timestamp[32];
	char key_id[32];
	char description[256];
	int file_count;
	struct payload_file files[MAX_FILES];
	bool script_present;
	struct script_file script;
};

int parse_manifest(const char *json_path, struct manifest *out);

#endif
