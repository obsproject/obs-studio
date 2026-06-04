#pragma once

#include <QString>

class QWidget;

struct MultitrackVideoError {
	static MultitrackVideoError critical(QString error);
	static MultitrackVideoError warning(QString error);
	static MultitrackVideoError cancel();

	bool ShowDialog(QWidget *parent, const QString &multitrack_video_name) const;

	enum struct Type {
		Critical,
		Warning,
		Cancel,
	};

	const Type type;
	const QString error;
};
