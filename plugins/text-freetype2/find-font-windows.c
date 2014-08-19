#include <ctype.h>
#include <obs.h>
#include <util/dstr.h>
#include <util/darray.h>
#include "find-font.h"
#include "text-freetype2.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

DARRAY(struct font_path_info) font_list;

struct mac_font_mapping {
	unsigned short encoding_id;
	unsigned short language_id;
	unsigned int   code_page;
};

#define TT_MAC_LANGID_ANY 0xFFFF

static const struct mac_font_mapping mac_codes[] = {
	{TT_MAC_ID_ROMAN,              TT_MAC_LANGID_ENGLISH,            10000},
	{TT_MAC_ID_ROMAN,              TT_MAC_LANGID_ICELANDIC,          10079},
	{TT_MAC_ID_ROMAN,              TT_MAC_LANGID_TURKISH,            10081},
	{TT_MAC_ID_ROMAN,              TT_MAC_LANGID_POLISH,             10029},
	{TT_MAC_ID_ROMAN,              TT_MAC_LANGID_ROMANIAN,           10010},
	{TT_MAC_ID_ROMAN,              TT_MAC_LANGID_CZECH,              10029},
	{TT_MAC_ID_ROMAN,              TT_MAC_LANGID_SLOVAK,             10029},
	{TT_MAC_ID_ROMAN,              TT_MAC_LANGID_ANY,                10000},
	{TT_MAC_ID_JAPANESE,           TT_MAC_LANGID_JAPANESE,           932},
	{TT_MAC_ID_JAPANESE,           TT_MAC_LANGID_ANY,                932},
	{TT_MAC_ID_TRADITIONAL_CHINESE,TT_MAC_LANGID_CHINESE_SIMPLIFIED, 950},
	{TT_MAC_ID_TRADITIONAL_CHINESE,TT_MAC_LANGID_ANY,                950},
	{TT_MAC_ID_KOREAN,             TT_MAC_LANGID_KOREAN,             51949},
	{TT_MAC_ID_KOREAN,             TT_MAC_LANGID_ANY,                51949},
	{TT_MAC_ID_ARABIC,             TT_MAC_LANGID_ARABIC,             10004},
	{TT_MAC_ID_ARABIC,             TT_MAC_LANGID_URDU,               0},
	{TT_MAC_ID_ARABIC,             TT_MAC_LANGID_FARSI,              0},
	{TT_MAC_ID_ARABIC,             TT_MAC_LANGID_ANY,                10004},
	{TT_MAC_ID_HEBREW,             TT_MAC_LANGID_HEBREW,             10005},
	{TT_MAC_ID_HEBREW,             TT_MAC_LANGID_ANY,                10005},
	{TT_MAC_ID_GREEK,              TT_MAC_LANGID_ANY,                10006},
	{TT_MAC_ID_RUSSIAN,            TT_MAC_LANGID_ANY,                10007},
	{TT_MAC_ID_DEVANAGARI,         TT_MAC_LANGID_ANY,                0},
	{TT_MAC_ID_GURMUKHI,           TT_MAC_LANGID_ANY,                0},
	{TT_MAC_ID_GUJARATI,           TT_MAC_LANGID_ANY,                0},
	{TT_MAC_ID_SIMPLIFIED_CHINESE, TT_MAC_LANGID_CHINESE_SIMPLIFIED, 936},
	{TT_MAC_ID_SIMPLIFIED_CHINESE, TT_MAC_LANGID_ANY,                936}
};

unsigned int iso_codes[] = {
	20127,
	0,
	28591
};

unsigned int ms_codes[] = {
	1201,
	1201,
	932,
	0,
	950,
	0,
	0,
	0,
	0,
	0,
	1201
};

static const size_t mac_code_count = sizeof(mac_codes) / sizeof(mac_codes[0]);
static const size_t iso_code_count = sizeof(iso_codes) / sizeof(iso_codes[0]);
static const size_t ms_code_count  = sizeof(ms_codes)  / sizeof(ms_codes[0]);

static unsigned int get_mac_code(uint16_t encoding_id, uint16_t language_id)
{
	for (size_t i = 0; i < mac_code_count; i++) {
		const struct mac_font_mapping *mac_code = &mac_codes[i];

		if (mac_code->encoding_id == encoding_id &&
		    mac_code->language_id == language_id)
			return mac_code->code_page;
	}

	return 0;
}

static unsigned int get_code_page_for_font(uint16_t platform_id,
		uint16_t encoding_id, uint16_t language_id)
{
	unsigned int ret;

	switch (platform_id) {
	case TT_PLATFORM_APPLE_UNICODE:
		return 1201;
	case TT_PLATFORM_MACINTOSH:
		ret = get_mac_code(encoding_id, language_id);
		if (!ret)
			ret = get_mac_code(encoding_id, TT_MAC_LANGID_ANY);
		return ret;
	case TT_PLATFORM_ISO:
		if (encoding_id < iso_code_count)
			return iso_codes[encoding_id];
		break;
	case TT_PLATFORM_MICROSOFT:
		if (encoding_id < ms_code_count)
			return ms_codes[encoding_id];
		break;
	}

	return 0;
}

static char *wide_to_utf8(const wchar_t *str, size_t len)
{
	size_t utf8_len;
	char   *utf8_str = NULL;

	utf8_len = (size_t)WideCharToMultiByte(CP_UTF8, 0, str, (int)len,
			NULL, 0, NULL, false);
	if (utf8_len) {
		utf8_str = bzalloc(utf8_len + 1);
		utf8_len = (size_t)WideCharToMultiByte(CP_UTF8, 0,
				str, (int)len,
				utf8_str, (int)utf8_len + 1, NULL, false);

		if (!utf8_len) {
			bfree(utf8_str);
			utf8_str = NULL;
		}
	}

	return utf8_str;
}

static char *convert_utf16_be_to_utf8(FT_SfntName *sfnt_name)
{
	size_t  utf16_len  = sfnt_name->string_len / 2;
	wchar_t *utf16_str = malloc((utf16_len + 1) * sizeof(wchar_t));
	char    *utf8_str  = NULL;

	utf16_str[utf16_len] = 0;

	/* convert to little endian */
	for (size_t i = 0; i < utf16_len; i++) {
		size_t  pos = i * 2;
		wchar_t ch  = *(wchar_t *)&sfnt_name->string[pos];

		utf16_str[i] = ((ch >> 8) & 0xFF) | ((ch << 8) & 0xFF00);
	}

	utf8_str = wide_to_utf8(utf16_str, utf16_len);

	free(utf16_str);
	return utf8_str;
}

static char *sfnt_name_to_utf8(FT_SfntName *sfnt_name)
{
	unsigned int code_page = get_code_page_for_font(
			sfnt_name->platform_id,
			sfnt_name->encoding_id,
			sfnt_name->language_id);

	char    *utf8_str = NULL;
	wchar_t *utf16_str;
	size_t  utf16_len;

	if (code_page == 1201)
		return convert_utf16_be_to_utf8(sfnt_name);
	else if (code_page == 0)
		return NULL;

	utf16_len = MultiByteToWideChar(code_page, 0,
			sfnt_name->string, sfnt_name->string_len, NULL, 0);
	if (utf16_len) {
		utf16_str = malloc((utf16_len + 1) * sizeof(wchar_t));
		utf16_len = MultiByteToWideChar(code_page, 0,
				sfnt_name->string, sfnt_name->string_len,
				utf16_str, (int)utf16_len);

		if (utf16_len) {
			utf16_str[utf16_len] = 0;
			utf8_str = wide_to_utf8(utf16_str, utf16_len);
		}

		free(utf16_str);
	}

	return utf8_str;
}

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
	if (!family_in || !style_in || !path)
		return;

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

static void build_font_path_info(FT_Face face, FT_Long idx, const char *path)
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

void load_os_font_list(void)
{
	struct dstr      path = {0};
	HANDLE           handle;
	WIN32_FIND_DATAA wfd;

	dstr_reserve(&path, MAX_PATH);

	HRESULT res = SHGetFolderPathA(NULL, CSIDL_FONTS, NULL,
			SHGFP_TYPE_CURRENT, path.array);
	if (res != S_OK) {
		blog(LOG_WARNING, "Error finding windows font folder");
		return;
	}

	path.len = strlen(path.array);
	dstr_cat(&path, "\\*.*");

	handle = FindFirstFileA(path.array, &wfd);
	if (handle == INVALID_HANDLE_VALUE)
		goto free_string;

	dstr_resize(&path, path.len - 4);

	do {
		struct dstr full_path = {0};
		FT_Face face;
		FT_Long idx = 0;
		FT_Long max_faces = 1;

		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		dstr_copy_dstr(&full_path, &path);
		dstr_cat(&full_path, "\\");
		dstr_cat(&full_path, wfd.cFileName);

		while (idx < max_faces) {
			FT_Error ret = FT_New_Face(ft2_lib, full_path.array,
					idx, &face);
			if (ret != 0)
				break;

			build_font_path_info(face, idx++, full_path.array);
			max_faces = face->num_faces;
			FT_Done_Face(face);
		}

		dstr_free(&full_path);
	} while (FindNextFileA(handle, &wfd));

	FindClose(handle);

free_string:
	dstr_free(&path);
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
