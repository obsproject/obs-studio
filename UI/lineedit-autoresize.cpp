#include "lineedit-autoresize.hpp"

LineEditAutoResize::LineEditAutoResize() : m_maxLength(32767)
{
	connect(this, &LineEditAutoResize::textChanged, this,
		&LineEditAutoResize::checkTextLength);
	connect(document()->documentLayout(),
		&QAbstractTextDocumentLayout::documentSizeChanged, this,
		&LineEditAutoResize::resizeVertically);
}

void LineEditAutoResize::checkTextLength()
{
	QString text = toPlainText();
	if (text.length() > m_maxLength) {
		setPlainText(text.left(m_maxLength));
		QTextCursor cursor = textCursor();
		cursor.setPosition(m_maxLength);
		setTextCursor(cursor);
	}
}

int LineEditAutoResize::maxLength()
{
	return m_maxLength;
}

void LineEditAutoResize::setMaxLength(int length)
{
	m_maxLength = length;
}

void LineEditAutoResize::keyPressEvent(QKeyEvent *event)
{
	/* Always allow events with modifiers, for example to Copy content */
	Qt::KeyboardModifiers modifiers = event->modifiers();
	if (modifiers != Qt::NoModifier && modifiers != Qt::ShiftModifier) {
		QTextEdit::keyPressEvent(event);
		return;
	}

	switch (event->key()) {
	/* Always ignore the return key and send the signal instead */
	case Qt::Key_Return:
		event->ignore();
		emit returnPressed();
		break;
	/* Always allow navigation and deletion */
	case Qt::Key_Left:
	case Qt::Key_Right:
	case Qt::Key_Up:
	case Qt::Key_Down:
	case Qt::Key_PageUp:
	case Qt::Key_PageDown:
	case Qt::Key_Delete:
	case Qt::Key_Backspace:
		QTextEdit::keyPressEvent(event);
		break;
	/* Allow only if the content is not already at max length.
	 * Some keys with modifiers should still be allowed though
	 * (for example for Copy), and we don't want to make assumptions
	 * about which modifiers are okay and which aren't, so let's
	 * allow all. Actions that will still exceed the limit (like
	 * Paste) can be caught in a later step. */
	default:
		if (toPlainText().length() >= m_maxLength &&
		    event->modifiers() == Qt::NoModifier &&
		    !textCursor().hasSelection())
			event->ignore();
		else
			QTextEdit::keyPressEvent(event);
		break;
	}
}

void LineEditAutoResize::resizeVertically(const QSizeF &newSize)
{
	QMargins margins = contentsMargins();
	setMaximumHeight(newSize.height() + margins.top() + margins.bottom());
}

QString LineEditAutoResize::text()
{
	return toPlainText();
}

void LineEditAutoResize::setText(const QString &text)
{
	QMetaObject::invokeMethod(this, "SetPlainText", Qt::QueuedConnection,
				  Q_ARG(const QString &, text));
}

void LineEditAutoResize::SetPlainText(const QString &text)
{
	setPlainText(text);
}
