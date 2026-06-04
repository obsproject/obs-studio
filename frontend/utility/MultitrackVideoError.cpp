#include "MultitrackVideoError.hpp"

#include <OBSApp.hpp>

#include <QMessageBox>
#include <QPushButton>

MultitrackVideoError MultitrackVideoError::critical(QString error)
{
	return {Type::Critical, error};
}

MultitrackVideoError MultitrackVideoError::warning(QString error)
{
	return {Type::Warning, error};
}

MultitrackVideoError MultitrackVideoError::cancel()
{
	return {Type::Cancel, {}};
}

bool MultitrackVideoError::ShowDialog(QWidget *parent, const QString &multitrack_video_name) const
{
	QMessageBox mb(parent);
	mb.setTextFormat(Qt::RichText);
	mb.setWindowTitle(QTStr("Output.StartStreamFailed"));

	if (type == Type::Warning) {
		mb.setText(error +
			   QTStr("FailedToStartStream.WarningRetryNonMultitrackVideo").arg(multitrack_video_name));
		mb.setIcon(QMessageBox::Warning);
		QAbstractButton *yesButton = mb.addButton(QTStr("Yes"), QMessageBox::YesRole);
		mb.addButton(QTStr("No"), QMessageBox::NoRole);
		mb.exec();

		return mb.clickedButton() == yesButton;
	} else if (type == Type::Critical) {
		mb.setText(error);
		mb.setIcon(QMessageBox::Critical);
		mb.setStandardButtons(QMessageBox::StandardButton::Ok); // cannot continue
		mb.exec();
	}

	return false;
}
