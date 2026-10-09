/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "OBSBasicInteraction.hpp"

#include <dialogs/OBSBasicProperties.hpp>
#include <utility/OBSEventFilter.hpp>
#include <utility/display-helpers.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QInputEvent>
#include <QInputMethod>
#include <QTextCharFormat>
#include <QTimer>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
#endif

#include "moc_OBSBasicInteraction.cpp"

using namespace std;

OBSBasicInteraction::OBSBasicInteraction(QWidget *parent, OBSSource source_)
	: QDialog(parent),
	  main(qobject_cast<OBSBasic *>(parent)),
	  ui(new Ui::OBSBasicInteraction),
	  source(source_),
	  removedSignal(obs_source_get_signal_handler(source), "remove", OBSBasicInteraction::SourceRemoved, this),
	  renamedSignal(obs_source_get_signal_handler(source), "rename", OBSBasicInteraction::SourceRenamed, this),
	  eventFilter(BuildEventFilter())
{
	int cx = (int)config_get_int(App()->GetAppConfig(), "InteractionWindow", "cx");
	int cy = (int)config_get_int(App()->GetAppConfig(), "InteractionWindow", "cy");

	Qt::WindowFlags flags = windowFlags();
	Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags & (~helpFlag));

	ui->setupUi(this);

	ui->preview->setMouseTracking(true);
	ui->preview->setFocusPolicy(Qt::StrongFocus);
	ui->preview->installEventFilter(eventFilter.get());
#ifdef _WIN32
	// Only opt in sources implementing the new composition-aware interface.
	imeEnabled = obs_source_supports_ime(source);
	ui->preview->setAttribute(Qt::WA_InputMethodEnabled, imeEnabled);
	if (imeEnabled) {
		auto *imeTimer = new QTimer(this);
		connect(imeTimer, &QTimer::timeout, this, [this]() {
			if (composing && imeGeneration != obs_source_get_ime_generation(source)) {
				CancelComposition();
			}
			if (composing && ui->preview->hasFocus()) {
				QGuiApplication::inputMethod()->update(Qt::ImCursorRectangle);
			}
		});
		imeTimer->start(50);
	}
#endif

	if (cx > 400 && cy > 400) {
		resize(cx, cy);
	}

	const char *name = obs_source_get_name(source);
	setWindowTitle(QTStr("Basic.InteractionWindow").arg(QT_UTF8(name)));

	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(ui->preview->GetDisplay(), OBSBasicInteraction::DrawPreview, this);
	};

	connect(ui->preview, &OBSQTDisplay::DisplayCreated, this, addDrawCallback);
}

OBSBasicInteraction::~OBSBasicInteraction()
{
	CancelComposition();
	// since QT fakes a mouse movement while destructing a widget
	// remove our event filter
	ui->preview->removeEventFilter(eventFilter.get());
}

OBSEventFilter *OBSBasicInteraction::BuildEventFilter()
{
	return new OBSEventFilter([this](QObject *, QEvent *event) {
		switch (event->type()) {
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseButtonDblClick:
			return this->HandleMouseClickEvent(static_cast<QMouseEvent *>(event));
		case QEvent::MouseMove:
		case QEvent::Enter:
		case QEvent::Leave:
			return this->HandleMouseMoveEvent(static_cast<QMouseEvent *>(event));

		case QEvent::Wheel:
			return this->HandleMouseWheelEvent(static_cast<QWheelEvent *>(event));
		case QEvent::FocusIn:
		case QEvent::FocusOut:
			return this->HandleFocusEvent(static_cast<QFocusEvent *>(event));
		case QEvent::InputMethod:
			return HandleInputMethodEvent(static_cast<QInputMethodEvent *>(event));
		case QEvent::InputMethodQuery:
			return HandleInputMethodQuery(static_cast<QInputMethodQueryEvent *>(event));
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
			return this->HandleKeyEvent(static_cast<QKeyEvent *>(event));
		default:
			return false;
		}
	});
}

void OBSBasicInteraction::SourceRemoved(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicInteraction *>(data), &OBSBasicInteraction::close);
}

void OBSBasicInteraction::SourceRenamed(void *data, calldata_t *params)
{
	const char *name = calldata_string(params, "new_name");
	QString title = QTStr("Basic.InteractionWindow").arg(QT_UTF8(name));

	QMetaObject::invokeMethod(static_cast<OBSBasicProperties *>(data), &OBSBasicInteraction::setWindowTitle, title);
}

void OBSBasicInteraction::DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
	OBSBasicInteraction *window = static_cast<OBSBasicInteraction *>(data);

	if (!window->source) {
		return;
	}

	uint32_t sourceCX = max(obs_source_get_width(window->source), 1u);
	uint32_t sourceCY = max(obs_source_get_height(window->source), 1u);

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	const bool previous = gs_set_linear_srgb(true);

	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);
	obs_source_video_render(window->source);

	gs_set_linear_srgb(previous);
	gs_projection_pop();
	gs_viewport_pop();
}

void OBSBasicInteraction::closeEvent(QCloseEvent *event)
{
	QDialog::closeEvent(event);
	if (!event->isAccepted()) {
		return;
	}

	CancelComposition();
	config_set_int(App()->GetAppConfig(), "InteractionWindow", "cx", width());
	config_set_int(App()->GetAppConfig(), "InteractionWindow", "cy", height());

	obs_display_remove_draw_callback(ui->preview->GetDisplay(), OBSBasicInteraction::DrawPreview, this);
}

bool OBSBasicInteraction::nativeEvent(const QByteArray &, void *message, qintptr *)
{
#ifdef _WIN32
	const MSG &msg = *static_cast<MSG *>(message);
	switch (msg.message) {
	case WM_MOVE:
		for (OBSQTDisplay *const display : findChildren<OBSQTDisplay *>()) {
			display->OnMove();
		}
		break;
	case WM_DISPLAYCHANGE:
		for (OBSQTDisplay *const display : findChildren<OBSQTDisplay *>()) {
			display->OnDisplayChange();
		}
	}
#else
	UNUSED_PARAMETER(message);
#endif

	return false;
}

static int TranslateQtKeyboardEventModifiers(QInputEvent *event, bool mouseEvent)
{
	int obsModifiers = INTERACT_NONE;

	if (event->modifiers().testFlag(Qt::ShiftModifier)) {
		obsModifiers |= INTERACT_SHIFT_KEY;
	}
	if (event->modifiers().testFlag(Qt::AltModifier)) {
		obsModifiers |= INTERACT_ALT_KEY;
	}
#ifdef __APPLE__
	// Mac: Meta = Control, Control = Command
	if (event->modifiers().testFlag(Qt::ControlModifier)) {
		obsModifiers |= INTERACT_COMMAND_KEY;
	}
	if (event->modifiers().testFlag(Qt::MetaModifier)) {
		obsModifiers |= INTERACT_CONTROL_KEY;
	}
#else
	// Handle windows key? Can a browser even trap that key?
	if (event->modifiers().testFlag(Qt::ControlModifier)) {
		obsModifiers |= INTERACT_CONTROL_KEY;
	}
#endif

	if (!mouseEvent) {
		if (event->modifiers().testFlag(Qt::KeypadModifier)) {
			obsModifiers |= INTERACT_IS_KEY_PAD;
		}
	}

	return obsModifiers;
}

static int TranslateQtMouseEventModifiers(QMouseEvent *event)
{
	int modifiers = TranslateQtKeyboardEventModifiers(event, true);

	if (event->buttons().testFlag(Qt::LeftButton)) {
		modifiers |= INTERACT_MOUSE_LEFT;
	}
	if (event->buttons().testFlag(Qt::MiddleButton)) {
		modifiers |= INTERACT_MOUSE_MIDDLE;
	}
	if (event->buttons().testFlag(Qt::RightButton)) {
		modifiers |= INTERACT_MOUSE_RIGHT;
	}

	return modifiers;
}

bool OBSBasicInteraction::GetSourceRelativeXY(int mouseX, int mouseY, int &relX, int &relY)
{
	float pixelRatio = devicePixelRatioF();
	int mouseXscaled = (int)roundf(mouseX * pixelRatio);
	int mouseYscaled = (int)roundf(mouseY * pixelRatio);

	QSize size = GetPixelSize(ui->preview);

	uint32_t sourceCX = max(obs_source_get_width(source), 1u);
	uint32_t sourceCY = max(obs_source_get_height(source), 1u);

	int x, y;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, size.width(), size.height(), x, y, scale);

	if (x > 0) {
		relX = int(float(mouseXscaled - x) / scale);
		relY = int(float(mouseYscaled / scale));
	} else {
		relX = int(float(mouseXscaled / scale));
		relY = int(float(mouseYscaled - y) / scale);
	}

	// Confirm mouse is inside the source
	if (relX < 0 || relX > int(sourceCX)) {
		return false;
	}
	if (relY < 0 || relY > int(sourceCY)) {
		return false;
	}

	return true;
}

bool OBSBasicInteraction::HandleMouseClickEvent(QMouseEvent *event)
{
	bool mouseUp = event->type() == QEvent::MouseButtonRelease;
	int clickCount = 1;
	if (event->type() == QEvent::MouseButtonDblClick) {
		clickCount = 2;
	}

	struct obs_mouse_event mouseEvent = {};

	mouseEvent.modifiers = TranslateQtMouseEventModifiers(event);

	int32_t button = 0;

	switch (event->button()) {
	case Qt::LeftButton:
		button = MOUSE_LEFT;
		break;
	case Qt::MiddleButton:
		button = MOUSE_MIDDLE;
		break;
	case Qt::RightButton:
		button = MOUSE_RIGHT;
		break;
	default:
		blog(LOG_WARNING, "unknown button type %d", event->button());
		return false;
	}

	// Why doesn't this work?
	//if (event->flags().testFlag(Qt::MouseEventCreatedDoubleClick))
	//	clickCount = 2;

	QPoint pos = event->pos();
	bool insideSource = GetSourceRelativeXY(pos.x(), pos.y(), mouseEvent.x, mouseEvent.y);

	if (!mouseUp && insideSource) {
		if (imeEnabled && composing && event->button() == Qt::LeftButton) {
			// Windows Qt reset commits synchronously to the current editor.
			// Finish it before CEF can move focus; a later commit must not
			// insert the old composition into the clicked input field.
			QGuiApplication::inputMethod()->reset();
			CancelComposition();
		}
		imeFallback = pos;
	}
	if (mouseUp || insideSource) {
		obs_source_send_mouse_click(source, &mouseEvent, button, mouseUp, clickCount);
	}

	return true;
}

bool OBSBasicInteraction::HandleMouseMoveEvent(QMouseEvent *event)
{
	struct obs_mouse_event mouseEvent = {};

	bool mouseLeave = event->type() == QEvent::Leave;

	if (!mouseLeave) {
		mouseEvent.modifiers = TranslateQtMouseEventModifiers(event);
		QPoint pos = event->pos();
		mouseLeave = !GetSourceRelativeXY(pos.x(), pos.y(), mouseEvent.x, mouseEvent.y);
	}

	obs_source_send_mouse_move(source, &mouseEvent, mouseLeave);

	return true;
}

bool OBSBasicInteraction::HandleMouseWheelEvent(QWheelEvent *event)
{
	struct obs_mouse_event mouseEvent = {};

	mouseEvent.modifiers = TranslateQtKeyboardEventModifiers(event, true);

	int xDelta = 0;
	int yDelta = 0;

	const QPoint angleDelta = event->angleDelta();
	if (!event->pixelDelta().isNull()) {
		if (angleDelta.x()) {
			xDelta = event->pixelDelta().x();
		} else {
			yDelta = event->pixelDelta().y();
		}
	} else {
		if (angleDelta.x()) {
			xDelta = angleDelta.x();
		} else {
			yDelta = angleDelta.y();
		}
	}

	const QPointF position = event->position();
	const int x = position.x();
	const int y = position.y();

	if (GetSourceRelativeXY(x, y, mouseEvent.x, mouseEvent.y)) {
		obs_source_send_mouse_wheel(source, &mouseEvent, xDelta, yDelta);
	}

	return true;
}

bool OBSBasicInteraction::HandleFocusEvent(QFocusEvent *event)
{
	bool focus = event->type() == QEvent::FocusIn;

	if (!focus) {
		CancelComposition();
	}
	obs_source_send_focus(source, focus);

	return true;
}

bool OBSBasicInteraction::HandleKeyEvent(QKeyEvent *event)
{
	struct obs_key_event keyEvent;

	QByteArray text = event->text().toUtf8();
	keyEvent.modifiers = TranslateQtKeyboardEventModifiers(event, false);
	keyEvent.text = text.data();
	keyEvent.native_modifiers = event->nativeModifiers();
	keyEvent.native_scancode = event->nativeScanCode();
	keyEvent.native_vkey = event->nativeVirtualKey();

	bool keyUp = event->type() == QEvent::KeyRelease;

	obs_source_send_key_click(source, &keyEvent, keyUp);

	return true;
}

void OBSBasicInteraction::CancelComposition()
{
	if (!imeEnabled) {
		return;
	}
	const bool hadComposition = composing;
	composing = false;
	// Windows Qt reset may synchronously deliver a commit. Cancellation must
	// discard that reentrant event rather than insert the preedit into the page.
	resettingIme = true;
	if (hadComposition && ui->preview->hasFocus()) {
		QGuiApplication::inputMethod()->reset();
	}
	resettingIme = false;
	obs_ime_event event = {};
	event.type = OBS_IME_CANCEL;
	event.generation = obs_source_get_ime_generation(source);
	obs_source_send_ime_event(source, &event);
}

bool OBSBasicInteraction::HandleInputMethodEvent(QInputMethodEvent *event)
{
	if (!imeEnabled) {
		return false;
	}
	if (resettingIme) {
		event->accept();
		return true;
	}
	const auto generation = obs_source_get_ime_generation(source);
	if (composing && generation != imeGeneration) {
		CancelComposition();
		event->accept();
		return true;
	}
	// CEF does not implement replacement_range for Windows OSR. Do not silently
	// apply a reconversion/replacement request at the wrong insertion point.
	if (event->replacementStart() || event->replacementLength()) {
		event->ignore();
		return false;
	}

	for (const auto &attribute : event->attributes()) {
		if (attribute.type == QInputMethodEvent::Selection) {
			event->ignore();
			return false;
		}
	}
	imeGeneration = generation;
	const QByteArray commit = event->commitString().toUtf8();
	const QByteArray preedit = event->preeditString().toUtf8();
	if (!commit.isEmpty()) {
		obs_ime_event input = {};
		input.type = OBS_IME_COMMIT;
		input.generation = generation;
		input.text = commit.constData();
		obs_source_send_ime_event(source, &input);
	}

	// A single Qt event can commit the old composition and start a new one.
	if (!preedit.isEmpty()) {
		const auto length = static_cast<uint32_t>(event->preeditString().size());
		uint32_t cursor = length;
		vector<obs_ime_underline> underlines;
		for (const auto &attribute : event->attributes()) {
			const auto start = static_cast<uint32_t>(std::clamp(attribute.start, 0, int(length)));
			if (attribute.type == QInputMethodEvent::Cursor) {
				cursor = start;
			} else if (attribute.type == QInputMethodEvent::TextFormat) {
				const auto format = qvariant_cast<QTextFormat>(attribute.value).toCharFormat();
				const auto end = static_cast<uint32_t>(std::clamp(
					int64_t(attribute.start) + attribute.length, int64_t(start), int64_t(length)));
				const uint32_t background = format.background().style() == Qt::NoBrush
								    ? 0u
								    : format.background().color().rgba();
				underlines.push_back({start, end, format.foreground().color().rgba(), background,
						      format.fontWeight() > QFont::Normal});
			}
		}
		obs_ime_event input = {};
		input.type = OBS_IME_COMPOSITION;
		input.generation = generation;
		input.text = preedit.constData();
		input.underlines = underlines.data();
		input.underline_count = underlines.size();
		input.selection_start = input.selection_end = cursor;
		obs_source_send_ime_event(source, &input);
		composing = true;
	} else {
		if (commit.isEmpty()) {
			obs_ime_event input = {};
			input.type = OBS_IME_CANCEL;
			input.generation = generation;
			obs_source_send_ime_event(source, &input);
		}
		composing = false;
	}
	event->accept();
	return true;
}

QRectF OBSBasicInteraction::GetImeCursorRect()
{
	obs_ime_rect rect = {};
	if (!obs_source_get_ime_rect(source, &rect)) {
		return QRectF(imeFallback, QSizeF(1, 20));
	}
	const QSize size = GetPixelSize(ui->preview);
	int x, y;
	float scale;
	GetScaleAndCenterPos(max(obs_source_get_width(source), 1u), max(obs_source_get_height(source), 1u),
			     size.width(), size.height(), x, y, scale);
	const qreal ratio = ui->preview->devicePixelRatioF();
	return QRectF((x + scale * rect.x) / ratio, (y + scale * rect.y) / ratio,
		      max(qreal(1), scale * rect.width / ratio), max(qreal(1), scale * rect.height / ratio));
}

bool OBSBasicInteraction::HandleInputMethodQuery(QInputMethodQueryEvent *event)
{
	if (!imeEnabled) {
		return false;
	}
	// queries() is a bit set: Qt may ask for several values in one event.
	if (event->queries().testFlag(Qt::ImEnabled)) {
		event->setValue(Qt::ImEnabled, true);
	}
	if (event->queries().testFlag(Qt::ImCursorRectangle)) {
		event->setValue(Qt::ImCursorRectangle, GetImeCursorRect());
	}
	if (event->queries().testFlag(Qt::ImHints)) {
		event->setValue(Qt::ImHints, int(Qt::ImhNone));
	}
	event->accept();
	return true;
}

void OBSBasicInteraction::Init()
{
	show();
}
