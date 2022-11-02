#include "settings-warning-box.hpp"

#include "obs-app.hpp"

void OBSWarningBox::AddLabel(bool error, const QString &msg)
{
	QLabel *label = new QLabel();
	label->setObjectName(error ? "errorLabel" : "warningLabel");
	label->setText(msg);
	label->setWordWrap(true);

	labels.push_back(QSharedPointer<QLabel>(label));

	if (labels.size() > 1)
		label->setContentsMargins(0, 10, 0, 0);

	addWidget(label);
}

void OBSWarningBox::UpdateLayout()
{
	labels.clear();

	if (restartIds.size() != 0)
		AddLabel(false, QTStr("Basic.Settings.ProgramRestart"));

	for (int i = 0; i < errors.size(); i++)
		AddLabel(true, errors.values()[i]);

	for (int i = 0; i < warnings.size(); i++)
		AddLabel(false, warnings.values()[i]);
}

void OBSWarningBox::AddWarning(const QString &id, const QString &message)
{
	if (warnings.contains(id))
		return;

	warnings.insert(id, message);

	UpdateLayout();
}

void OBSWarningBox::AddError(const QString &id, const QString &message)
{
	if (errors.contains(id))
		return;

	errors.insert(id, message);

	UpdateLayout();
}

void OBSWarningBox::RemoveWarning(const QString &id)
{
	if (!warnings.contains(id))
		return;

	warnings.remove(id);

	UpdateLayout();
}

void OBSWarningBox::RemoveError(const QString &id)
{
	if (!errors.contains(id))
		return;

	errors.remove(id);

	UpdateLayout();
}

void OBSWarningBox::AddRestartWarning(const QString &id)
{
	if (restartIds.contains(id))
		return;

	size_t size = restartIds.size();

	restartIds << id;

	if (size == 0)
		UpdateLayout();
}

void OBSWarningBox::RemoveRestartWarning(const QString &id)
{
	if (!restartIds.contains(id))
		return;

	restartIds.removeAll(id);

	if (restartIds.size() == 0)
		UpdateLayout();
}

QString OBSWarningBox::GetHTMLErrorsList()
{
	QString list("<ul>");

	for (int i = 0; i < errors.size(); i++)
		list.append(QString("<li>%1</li>").arg(errors.values()[i]));

	list.append("</ul>");

	return list;
}
