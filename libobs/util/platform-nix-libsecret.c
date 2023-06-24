/*
 * Copyright (c) 2023 Dennis Sädtler <dennis@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <gio/gio.h>
#include <libsecret/secret.h>

#include "platform.h"
#include "bmem.h"

static const SecretSchema obs_schema = {
	"com.obsproject.libobs.Secret",
	SECRET_SCHEMA_NONE,
	{
		{"key", SECRET_SCHEMA_ATTRIBUTE_STRING},
		{"NULL", 0},
	}};

bool os_keychain_available(void)
{
	return true;
}

bool os_keychain_save(const char *label, const char *key, const char *data)
{
	if (!label || !key || !data)
		return false;

	GError *error = NULL;
	secret_password_store_sync(&obs_schema, SECRET_COLLECTION_DEFAULT,
				   label, data, NULL, &error, "key", key, NULL);
	if (error != NULL) {
		blog(LOG_ERROR,
		     "Keychain item \"%s::%s\" could not be saved: %s", label,
		     key, error->message);
		g_error_free(error);
		return false;
	}

	return true;
}

bool os_keychain_load(const char *label, const char *key, char **data)
{
	if (!label || !key || !data)
		return false;

	GError *error = NULL;
	gchar *password = secret_password_lookup_sync(&obs_schema, NULL, &error,
						      "key", key, NULL);

	if (error != NULL) {
		blog(LOG_ERROR,
		     "Keychain item \"%s::%s\" could not be read: %s", label,
		     key, error->message);
		g_error_free(error);
		return false;
	} else if (password == NULL) {
		return false;
	}

	*data = bstrdup(password);
	secret_password_free(password);
	return true;
}

bool os_keychain_delete(const char *label, const char *key)
{
	if (!label || !key)
		return false;

	GError *error = NULL;
	gboolean removed = secret_password_clear_sync(&obs_schema, NULL, &error,
						      "key", key, NULL);

	if (error != NULL) {
		if (error->code == SECRET_ERROR_NO_SUCH_OBJECT) {
			removed = true;
		} else {
			blog(LOG_ERROR,
			     "Keychain item \"%s::%s\" could not be deleted: %s",
			     label, key, error->message);
		}

		g_error_free(error);
	}

	return removed;
}
