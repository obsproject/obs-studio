/******************************************************************************
    Copyright (C) 2014 by Ruwen Hahn <palana@stunned.de>

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

#include "RemuxEntryPathItemDelegate.hpp"
#include "RemuxQueueModel.hpp"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>

#include "moc_RemuxEntryPathItemDelegate.cpp"

RemuxEntryPathItemDelegate::RemuxEntryPathItemDelegate(bool isOutput, const QString &defaultPath)
	: QStyledItemDelegate(),
	  isOutput(isOutput),
	  defaultPath(defaultPath)
{
}

QWidget *RemuxEntryPathItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */,
						  const QModelIndex &index) const
{
	RemuxEntryState state = index.model()
					->index(index.row(), RemuxEntryColumn::State)
					.data(RemuxEntryRole::EntryStateRole)
					.value<RemuxEntryState>();
	if (state == RemuxEntryState::Pending || state == RemuxEntryState::InProgress) {
		// Never allow modification of rows that are
		// in progress.
		return Q_NULLPTR;
	} else if (isOutput && state != RemuxEntryState::Ready) {
		// Do not allow modification of output rows
		// that aren't associated with a valid input.
		return Q_NULLPTR;
	} else if (!isOutput && state == RemuxEntryState::Complete) {
		// Don't allow modification of rows that are
		// already complete.
		return Q_NULLPTR;
	} else {
		QSizePolicy buttonSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding,
					     QSizePolicy::ControlType::PushButton);

		QWidget *container = new QWidget(parent);

		auto browseCallback = [this, container]() {
			const_cast<RemuxEntryPathItemDelegate *>(this)->handleBrowse(container);
		};

		auto clearCallback = [this, container]() {
			const_cast<RemuxEntryPathItemDelegate *>(this)->handleClear(container);
		};

		QHBoxLayout *layout = new QHBoxLayout();
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		QLineEdit *text = new QLineEdit();
		text->setObjectName(QStringLiteral("text"));
		text->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding,
						QSizePolicy::ControlType::LineEdit));
		layout->addWidget(text);

		QObject::connect(text, &QLineEdit::editingFinished, this, &RemuxEntryPathItemDelegate::updateText);

		QToolButton *browseButton = new QToolButton();
		browseButton->setText("...");
		browseButton->setSizePolicy(buttonSizePolicy);
		layout->addWidget(browseButton);

		container->connect(browseButton, &QToolButton::clicked, browseCallback);

		// The "clear" button is not shown in output cells
		// or the insertion point's input cell.
		if (!isOutput && state != RemuxEntryState::Empty) {
			QToolButton *clearButton = new QToolButton();
			clearButton->setText("X");
			clearButton->setSizePolicy(buttonSizePolicy);
			layout->addWidget(clearButton);

			container->connect(clearButton, &QToolButton::clicked, clearCallback);
		}

		container->setLayout(layout);
		container->setFocusProxy(text);
		return container;
	}
}

void RemuxEntryPathItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QLineEdit *text = editor->findChild<QLineEdit *>();
	text->setText(index.data().toString());
	editor->setProperty(PATH_LIST_PROP, QVariant());
}

void RemuxEntryPathItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
					      const QModelIndex &index) const
{
	// We use the PATH_LIST_PROP property to pass a list of
	// path strings from the editor widget into the model's
	// NewPathsToProcessRole. This is only used when paths
	// are selected through the "browse" or "delete" buttons
	// in the editor. If the user enters new text in the
	// text box, we simply pass that text on to the model
	// as normal text data in the default role.
	QVariant pathListProp = editor->property(PATH_LIST_PROP);
	if (pathListProp.isValid()) {
		QStringList list = editor->property(PATH_LIST_PROP).toStringList();
		if (isOutput) {
			if (list.size() > 0)
				model->setData(index, list);
		} else
			model->setData(index, list, RemuxEntryRole::NewPathsToProcessRole);
	} else {
		QLineEdit *lineEdit = editor->findChild<QLineEdit *>();
		model->setData(index, lineEdit->text());
	}
}

void RemuxEntryPathItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
				       const QModelIndex &index) const
{
	RemuxEntryState state = index.model()
					->index(index.row(), RemuxEntryColumn::State)
					.data(RemuxEntryRole::EntryStateRole)
					.value<RemuxEntryState>();

	QStyleOptionViewItem localOption = option;
	initStyleOption(&localOption, index);

	if (isOutput) {
		if (state != Ready) {
			QColor background =
				localOption.palette.color(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Window);

			localOption.backgroundBrush = QBrush(background);
		}
	}

	QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &localOption, painter);
}

void RemuxEntryPathItemDelegate::handleBrowse(QWidget *container)
{
	QString ExtensionPattern = "(*.mp4 *.flv *.mov *.mkv *.ts *.m3u8)";

	QLineEdit *text = container->findChild<QLineEdit *>();

	QString currentPath = text->text();
	if (currentPath.isEmpty())
		currentPath = defaultPath;

	bool isSet = false;
	if (isOutput) {
		QString newPath = SaveFile(container, QTStr("Remux.SelectTarget"), currentPath, ExtensionPattern);

		if (!newPath.isEmpty()) {
			container->setProperty(PATH_LIST_PROP, QStringList() << newPath);
			isSet = true;
		}
	} else {
		QStringList paths = OpenFiles(container, QTStr("Remux.SelectRecording"), currentPath,
					      QTStr("Remux.OBSRecording") + QString(" ") + ExtensionPattern);

		if (!paths.empty()) {
			container->setProperty(PATH_LIST_PROP, paths);
			isSet = true;
		}
#ifdef __APPLE__
		// TODO: Revisit when QTBUG-42661 is fixed
		container->window()->raise();
#endif
	}

	if (isSet)
		emit commitData(container);
}

void RemuxEntryPathItemDelegate::handleClear(QWidget *container)
{
	// An empty string list will indicate that the entry is being
	// blanked and should be deleted.
	container->setProperty(PATH_LIST_PROP, QStringList());

	emit commitData(container);
}

void RemuxEntryPathItemDelegate::updateText()
{
	QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(sender());
	QWidget *editor = lineEdit->parentWidget();
	emit commitData(editor);
}
