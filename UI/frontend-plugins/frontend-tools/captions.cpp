#include <obs-frontend-api.h>
#include "captions-stream.hpp"
#include "captions.hpp"
#include "tool-helpers.hpp"
#include <sphelper.h>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>
#include <util/windows/CoTaskMemPtr.hpp>
#include <util/threading.h>
#include <obs-module.h>

#include <string>
#include <thread>
#include <mutex>

#define do_log(type, format, ...) blog(type, "[Captions] " format, \
		##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

using namespace std;

struct obs_captions {
	thread th;
	recursive_mutex m;
	WinHandle stop_event;

	string source_name;
	OBSWeakSource source;
	LANGID lang_id;

	void main_thread();
	void start();
	void stop();

	inline obs_captions() :
		stop_event(CreateEvent(nullptr, false, false, nullptr)),
		lang_id(GetUserDefaultUILanguage())
	{
	}

	inline ~obs_captions() {stop();}
};

static obs_captions *captions = nullptr;

/* ------------------------------------------------------------------------- */

struct locale_info {
	DStr name;
	LANGID id;

	inline locale_info() {}
	inline locale_info(const locale_info &) = delete;
	inline locale_info(locale_info &&li)
		: name(std::move(li.name)),
		  id(li.id)
	{}
};

static void get_valid_locale_names(vector<locale_info> &names);
static bool valid_lang(LANGID id);

/* ------------------------------------------------------------------------- */

CaptionsDialog::CaptionsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui_CaptionsDialog)
{
	ui->setupUi(this);

	lock_guard<recursive_mutex> lock(captions->m);

	auto cb = [this] (obs_source_t *source)
	{
		uint32_t caps = obs_source_get_output_flags(source);
		QString name = obs_source_get_name(source);

		if (caps & OBS_SOURCE_AUDIO)
			ui->source->addItem(name);

		OBSWeakSource weak = OBSGetWeakRef(source);
		if (weak == captions->source)
			ui->source->setCurrentText(name);
		return true;
	};

	using cb_t = decltype(cb);

	ui->source->blockSignals(true);
	ui->source->addItem(QStringLiteral(""));
	ui->source->setCurrentIndex(0);
	obs_enum_sources([] (void *data, obs_source_t *source) {
			return (*static_cast<cb_t*>(data))(source);}, &cb);
	ui->source->blockSignals(false);

	ui->enable->blockSignals(true);
	ui->enable->setChecked(captions->th.joinable());
	ui->enable->blockSignals(false);

	vector<locale_info> locales;
	get_valid_locale_names(locales);

	bool set_language = false;

	ui->language->blockSignals(true);
	for (int idx = 0; idx < (int)locales.size(); idx++) {
		locale_info &locale = locales[idx];

		ui->language->addItem(locale.name->array, (int)locale.id);

		if (locale.id == captions->lang_id) {
			ui->language->setCurrentIndex(idx);
			set_language = true;
		}
	}

	if (!set_language && locales.size())
		ui->language->setCurrentIndex(0);

	ui->language->blockSignals(false);

	if (!locales.size()) {
		ui->source->setEnabled(false);
		ui->enable->setEnabled(false);
		ui->language->setEnabled(false);

	} else if (!set_language) {
		bool started = captions->th.joinable();
		if (started)
			captions->stop();

		captions->m.lock();
		captions->lang_id = locales[0].id;
		captions->m.unlock();

		if (started)
			captions->start();
	}
}

void CaptionsDialog::on_source_currentIndexChanged(int)
{
	bool started = captions->th.joinable();
	if (started)
		captions->stop();

	captions->m.lock();
	captions->source_name = ui->source->currentText().toUtf8().constData();
	captions->source = GetWeakSourceByName(captions->source_name.c_str());
	captions->m.unlock();

	if (started)
		captions->start();
}

void CaptionsDialog::on_enable_clicked(bool checked)
{
	if (checked)
		captions->start();
	else
		captions->stop();
}

void CaptionsDialog::on_language_currentIndexChanged(int)
{
	bool started = captions->th.joinable();
	if (started)
		captions->stop();

	captions->m.lock();
	captions->lang_id = (LANGID)ui->language->currentData().toInt();
	captions->m.unlock();

	if (started)
		captions->start();
}

/* ------------------------------------------------------------------------- */

void obs_captions::main_thread()
try {
	ComPtr<CaptionStream>  audio;
	ComPtr<ISpObjectToken> token;
	ComPtr<ISpRecoGrammar> grammar;
	ComPtr<ISpRecognizer>  recognizer;
	ComPtr<ISpRecoContext> context;
	HRESULT hr;

	auto cb = [&] (const struct audio_data *audio_data,
			bool muted)
	{
		audio->PushAudio(audio_data, muted);
	};

	using cb_t = decltype(cb);

	auto pre_cb = [] (void *param, obs_source_t*,
		const struct audio_data *audio_data, bool muted)
	{
		return (*static_cast<cb_t*>(param))(audio_data, muted);
	};

	os_set_thread_name(__FUNCTION__);

	CoInitialize(nullptr);

	wchar_t lang_str[32];
	_snwprintf(lang_str, 31, L"language=%x", (int)captions->lang_id);

	hr = SpFindBestToken(SPCAT_RECOGNIZERS, lang_str, nullptr, &token);
	if (FAILED(hr))
		throw HRError("SpFindBestToken failed", hr);

	hr = CoCreateInstance(CLSID_SpInprocRecognizer, nullptr, CLSCTX_ALL,
			__uuidof(ISpRecognizer), (void**)&recognizer);
	if (FAILED(hr))
		throw HRError("CoCreateInstance for recognizer failed", hr);

	hr = recognizer->SetRecognizer(token);
	if (FAILED(hr))
		throw HRError("SetRecognizer failed", hr);

	hr = recognizer->SetRecoState(SPRST_INACTIVE);
	if (FAILED(hr))
		throw HRError("SetRecoState(SPRST_INACTIVE) failed", hr);

	hr = recognizer->CreateRecoContext(&context);
	if (FAILED(hr))
		throw HRError("CreateRecoContext failed", hr);

	ULONGLONG interest = SPFEI(SPEI_RECOGNITION) |
		SPFEI(SPEI_END_SR_STREAM);
	hr = context->SetInterest(interest, interest);
	if (FAILED(hr))
		throw HRError("SetInterest failed", hr);

	HANDLE notify;

	hr = context->SetNotifyWin32Event();
	if (FAILED(hr))
		throw HRError("SetNotifyWin32Event", hr);

	notify = context->GetNotifyEventHandle();
	if (notify == INVALID_HANDLE_VALUE)
		throw HRError("GetNotifyEventHandle failed", E_NOINTERFACE);

	size_t sample_rate = audio_output_get_sample_rate(obs_get_audio());
	audio = new CaptionStream((DWORD)sample_rate);
	audio->Release();

	hr = recognizer->SetInput(audio, false);
	if (FAILED(hr))
		throw HRError("SetInput failed", hr);

	hr = context->CreateGrammar(1, &grammar);
	if (FAILED(hr))
		throw HRError("CreateGrammar failed", hr);

	hr = grammar->LoadDictation(nullptr, SPLO_STATIC);
	if (FAILED(hr))
		throw HRError("LoadDictation failed", hr);

	hr = grammar->SetDictationState(SPRS_ACTIVE);
	if (FAILED(hr))
		throw HRError("SetDictationState failed", hr);

	hr = recognizer->SetRecoState(SPRST_ACTIVE);
	if (FAILED(hr))
		throw HRError("SetRecoState(SPRST_ACTIVE) failed", hr);

	HANDLE events[] = {notify, stop_event};

	{
		captions->source = GetWeakSourceByName(
				captions->source_name.c_str());
		OBSSource strong = OBSGetStrongRef(source);
		if (strong)
			obs_source_add_audio_capture_callback(strong,
					pre_cb, &cb);
	}

	for (;;) {
		DWORD ret = WaitForMultipleObjects(2, events, false, INFINITE);
		if (ret != WAIT_OBJECT_0)
			break;

		CSpEvent event;
		bool exit = false;

		while (event.GetFrom(context) == S_OK) {
			if (event.eEventId == SPEI_RECOGNITION) {
				ISpRecoResult *result = event.RecoResult();

				CoTaskMemPtr<wchar_t> text;
				hr = result->GetText((ULONG)-1, (ULONG)-1,
						true, &text, nullptr);
				if (FAILED(hr))
					continue;

				char text_utf8[512];
				os_wcs_to_utf8(text, 0, text_utf8, 512);

				obs_output_t *output =
					obs_frontend_get_streaming_output();
				if (output)
					obs_output_output_caption_text1(output,
							text_utf8);

				debug("\"%s\"", text_utf8);

				obs_output_release(output);

			} else if (event.eEventId == SPEI_END_SR_STREAM) {
				exit = true;
				break;
			}
		}

		if (exit)
			break;
	}

	{
		OBSSource strong = OBSGetStrongRef(source);
		if (strong)
			obs_source_remove_audio_capture_callback(strong,
					pre_cb, &cb);
	}

	audio->Stop();

	CoUninitialize();

} catch (HRError err) {
	error("%s failed: %s (%lX)", __FUNCTION__, err.str, err.hr);
	CoUninitialize();
	captions->th.detach();
}

void obs_captions::start()
{
	if (!captions->th.joinable()) {
		ResetEvent(captions->stop_event);

		if (valid_lang(captions->lang_id))
			captions->th = thread([] () {captions->main_thread();});
	}
}

void obs_captions::stop()
{
	if (!captions->th.joinable())
		return;

	SetEvent(captions->stop_event);
	captions->th.join();
}

static bool get_locale_name(LANGID id, char *out)
{
	wchar_t name[256];

	int size = GetLocaleInfoW(id, LOCALE_SENGLISHLANGUAGENAME, name, 256);
	if (size <= 0)
		return false;

	os_wcs_to_utf8(name, 0, out, 256);
	return true;
}

static bool valid_lang(LANGID id)
{
	ComPtr<ISpObjectToken> token;
	wchar_t lang_str[32];
	HRESULT hr;

	_snwprintf(lang_str, 31, L"language=%x", (int)id);

	hr = SpFindBestToken(SPCAT_RECOGNIZERS, lang_str, nullptr, &token);
	return SUCCEEDED(hr);
}

static void get_valid_locale_names(vector<locale_info> &locales)
{
	locale_info cur;
	char locale_name[256];

	static const LANGID default_locales[] = {
		0x0409,
		0x0401,
		0x0402,
		0x0403,
		0x0404,
		0x0405,
		0x0406,
		0x0407,
		0x0408,
		0x040a,
		0x040b,
		0x040c,
		0x040d,
		0x040e,
		0x040f,
		0x0410,
		0x0411,
		0x0412,
		0x0413,
		0x0414,
		0x0415,
		0x0416,
		0x0417,
		0x0418,
		0x0419,
		0x041a,
		0
	};

	/* ---------------------------------- */

	LANGID def_id = GetUserDefaultUILanguage();
	LANGID id = def_id;
	if (valid_lang(id) && get_locale_name(id, locale_name)) {
		dstr_copy(cur.name, obs_module_text(
					"Captions.CurrentSystemLanguage"));
		dstr_replace(cur.name, "%1", locale_name);
		cur.id = id;

		locales.push_back(std::move(cur));
	}

	/* ---------------------------------- */

	const LANGID *locale = default_locales;

	while (*locale) {
		id = *locale;

		if (id != def_id &&
		    valid_lang(id) &&
		    get_locale_name(id, locale_name)) {

			dstr_copy(cur.name, locale_name);
			cur.id = id;

			locales.push_back(std::move(cur));
		}

		locale++;
	}
}

/* ------------------------------------------------------------------------- */

extern "C" void FreeCaptions()
{
	delete captions;
	captions = nullptr;
}

static void obs_event(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_EXIT)
		FreeCaptions();
}

static void save_caption_data(obs_data_t *save_data, bool saving, void*)
{
	if (saving) {
		lock_guard<recursive_mutex> lock(captions->m);
		obs_data_t *obj = obs_data_create();

		obs_data_set_string(obj, "source",
				captions->source_name.c_str());
		obs_data_set_bool(obj, "enabled", captions->th.joinable());
		obs_data_set_int(obj, "lang_id", captions->lang_id);

		obs_data_set_obj(save_data, "captions", obj);
		obs_data_release(obj);
	} else {
		captions->stop();

		captions->m.lock();

		obs_data_t *obj = obs_data_get_obj(save_data, "captions");
		if (!obj)
			obj = obs_data_create();

		obs_data_set_default_int(obj, "lang_id",
				GetUserDefaultUILanguage());

		bool enabled = obs_data_get_bool(obj, "enabled");
		captions->source_name = obs_data_get_string(obj, "source");
		captions->lang_id = (int)obs_data_get_int(obj, "lang_id");
		captions->source = GetWeakSourceByName(
				captions->source_name.c_str());
		obs_data_release(obj);

		captions->m.unlock();

		if (enabled)
			captions->start();
	}
}

extern "C" void InitCaptions()
{
	QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(
			obs_module_text("Captions"));

	captions = new obs_captions;

	auto cb = [] ()
	{
		obs_frontend_push_ui_translation(obs_module_get_string);

		QWidget *window =
			(QWidget*)obs_frontend_get_main_window();

		CaptionsDialog dialog(window);
		dialog.exec();

		obs_frontend_pop_ui_translation();
	};

	obs_frontend_add_save_callback(save_caption_data, nullptr);
	obs_frontend_add_event_callback(obs_event, nullptr);

	action->connect(action, &QAction::triggered, cb);
}
