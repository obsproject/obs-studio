#include "OBSInlineNameEditor.hpp"

#include <widgets/OBSBasic.hpp>

#include <Idian/Utils.hpp>

#include <QKeyEvent>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

OBSInlineNameEditor::OBSInlineNameEditor(OBSSource source, QWidget *parent) : QLineEdit(parent)
{
	setFocusPolicy(Qt::StrongFocus);
	setReadOnly(true);

	setCursor(Qt::PointingHandCursor);

	setSource(source);

	clearPendingName();

	idian::Utils::addClass(this, "text-title");

	errorAnimation = new QPropertyAnimation(this, "pos", this);
	errorAnimation->setDuration(600);
	errorAnimation->setEasingCurve(QEasingCurve::Linear);

	connect(errorAnimation, &QPropertyAnimation::finished, this,
		[this]() { idian::Utils::removeClass(this, "text-danger"); });

	connect(this, &QLineEdit::textEdited, this, &OBSInlineNameEditor::setPendingName);
	connect(this, &QLineEdit::editingFinished, this, [this]() { finishEditing(true); });
}

void OBSInlineNameEditor::obsSourceRenamed(void *data, calldata_t *params)
{
	auto editor = static_cast<OBSInlineNameEditor *>(data);

	const char *name = calldata_string(params, "new_name");
	QMetaObject::invokeMethod(editor, &OBSInlineNameEditor::setOriginalName, Qt::QueuedConnection,
				  QString::fromUtf8(name));
}

void OBSInlineNameEditor::obsSourceRemoved(void *data, calldata_t *)
{
	auto editor = static_cast<OBSInlineNameEditor *>(data);
	QMetaObject::invokeMethod(editor, &OBSInlineNameEditor::clearSource, Qt::QueuedConnection);
}

void OBSInlineNameEditor::obsSourceDestroyed(void *data, calldata_t *)
{
	auto editor = static_cast<OBSInlineNameEditor *>(data);
	QMetaObject::invokeMethod(editor, &OBSInlineNameEditor::clearSource, Qt::QueuedConnection);
}

void OBSInlineNameEditor::playErrorAnimation()
{
	if (errorAnimation->state() == QAbstractAnimation::Running) {
		return;
	}

	idian::Utils::addClass(this, "text-danger");

	const QPoint origin = pos();
	constexpr int shakeAmount = 4;

	errorAnimation->setStartValue(origin);
	errorAnimation->setKeyValueAt(0.10, origin + QPoint(-shakeAmount, 0));
	errorAnimation->setKeyValueAt(0.20, origin + QPoint(shakeAmount, 0));
	errorAnimation->setKeyValueAt(0.30, origin + QPoint(-shakeAmount, 0));
	errorAnimation->setKeyValueAt(0.40, origin + QPoint(shakeAmount, 0));
	errorAnimation->setKeyValueAt(0.50, origin + QPoint(-shakeAmount / 2, 0));
	errorAnimation->setKeyValueAt(0.60, origin);
	errorAnimation->setEndValue(origin);

	errorAnimation->start();
}

void OBSInlineNameEditor::setSource(OBSSource source_)
{
	clearPendingName();

	obsSignals.clear();

	source = source_;
	if (!source) {
		setText("");
		return;
	}

	signal_handler_t *handler = obs_source_get_signal_handler(source);

	obsSignals.emplace_back(handler, "rename", &OBSInlineNameEditor::obsSourceRenamed, this);
	obsSignals.emplace_back(handler, "remove", &OBSInlineNameEditor::obsSourceRemoved, this);
	obsSignals.emplace_back(handler, "destroy", &OBSInlineNameEditor::obsSourceDestroyed, this);

	const char *name = obs_source_get_name(source);
	setOriginalName(QString::fromUtf8(name));
	setText(name);
}

void OBSInlineNameEditor::clearSource()
{
	setSource(nullptr);
}

void OBSInlineNameEditor::clearPendingName()
{
	pendingName.clear();
}

void OBSInlineNameEditor::setOriginalName(QString name)
{
	originalName = name;
	setPlaceholderText(originalName);
	if (isReadOnly()) {
		setText(originalName);
	}
}

void OBSInlineNameEditor::setPendingName(QString name)
{
	pendingName = name;
}

void OBSInlineNameEditor::beginEditing()
{
	if (!isReadOnly()) {
		return;
	}

	if (errorAnimation->state() == QAbstractAnimation::Running) {
		errorAnimation->setCurrentTime(errorAnimation->duration());
		errorAnimation->stop();
	}

	setReadOnly(false);

	if (!pendingName.isEmpty()) {
		setText(pendingName);
	}

	setFocus();

	setOriginalName(QString::fromUtf8(obs_source_get_name(source)));
}

void OBSInlineNameEditor::finishEditing(bool save)
{
	if (isReadOnly()) {
		return;
	}

	deselect();
	setReadOnly(true);

	if (!source) {
		return;
	}

	const QString enteredText = text();

	if (!save || enteredText.isEmpty()) {
		setText(originalName);
		return;
	}

	if (enteredText == originalName) {
		return;
	}

	OBSSourceAutoRelease existing = obs_get_source_by_name(enteredText.toUtf8().constData());
	if (existing) {
		setText(originalName);
		playErrorAnimation();
		return;
	}

	const std::string uuid = obs_source_get_uuid(source);

	const std::string prevName{obs_source_get_name(source)};
	auto undo = [prevName](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
		if (source) {
			obs_source_set_name(source, prevName.c_str());
		}
	};

	obs_source_set_name(source, enteredText.toUtf8().constData());

	const std::string newName{obs_source_get_name(source)};
	auto redo = [newName](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
		if (source) {
			obs_source_set_name(source, newName.c_str());
		}
	};

	OBSBasic *main = OBSBasic::Get();

	main->undo_s.add_action(QTStr("Undo.Rename").arg(newName.c_str()), undo, redo, uuid, uuid);

	clearPendingName();
}

void OBSInlineNameEditor::focusInEvent(QFocusEvent *event)
{
	if (event->reason() == Qt::PopupFocusReason) {
		QLineEdit::focusInEvent(event);
		return;
	}

	beginEditing();
	QLineEdit::focusInEvent(event);

	if (event->reason() == Qt::MouseFocusReason) {
		wasFocusedFromMouseClick = true;
	} else {
		selectAll();
	}
}

void OBSInlineNameEditor::focusOutEvent(QFocusEvent *event)
{
	wasFocusedFromMouseClick = false;

	if (event->reason() == Qt::PopupFocusReason) {
		QLineEdit::focusOutEvent(event);
		return;
	}

	finishEditing(true);
	QLineEdit::focusOutEvent(event);
}

void OBSInlineNameEditor::enterEvent(QEnterEvent *event)
{
	if (isReadOnly()) {
		setCursor(Qt::PointingHandCursor);
	}
	QLineEdit::enterEvent(event);
}

void OBSInlineNameEditor::mousePressEvent(QMouseEvent *event)
{
	if (wasFocusedFromMouseClick) {
		wasFocusedFromMouseClick = false;
		event->accept();
		selectAll();
		return;
	}

	QLineEdit::mousePressEvent(event);
}

void OBSInlineNameEditor::keyPressEvent(QKeyEvent *event)
{
	if (!isReadOnly()) {
		switch (event->key()) {
		case Qt::Key_Enter:
		case Qt::Key_Return:
			event->accept();

			clearFocus();
			if (previousInFocusChain()) {
				previousInFocusChain()->setFocus();
			}
			return;
		case Qt::Key_Escape:
			event->accept();

			clearFocus();
			if (previousInFocusChain()) {
				previousInFocusChain()->setFocus();
			}
			return;
		}
	}

	QLineEdit::keyPressEvent(event);
}
