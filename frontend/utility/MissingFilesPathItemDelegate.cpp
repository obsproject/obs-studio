/******************************************************************************
    Copyright (C) 2019 by Dillon Pentz <dillon@vodbox.io>

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

#include "MissingFilesPathItemDelegate.hpp"

#include <OBSApp.hpp>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>

#include "moc_MissingFilesPathItemDelegate.cpp"

enum MissingFilesRole { EntryStateRole = Qt::UserRole, NewPathsToProcessRole };

MissingFilesPathItemDelegate::MissingFilesPathItemDelegate(bool isOutput, const QString &defaultPath)
	: QStyledItemDelegate(),
	  isOutput(isOutput),
	  defaultPath(defaultPath)
{
}

QWidget *MissingFilesPathItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */,
						    const QModelIndex &) const
{
	QSizePolicy buttonSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding,
				     QSizePolicy::ControlType::PushButton);

	QWidget *container = new QWidget(parent);

	auto browseCallback = [this, container]() {
		const_cast<MissingFilesPathItemDelegate *>(this)->handleBrowse(container);
	};

	auto clearCallback = [this, container]() {
		const_cast<MissingFilesPathItemDelegate *>(this)->handleClear(container);
	};

	QHBoxLayout *layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	QLineEdit *text = new QLineEdit();
	text->setObjectName(QStringLiteral("text"));
	text->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding,
					QSizePolicy::ControlType::LineEdit));
	layout->addWidget(text);

	QToolButton *browseButton = new QToolButton();
	browseButton->setText("...");
	browseButton->setSizePolicy(buttonSizePolicy);
	layout->addWidget(browseButton);

	container->connect(browseButton, &QToolButton::clicked, browseCallback);

	// The "clear" button is not shown in input cells
	if (isOutput) {
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

void MissingFilesPathItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QLineEdit *text = editor->findChild<QLineEdit *>();
	text->setText(index.data().toString());

	editor->setProperty(PATH_LIST_PROP, QVariant());
}

void MissingFilesPathItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
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
			model->setData(index, list);
		} else
			model->setData(index, list, MissingFilesRole::NewPathsToProcessRole);
	} else {
		QLineEdit *lineEdit = editor->findChild<QLineEdit *>();
		model->setData(index, lineEdit->text(), 0);
	}
}

void MissingFilesPathItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
					 const QModelIndex &index) const
{
	QStyleOptionViewItem localOption = option;
	initStyleOption(&localOption, index);

	QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &localOption, painter);
}

void MissingFilesPathItemDelegate::handleBrowse(QWidget *container)
{

	QLineEdit *text = container->findChild<QLineEdit *>();

	QString currentPath = text->text();
	if (currentPath.isEmpty() || currentPath.compare(QTStr("MissingFiles.Clear")) == 0)
		currentPath = defaultPath;

	bool isSet = false;
	if (isOutput) {
		QString newPath =
			QFileDialog::getOpenFileName(container, QTStr("MissingFiles.SelectFile"), currentPath, nullptr);

#ifdef __APPLE__
		// TODO: Revisit when QTBUG-42661 is fixed
		container->window()->raise();
#endif

		if (!newPath.isEmpty()) {
			container->setProperty(PATH_LIST_PROP, QStringList() << newPath);
			isSet = true;
		}
	}

	if (isSet)
		emit commitData(container);
}

void MissingFilesPathItemDelegate::handleClear(QWidget *container)
{
	// An empty string list will indicate that the entry is being
	// blanked and should be deleted.
	container->setProperty(PATH_LIST_PROP, QStringList() << QTStr("MissingFiles.Clear"));
	container->findChild<QLineEdit *>()->clearFocus();
	((QWidget *)container->parent())->setFocus();
	emit commitData(container);
}
