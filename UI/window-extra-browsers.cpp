#include "window-extra-browsers.hpp"
#include "window-dock-browser.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#include <QLineEdit>
#include <QHBoxLayout>
#include <QUuid>

#include <json11.hpp>

#include "ui_OBSExtraBrowsers.h"

using namespace json11;

#define OBJ_NAME_SUFFIX "_extraBrowser"

enum class Column : int {
	Title,
	Url,
	CustomCss,
	Delete,

	Count,
};

/* ------------------------------------------------------------------------- */

void ExtraBrowsersModel::Reset()
{
	items.clear();

	OBSBasic *main = OBSBasic::Get();

	for (int i = 0; i < main->extraBrowserDocks.size(); i++) {
		BrowserDock *dock = reinterpret_cast<BrowserDock *>(
			main->extraBrowserDocks[i].data());

		Item item;
		item.prevIdx = i;
		item.title = dock->windowTitle();
		item.url = main->extraBrowserDockTargets[i];
		item.customCss = main->extraBrowserDockCustomCss[i];
		items.push_back(item);
	}
}

int ExtraBrowsersModel::rowCount(const QModelIndex &) const
{
	int count = items.size() + 1;
	return count;
}

int ExtraBrowsersModel::columnCount(const QModelIndex &) const
{
	return (int)Column::Count;
}

QVariant ExtraBrowsersModel::data(const QModelIndex &index, int role) const
{
	int column = index.column();
	int idx = index.row();
	int count = items.size();
	bool validRole = role == Qt::DisplayRole ||
			 role == Qt::AccessibleTextRole;

	if (!validRole)
		return QVariant();

	if (idx >= 0 && idx < count) {
		switch (column) {
		case (int)Column::Title:
			return items[idx].title;
		case (int)Column::Url:
			return items[idx].url;
		case (int)Column::CustomCss:
			return items[idx].customCss;
		}
	} else if (idx == count) {
		switch (column) {
		case (int)Column::Title:
			return newTitle;
		case (int)Column::Url:
			return newURL;
		case (int)Column::CustomCss:
			return newCustomCss;
		}
	}

	return QVariant();
}

QVariant ExtraBrowsersModel::headerData(int section,
					Qt::Orientation orientation,
					int role) const
{
	bool validRole = role == Qt::DisplayRole ||
			 role == Qt::AccessibleTextRole;

	if (validRole && orientation == Qt::Orientation::Horizontal) {
		switch (section) {
		case (int)Column::Title:
			return QTStr("ExtraBrowsers.DockName");
		case (int)Column::Url:
			return QStringLiteral("URL");
		case (int)Column::CustomCss:
			return QStringLiteral("CSS");
		}
	}

	return QVariant();
}

Qt::ItemFlags ExtraBrowsersModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);

	if (index.column() != (int)Column::Delete)
		flags |= Qt::ItemIsEditable;

	return flags;
}

class DelButton : public QPushButton {
public:
	inline DelButton(QModelIndex index_) : QPushButton(), index(index_) {}

	QPersistentModelIndex index;
};

class EditWidget : public QLineEdit {
public:
	inline EditWidget(QWidget *parent, QModelIndex index_)
		: QLineEdit(parent), index(index_)
	{
	}

	QPersistentModelIndex index;
};

void ExtraBrowsersModel::AddDeleteButton(int idx)
{
	QTableView *widget = reinterpret_cast<QTableView *>(parent());

	QModelIndex index = createIndex(idx, (int)Column::Delete, nullptr);

	QPushButton *del = new DelButton(index);
	del->setProperty("themeID", "removeIconSmall");
	del->setObjectName("extraPanelDelete");
	del->setMinimumSize(QSize(20, 20));
	connect(del, &QPushButton::clicked, this,
		&ExtraBrowsersModel::DeleteItem);

	widget->setIndexWidget(index, del);
	widget->setRowHeight(idx, 20);
	widget->setColumnWidth(idx, 20);
}

void ExtraBrowsersModel::CheckToAdd()
{
	if (newTitle.isEmpty() || newURL.isEmpty())
		return;

	int idx = items.size() + 1;
	beginInsertRows(QModelIndex(), idx, idx);

	Item item;
	item.prevIdx = -1;
	item.title = newTitle;
	item.url = newURL;
	item.customCss = newCustomCss;
	items.push_back(item);

	newTitle = "";
	newURL = "";
	newCustomCss = "";

	endInsertRows();

	AddDeleteButton(idx - 1);
}

void ExtraBrowsersModel::UpdateItem(Item &item)
{
	int idx = item.prevIdx;

	OBSBasic *main = OBSBasic::Get();
	BrowserDock *dock = reinterpret_cast<BrowserDock *>(
		main->extraBrowserDocks[idx].data());
	dock->setWindowTitle(item.title);
	dock->setObjectName(item.title + OBJ_NAME_SUFFIX);
	main->extraBrowserDockActions[idx]->setText(item.title);

	if (main->extraBrowserDockTargets[idx] != item.url) {
		dock->cefWidget->setURL(QT_TO_UTF8(item.url));
		main->extraBrowserDockTargets[idx] = item.url;
	}
	if (main->extraBrowserDockCustomCss[idx] != item.customCss) {
		if (true) {
			// Attempt to force a change and reload of new script
			QCefWidget* browser = dock->cefWidget.data();
			ExtraBrowsersModel::SetCustomBrowserScriptFromUrlAndCustomCss(browser, item.url, item.customCss);
			// doesnt have any effect on reloading new script so don't annoy user with refresh
			//browser->reloadPage();
		}
		main->extraBrowserDockCustomCss[idx] = item.customCss;
	}
}

void ExtraBrowsersModel::DeleteItem()
{
	QTableView *widget = reinterpret_cast<QTableView *>(parent());

	DelButton *del = reinterpret_cast<DelButton *>(sender());
	int row = del->index.row();

	/* there's some sort of internal bug in Qt and deleting certain index
	 * widgets or "editors" that can cause a crash inside Qt if the widget
	 * is not manually removed, at least on 5.7 */
	widget->setIndexWidget(del->index, nullptr);
	del->deleteLater();

	/* --------- */

	beginRemoveRows(QModelIndex(), row, row);

	int prevIdx = items[row].prevIdx;
	items.removeAt(row);

	if (prevIdx != -1) {
		int i = 0;
		for (; i < deleted.size() && deleted[i] < prevIdx; i++)
			;
		deleted.insert(i, prevIdx);
	}

	endRemoveRows();
}

void ExtraBrowsersModel::Apply()
{
	OBSBasic *main = OBSBasic::Get();

	for (Item &item : items) {
		if (item.prevIdx != -1) {
			UpdateItem(item);
		} else {
			QString uuid = QUuid::createUuid().toString();
			uuid.replace(QRegularExpression("[{}-]"), "");
			main->AddExtraBrowserDock(item.title, item.url, uuid,
				item.customCss, true);
		}
	}

	for (int i = deleted.size() - 1; i >= 0; i--) {
		int idx = deleted[i];
		main->extraBrowserDockActions.removeAt(idx);
		main->extraBrowserDockTargets.removeAt(idx);
		main->extraBrowserDockCustomCss.removeAt(idx);
		main->extraBrowserDocks.removeAt(idx);
	}

	deleted.clear();

	Reset();
}

void ExtraBrowsersModel::TabSelection(bool forward)
{
	QListView *widget = reinterpret_cast<QListView *>(parent());
	QItemSelectionModel *selModel = widget->selectionModel();

	QModelIndex sel = selModel->currentIndex();
	int row = sel.row();
	int col = sel.column();

	switch (sel.column()) {
	case (int)Column::Title:
		if (!forward) {
			if (row == 0) {
				return;
			}

			row -= 1;
		}

		col += 1;
		break;

	case (int)Column::Url:
		if (!forward) {
			if (row == 0) {
				return;
			}

			row -= 1;
		}

		col += 1;
		break;

	case (int)Column::CustomCss:
		if (forward) {
			if (row == items.size()) {
				return;
			}

			row += 1;
		}

		col -= 1;
		break;
	}

	sel = createIndex(row, col, nullptr);
	selModel->setCurrentIndex(sel, QItemSelectionModel::Clear);
}

void ExtraBrowsersModel::Init()
{
	for (int i = 0; i < items.count(); i++)
		AddDeleteButton(i);
}

/* ------------------------------------------------------------------------- */

QWidget *ExtraBrowsersDelegate::createEditor(QWidget *parent,
					     const QStyleOptionViewItem &,
					     const QModelIndex &index) const
{
	QLineEdit *text = new EditWidget(parent, index);
	text->installEventFilter(const_cast<ExtraBrowsersDelegate *>(this));
	text->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding,
					QSizePolicy::Policy::Expanding,
					QSizePolicy::ControlType::LineEdit));
	return text;
}

void ExtraBrowsersDelegate::setEditorData(QWidget *editor,
					  const QModelIndex &index) const
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
	} else if (col == (int)Column::Url) {
		oldText = newItem ? model->newURL : model->items[row].url;
	} else if (col == (int)Column::CustomCss) {
		oldText = newItem ? model->newCustomCss : model->items[row].customCss;
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

	if (!newItem && text.isEmpty() && col != (int)Column::CustomCss) {
		return false;
	}

	if (col == (int)Column::Title) {
		QString oldText = newItem ? model->newTitle
					  : model->items[row].title;
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
		case (int)Column::CustomCss:
			model->items[row].customCss = text;
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
		case (int)Column::CustomCss:
			model->newCustomCss = text;
			break;
		}

		model->CheckToAdd();
	}

	emit commitData(edit);
	return true;
}

/* ------------------------------------------------------------------------- */

OBSExtraBrowsers::OBSExtraBrowsers(QWidget *parent)
	: QDialog(parent), ui(new Ui::OBSExtraBrowsers)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	model = new ExtraBrowsersModel(ui->table);

	ui->table->setModel(model);
	ui->table->setItemDelegateForColumn((int)Column::Title,
					    new ExtraBrowsersDelegate(model));
	ui->table->setItemDelegateForColumn((int)Column::Url,
					    new ExtraBrowsersDelegate(model));
	ui->table->setItemDelegateForColumn((int)Column::CustomCss,
					    new ExtraBrowsersDelegate(model));
	ui->table->horizontalHeader()->setSectionResizeMode(
		QHeaderView::ResizeMode::Stretch);
	ui->table->horizontalHeader()->setSectionResizeMode(
		(int)Column::Delete, QHeaderView::ResizeMode::Fixed);
	ui->table->setEditTriggers(
		QAbstractItemView::EditTrigger::CurrentChanged);
}

OBSExtraBrowsers::~OBSExtraBrowsers() {}

void OBSExtraBrowsers::closeEvent(QCloseEvent *event)
{
	QDialog::closeEvent(event);
	model->Apply();
}

void OBSExtraBrowsers::on_apply_clicked()
{
	model->Apply();
}

/* ------------------------------------------------------------------------- */

void OBSBasic::ClearExtraBrowserDocks()
{
	extraBrowserDockTargets.clear();
	extraBrowserDockCustomCss.clear();
	extraBrowserDockActions.clear();
	extraBrowserDocks.clear();
}

void OBSBasic::LoadExtraBrowserDocks()
{
	const char *jsonStr = config_get_string(
		App()->GlobalConfig(), "BasicWindow", "ExtraBrowserDocks");

	std::string err;
	Json json = Json::parse(jsonStr, err);
	if (!err.empty())
		return;

	Json::array array = json.array_items();
	if (!array.empty())
		ui->menuDocks->addSeparator();

	for (Json &item : array) {
		std::string title = item["title"].string_value();
		std::string url = item["url"].string_value();
		std::string uuid = item["uuid"].string_value();
		std::string customCss = item["customCss"].string_value();

		AddExtraBrowserDock(title.c_str(), url.c_str(), uuid.c_str(),
				    customCss.c_str(), false);
	}
}

void OBSBasic::SaveExtraBrowserDocks()
{
	Json::array array;
	for (int i = 0; i < extraBrowserDocks.size(); i++) {
		QDockWidget *dock = extraBrowserDocks[i].data();
		QString url = extraBrowserDockTargets[i];
		QString customCss = extraBrowserDockCustomCss[i];
		QString uuid = dock->property("uuid").toString();
		Json::object obj{
			{"title", QT_TO_UTF8(dock->windowTitle())},
			{"url", QT_TO_UTF8(url)},
			{"customCss", QT_TO_UTF8(customCss)},
			{"uuid", QT_TO_UTF8(uuid)},
		};
		array.push_back(obj);
	}

	std::string output = Json(array).dump();
	config_set_string(App()->GlobalConfig(), "BasicWindow",
			  "ExtraBrowserDocks", output.c_str());
}

void OBSBasic::ManageExtraBrowserDocks()
{
	if (!extraBrowsers.isNull()) {
		extraBrowsers->show();
		extraBrowsers->raise();
		return;
	}

	extraBrowsers = new OBSExtraBrowsers(this);
	extraBrowsers->show();
}

void OBSBasic::AddExtraBrowserDock(const QString &title, const QString &inUrl,
				   const QString &uuid, const QString &inCustomCss, bool firstCreate)
{
	static int panel_version = -1;
	if (panel_version == -1) {
		panel_version = obs_browser_qcef_version();
	}

	BrowserDock *dock = new BrowserDock();
	QString bId(uuid.isEmpty() ? QUuid::createUuid().toString() : uuid);
	bId.replace(QRegularExpression("[{}-]"), "");
	dock->setProperty("uuid", bId);
	dock->setObjectName(title + OBJ_NAME_SUFFIX);
	dock->resize(460, 600);
	dock->setMinimumSize(80, 80);
	dock->setWindowTitle(title);
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	QString customCss = inCustomCss;
	QString url = inUrl;


	QCefWidget *browser =
		cef->create_widget(dock, QT_TO_UTF8(url), nullptr);
	if (browser && panel_version >= 1)
		browser->allowAllPopups(true);

	dock->SetWidget(browser);

	ExtraBrowsersModel::SetCustomBrowserScriptFromUrlAndCustomCss(browser, url, customCss);

	addDockWidget(Qt::RightDockWidgetArea, dock);

	if (firstCreate) {
		dock->setFloating(true);

		QPoint curPos = pos();
		QSize wSizeD2 = size() / 2;
		QSize dSizeD2 = dock->size() / 2;

		curPos.setX(curPos.x() + wSizeD2.width() - dSizeD2.width());
		curPos.setY(curPos.y() + wSizeD2.height() - dSizeD2.height());

		dock->move(curPos);
		dock->setVisible(true);
	}

	QAction *action = AddDockWidget(dock);
	if (firstCreate) {
		action->blockSignals(true);
		action->setChecked(true);
		action->blockSignals(false);
	}

	extraBrowserDocks.push_back(QSharedPointer<QDockWidget>(dock));
	extraBrowserDockActions.push_back(QSharedPointer<QAction>(action));
	extraBrowserDockTargets.push_back(url);
	extraBrowserDockCustomCss.push_back(customCss);
}



// static helper function for setting script of browser
void ExtraBrowsersModel::SetCustomBrowserScriptFromUrlAndCustomCss(QCefWidget* browser, QString url, QString customCss) {
	std::string script;
	//
	/* Add support for Twitch Dashboard panels */
	if (url.contains("twitch.tv/popout") &&
		url.contains("dashboard/live")) {
		QRegularExpression re("twitch.tv\\/popout\\/([^/]+)\\/");
		QRegularExpressionMatch match = re.match(url);
		QString username = match.captured(1);
		if (username.length() > 0) {
			script +=
				"Object.defineProperty(document, 'referrer', { get: () => '";
			script += "https://twitch.tv/";
			script += QT_TO_UTF8(username);
			script += "/dashboard/live";
			script += "'});";
		}
	}

	// Custom css
	if (customCss != "") {
		// code taken from OnLoadEnd() in browser-client.cpp
		// unfortunately CefURIEncode doesnt seem to be available to base OBS so we require user to uri encode it in the url
		//		std::string uriEncodedCSS = CefURIEncode(customCss, false).ToString();
		auto CefURIEncodePrivate = [](std::string str, bool use_plus) {
			// from https://stackoverflow.com/questions/154536/encode-decode-urls-in-c
			std::string new_str = "";
			char c;
			int ic;
			const char* chars = str.c_str();
			char bufHex[10];
			size_t len = strlen(chars);

			for (int i = 0; i < len; i++) {
				c = chars[i];
				ic = c;
				// uncomment this if you want to encode spaces with +
				if (use_plus && c == ' ') {
					new_str += '+';
				}
				else if (isalnum(c) || c == '-' || c == '_' ||
					c == '.' || c == '~') {
					new_str += c;
				}
				else {
					sprintf(bufHex, "%X", c);
					if (ic < 16)
						new_str += "%0";
					else
						new_str += "%";
					new_str += bufHex;
				}
			}
			return new_str;
		};
		std::string uriEncodedCSS = CefURIEncodePrivate(customCss.toStdString(), false);
		script += "const obsCSS = document.createElement('style');";
		script += "obsCSS.innerHTML = decodeURIComponent(\"" + uriEncodedCSS + "\");";
		script += "document.querySelector('head').appendChild(obsCSS);";
	}
	// we run this even if blank so that we can delete any previous custom script if updating browser
	if (true || script != "") {
		browser->setStartupScript(script);
	}
}
