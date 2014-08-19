#include <ctype.h>
#include <obs.h>
#include "find-font.h"

DARRAY(struct font_path_info) font_list;

static void create_bitmap_sizes(struct font_path_info *info, FT_Face face)
{
	DARRAY(int) sizes;

	if (!info->is_bitmap) {
		info->num_sizes = 0;
		info->sizes     = NULL;
		return;
	}

	da_init(sizes);
	da_reserve(sizes, face->num_fixed_sizes);

	for (int i = 0; i < face->num_fixed_sizes; i++) {
		int val = face->available_sizes[i].size >> 6;
		da_push_back(sizes, &val);
	}

	info->sizes     = sizes.array;
	info->num_sizes = face->num_fixed_sizes;
}

static void add_font_path(FT_Face face,
		FT_Long idx,
		const char *family_in,
		const char *style_in,
		const char *path)
{
	struct dstr face_and_style = {0};
	struct font_path_info info;

	dstr_copy(&face_and_style, family_in);
	if (face->style_name) {
		struct dstr style = {0};

		dstr_copy(&style, style_in);
		dstr_replace(&style, "Bold", "");
		dstr_replace(&style, "Italic", "");
		dstr_replace(&style, "  ", " ");
		dstr_depad(&style);

		if (!dstr_is_empty(&style)) {
			dstr_cat(&face_and_style, " ");
			dstr_cat_dstr(&face_and_style, &style);
		}

		dstr_free(&style);
	}

	info.face_and_style = face_and_style.array;
	info.full_len       = face_and_style.len;
	info.face_len       = strlen(family_in);

	info.is_bitmap      = !!(face->face_flags  & FT_FACE_FLAG_FIXED_SIZES);
	info.bold           = !!(face->style_flags & FT_STYLE_FLAG_BOLD);
	info.italic         = !!(face->style_flags & FT_STYLE_FLAG_ITALIC);
	info.index          = idx;

	info.path           = bstrdup(path);

	create_bitmap_sizes(&info, face);
	da_push_back(font_list, &info);

	/*blog(LOG_DEBUG, "name: %s\n\tstyle: %s\n\tpath: %s\n",
			family_in,
			style_in,
			path);*/
}

void build_font_path_info(FT_Face face, FT_Long idx, const char *path)
{
	FT_UInt num_names = FT_Get_Sfnt_Name_Count(face);
	DARRAY(char*) family_names;

	da_init(family_names);
	da_push_back(family_names, &face->family_name);

	for (FT_UInt i = 0; i < num_names; i++) {
		FT_SfntName name;
		char        *family;
		FT_Error    ret = FT_Get_Sfnt_Name(face, i, &name);

		if (ret != 0 || name.name_id != TT_NAME_ID_FONT_FAMILY)
			continue;

		family = sfnt_name_to_utf8(&name);
		if (!family)
			continue;

		for (size_t i = 0; i < family_names.num; i++) {
			if (astrcmpi(family_names.array[i], family) == 0) {
				bfree(family);
				family = NULL;
				break;
			}
		}

		if (family)
			da_push_back(family_names, &family);
	}

	for (size_t i = 0; i < family_names.num; i++) {
		add_font_path(face, idx, family_names.array[i],
				face->style_name, path);

		/* first item isn't our allocation */
		if (i > 0)
			bfree(family_names.array[i]);
	}

	da_free(family_names);
}

void free_os_font_list(void)
{
	for (size_t i = 0; i < font_list.num; i++)
		font_path_info_free(font_list.array + i);
	da_free(font_list);
}

static inline size_t get_rating(struct font_path_info *info, struct dstr *cmp)
{
	const char *src = info->face_and_style;
	const char *dst = cmp->array;
	size_t num = 0;

	do {
		char ch1 = (char)toupper(*src);
		char ch2 = (char)toupper(*dst);

		if (ch1 != ch2)
			break;

		num++;
	} while (*src++ && *dst++);

	return num;
}

const char *get_font_path(const char *family, uint16_t size, const char *style,
		uint32_t flags, FT_Long *idx)
{
	const char  *best_path     = NULL;
	double      best_rating    = 0.0;
	struct dstr face_and_style = {0};
	struct dstr style_str      = {0};
	bool        bold           = !!(flags & OBS_FONT_BOLD);
	bool        italic         = !!(flags & OBS_FONT_ITALIC);

	if (!family)
		return NULL;

	dstr_copy(&style_str, style);
	dstr_replace(&style_str, "Bold", "");
	dstr_replace(&style_str, "Italic", "");
	dstr_replace(&style_str, "  ", " ");
	dstr_depad(&style_str);

	dstr_copy(&face_and_style, family);
	if (!dstr_is_empty(&style_str)) {
		dstr_cat(&face_and_style, " ");
		dstr_cat_dstr(&face_and_style, &style_str);
	}

	for (size_t i = 0; i < font_list.num; i++) {
		struct font_path_info *info = font_list.array + i;

		double rating = (double)get_rating(info, &face_and_style);
		if (rating < info->face_len)
			continue;

		if (info->is_bitmap) {
			int best_diff = 1000;
			for (size_t j = 0; j < info->num_sizes; j++) {
				int diff = abs(info->sizes[j] - size);
				if (diff < best_diff)
					best_diff = diff;
			}

			rating /= (double)(best_diff + 1.0);
		}

		if (info->bold   == bold)   rating += 1.0;
		if (info->italic == italic) rating += 1.0;

		if (rating > best_rating) {
			best_path   = info->path;
			*idx        = info->index;
			best_rating = rating;
		}
	}

	dstr_free(&style_str);
	dstr_free(&face_and_style);
	return best_path;
}
