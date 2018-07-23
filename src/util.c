/*
 * util.c
 *
 *  Created on: 13.04.2018
 *      Author: dk
 */
#include "mgos.h"

char* get_config_by_key(char *key) {
	const struct mgos_conf_entry *schema;
	schema = mgos_conf_find_schema_entry(key, mgos_config_schema());
	if (!schema)
		return NULL;
	return (char *) mgos_conf_value_string(&mgos_sys_config, schema);
}

