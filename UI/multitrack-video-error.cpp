#include "multitrack-video-error.hpp"

#include <QMessageBox>
#include "obs-app.hpp"

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

bool MultitrackVideoError::ShowDialog(
	QWidget *parent, const QString &multitrack_video_name) const
{
	QMessageBox mb(parent);
	mb.setTextFormat(Qt::RichText);
	mb.setWindowTitle(QTStr("Output.StartStreamFailed"));

	if (type == Type::Warning) {
		mb.setText(
			error +
			QTStr("FailedToStartStream.WarningRetryNonMultitrackVideo")
				.arg(multitrack_video_name));
		mb.setIcon(QMessageBox::Warning);
		mb.setStandardButtons(QMessageBox::StandardButton::Yes |
				      QMessageBox::StandardButton::No);
		return mb.exec() == QMessageBox::StandardButton::Yes;
	} else if (type == Type::Critical) {
		mb.setText(error);
		mb.setIcon(QMessageBox::Critical);
		mb.setStandardButtons(
			QMessageBox::StandardButton::Ok); // cannot continue
		mb.exec();
	}

	return false;
}
