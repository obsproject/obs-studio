#include "ExtraBrowsersDelegate.hpp"
#include "ExtraBrowsersModel.hpp"

#include <components/EditWidget.hpp>

#include <qt-wrappers.hpp>

#include <QKeyEvent>

#include "moc_ExtraBrowsersDelegate.cpp"

QWidget *ExtraBrowsersDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
					     const QModelIndex &index) const
{
	QLineEdit *text = new EditWidget(parent, index);
	text->installEventFilter(const_cast<ExtraBrowsersDelegate *>(this));
	text->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding,
					QSizePolicy::ControlType::LineEdit));
	return text;
}

void ExtraBrowsersDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QLineEdit *text = reinterpret_cast<QLineEdit *>(editor);
	text->blockSignals(true);
	text->setText(index.data().toString());
	text->blockSignals(false);
}

bool ExtraBrowsersDelegate::eventFilter(QObject *object, QEvent *event)
{
	QLineEdit *edit = qobject_cast<QLineEdit *>(object);
	if (!edit)
		return false;

	if (LineEditCanceled(event)) {
		RevertText(edit);
	}
	if (LineEditChanged(event)) {
		UpdateText(edit);

		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->key() == Qt::Key_Tab) {
				model->TabSelection(true);
			} else if (keyEvent->key() == Qt::Key_Backtab) {
				model->TabSelection(false);
			}
		}
		return true;
	}

	return false;
}

bool ExtraBrowsersDelegate::ValidName(const QString &name) const
{
	for (auto &item : model->items) {
		if (name.compare(item.title, Qt::CaseInsensitive) == 0) {
			return false;
		}
	}
	return true;
}

void ExtraBrowsersDelegate::RevertText(QLineEdit *edit_)
{
	EditWidget *edit = reinterpret_cast<EditWidget *>(edit_);
	int row = edit->index.row();
	int col = edit->index.column();
	bool newItem = (row == model->items.size());

	QString oldText;
	if (col == (int)Column::Title) {
		oldText = newItem ? model->newTitle : model->items[row].title;
	} else {
		oldText = newItem ? model->newURL : model->items[row].url;
	}

	edit->setText(oldText);
}

bool ExtraBrowsersDelegate::UpdateText(QLineEdit *edit_)
{
	EditWidget *edit = reinterpret_cast<EditWidget *>(edit_);
	int row = edit->index.row();
	int col = edit->index.column();
	bool newItem = (row == model->items.size());

	QString text = edit->text().trimmed();

	if (!newItem && text.isEmpty()) {
		return false;
	}

	if (col == (int)Column::Title) {
		QString oldText = newItem ? model->newTitle : model->items[row].title;
		bool same = oldText.compare(text, Qt::CaseInsensitive) == 0;

		if (!same && !ValidName(text)) {
			edit->setText(oldText);
			return false;
		}
	}

	if (!newItem) {
		/* if edited existing item, update it*/
		switch (col) {
		case (int)Column::Title:
			model->items[row].title = text;
			break;
		case (int)Column::Url:
			model->items[row].url = text;
			break;
		}
	} else {
		/* if both new values filled out, create new one */
		switch (col) {
		case (int)Column::Title:
			model->newTitle = text;
			break;
		case (int)Column::Url:
			model->newURL = text;
			break;
		}

		model->CheckToAdd();
	}

	emit commitData(edit);
	return true;
}
