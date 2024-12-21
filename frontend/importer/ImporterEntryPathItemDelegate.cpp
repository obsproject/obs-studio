/******************************************************************************
    Copyright (C) 2019-2020 by Dillon Pentz <dillon@vodbox.io>

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

#include "ImporterEntryPathItemDelegate.hpp"
#include "ImporterModel.hpp"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>

#include "moc_ImporterEntryPathItemDelegate.cpp"

ImporterEntryPathItemDelegate::ImporterEntryPathItemDelegate() : QStyledItemDelegate() {}

QWidget *ImporterEntryPathItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */,
						     const QModelIndex &index) const
{
	bool empty = index.model()
			     ->index(index.row(), ImporterColumn::Path)
			     .data(ImporterEntryRole::CheckEmpty)
			     .value<bool>();

	QSizePolicy buttonSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding,
				     QSizePolicy::ControlType::PushButton);

	QWidget *container = new QWidget(parent);

	auto browseCallback = [this, container]() {
		const_cast<ImporterEntryPathItemDelegate *>(this)->handleBrowse(container);
	};

	auto clearCallback = [this, container]() {
		const_cast<ImporterEntryPathItemDelegate *>(this)->handleClear(container);
	};

	QHBoxLayout *layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	QLineEdit *text = new QLineEdit();
	text->setObjectName(QStringLiteral("text"));
	text->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding,
					QSizePolicy::ControlType::LineEdit));
	layout->addWidget(text);

	QObject::connect(text, &QLineEdit::editingFinished, this, &ImporterEntryPathItemDelegate::updateText);

	QToolButton *browseButton = new QToolButton();
	browseButton->setText("...");
	browseButton->setSizePolicy(buttonSizePolicy);
	layout->addWidget(browseButton);

	container->connect(browseButton, &QToolButton::clicked, browseCallback);

	// The "clear" button is not shown in output cells
	// or the insertion point's input cell.
	if (!empty) {
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

void ImporterEntryPathItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QLineEdit *text = editor->findChild<QLineEdit *>();
	text->setText(index.data().toString());
	editor->setProperty(PATH_LIST_PROP, QVariant());
}

void ImporterEntryPathItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
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
		model->setData(index, list, ImporterEntryRole::NewPath);
	} else {
		QLineEdit *lineEdit = editor->findChild<QLineEdit *>();
		model->setData(index, lineEdit->text());
	}
}

void ImporterEntryPathItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
					  const QModelIndex &index) const
{
	QStyleOptionViewItem localOption = option;
	initStyleOption(&localOption, index);

	QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &localOption, painter);
}

void ImporterEntryPathItemDelegate::handleBrowse(QWidget *container)
{
	QString Pattern = "(*.json *.bpres *.xml *.xconfig)";

	QLineEdit *text = container->findChild<QLineEdit *>();

	QString currentPath = text->text();

	bool isSet = false;
	QStringList paths = OpenFiles(container, QTStr("Importer.SelectCollection"), currentPath,
				      QTStr("Importer.Collection") + QString(" ") + Pattern);

	if (!paths.empty()) {
		container->setProperty(PATH_LIST_PROP, paths);
		isSet = true;
	}

	if (isSet)
		emit commitData(container);
}

void ImporterEntryPathItemDelegate::handleClear(QWidget *container)
{
	// An empty string list will indicate that the entry is being
	// blanked and should be deleted.
	container->setProperty(PATH_LIST_PROP, QStringList());

	emit commitData(container);
}

void ImporterEntryPathItemDelegate::updateText()
{
	QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(sender());
	QWidget *editor = lineEdit->parentWidget();
	emit commitData(editor);
}
