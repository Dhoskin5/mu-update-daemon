#include "mu_manifest.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------ */
int parse_manifest(const char *json_path, struct manifest *out) {
	FILE *fp = fopen(json_path, "r");
	if (!fp)
		return -1;

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	rewind(fp);

	char *data = malloc(size + 1);
	fread(data, 1, size, fp);
	data[size] = '\0';
	fclose(fp);

	cJSON *root = cJSON_Parse(data);
	free(data);
	if (!root)
		return -2;

	cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
	cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(root, "timestamp");
	cJSON *key_id = cJSON_GetObjectItemCaseSensitive(root, "key_id");
	cJSON *description = cJSON_GetObjectItemCaseSensitive(root,
							      "description");
	cJSON *files = cJSON_GetObjectItemCaseSensitive(root, "files");

	if (!cJSON_IsString(version) || !cJSON_IsString(timestamp) ||
	    !cJSON_IsArray(files)) {
		cJSON_Delete(root);
		return -3;
	}

	strncpy(out->version, version->valuestring, sizeof(out->version));
	strncpy(out->timestamp, timestamp->valuestring, sizeof(out->timestamp));
	strncpy(out->description, description->valuestring,
		sizeof(out->description));
	strncpy(out->key_id, key_id->valuestring, sizeof(out->key_id));

	cJSON *script = cJSON_GetObjectItemCaseSensitive(root, "script");
	out->script_present = cJSON_IsObject(script) &&
			      cJSON_HasObjectItem(script, "name") &&
			      cJSON_HasObjectItem(script, "sha256");
	if (out->script_present) {
		cJSON *script_name = cJSON_GetObjectItemCaseSensitive(script,
								      "name");
		cJSON *script_sha256 =
		    cJSON_GetObjectItemCaseSensitive(script, "sha256");

		if (cJSON_IsString(script_name)) {
			strncpy(out->script.script_name,
				script_name->valuestring,
				sizeof(out->script.script_name));
		} else {
			out->script.script_name[0] = '\0';
		}

		if (cJSON_IsString(script_sha256)) {
			strncpy(out->script.sha256, script_sha256->valuestring,
				sizeof(out->script.sha256));
		} else {
			out->script.sha256[0] = '\0';
		}
	} else {
		// Script block missing entirely
		out->script.script_name[0] = '\0';
		out->script.sha256[0] = '\0';
	}

	out->file_count = 0;
	cJSON *file;
	cJSON_ArrayForEach(file, files) {
		if (out->file_count >= MAX_FILES)
			break;

		cJSON *path = cJSON_GetObjectItem(file, "path");
		cJSON *file_sha256 = cJSON_GetObjectItem(file, "sha256");
		cJSON *payload_name = cJSON_GetObjectItem(file, "name");

		if (cJSON_IsString(path) && cJSON_IsString(file_sha256) &&
		    cJSON_IsString(payload_name)) {
			struct payload_file *f = &out->files[out->file_count++];
			strncpy(f->path, path->valuestring, sizeof(f->path));
			strncpy(f->sha256, file_sha256->valuestring,
				sizeof(f->sha256));
			strncpy(f->payload_name, payload_name->valuestring,
				sizeof(f->payload_name));
		}
	}

	cJSON_Delete(root);
	return 0;
}
