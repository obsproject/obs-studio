/******************************************************************************
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

#include "RecordingSegmentMover.hpp"

#include <qt-wrappers.hpp>
#include <util/base.h>
#include <util/threading.h>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#include "moc_RecordingSegmentMover.cpp"

extern volatile bool recording_active;

namespace {
/* How long to wait for the muxer to finalize the previous file. */
constexpr qint64 FinalizeTimeoutMs = 60 * 1000;
/* How long to keep retrying while the file is still locked. */
constexpr qint64 MoveTimeoutMs = 120 * 1000;
constexpr unsigned long PollIntervalMs = 100;
constexpr unsigned long RetryIntervalMs = 250;

bool SamePath(const QString &a, const QString &b)
{
#ifdef _WIN32
	return QDir::cleanPath(a).compare(QDir::cleanPath(b), Qt::CaseInsensitive) == 0;
#else
	return QDir::cleanPath(a) == QDir::cleanPath(b);
#endif
}

/* Waits until the previous segment is closed: either the output already opened
 * the next file (which only happens after the previous one was finalized), or
 * the recording stopped altogether. */
void WaitForFinalized(const QString &nextFile, const std::atomic_bool &abort)
{
	QElapsedTimer timer;
	timer.start();

	while (!abort && timer.elapsed() < FinalizeTimeoutMs) {
		if (nextFile.isEmpty() || QFileInfo::exists(nextFile) || !os_atomic_load_bool(&recording_active)) {
			return;
		}
		QThread::msleep(PollIntervalMs);
	}
}
} // namespace

RecordingSegmentMover::RecordingSegmentMover(QObject *parent)
	: QObject(parent),
	  abort(std::make_shared<std::atomic_bool>(false))
{
	pool.setMaxThreadCount(1);
}

RecordingSegmentMover::~RecordingSegmentMover()
{
	*abort = true;
	pool.waitForDone();
}

QString RecordingSegmentMover::UniqueDestination(const QString &folder, const QString &fileName)
{
	QDir dir(folder);
	QString candidate = dir.filePath(fileName);
	if (!QFileInfo::exists(candidate)) {
		return candidate;
	}

	QFileInfo fi(fileName);
	QString base = fi.completeBaseName();
	QString suffix = fi.suffix().isEmpty() ? QString() : "." + fi.suffix();

	for (int i = 2;; i++) {
		candidate = dir.filePath(QStringLiteral("%1 (%2)%3").arg(base).arg(i).arg(suffix));
		if (!QFileInfo::exists(candidate)) {
			return candidate;
		}
	}
}

void RecordingSegmentMover::Move(const QString &source, const QString &nextFile, const QString &folder)
{
	std::shared_ptr<std::atomic_bool> abortFlag = abort;

	pool.start([this, abortFlag, source, nextFile, folder]() {
		QString destination;
		bool success = false;

		WaitForFinalized(nextFile, *abortFlag);

		if (!QDir().mkpath(folder)) {
			blog(LOG_WARNING, "[Split to Folder] Could not create folder '%s'", QT_TO_UTF8(folder));
		} else if (SamePath(QFileInfo(source).absolutePath(), QDir(folder).absolutePath())) {
			/* Already where the user wants it */
			destination = source;
			success = true;
		} else {
			QElapsedTimer timer;
			timer.start();

			while (!*abortFlag) {
				if (!QFileInfo::exists(source)) {
					blog(LOG_WARNING, "[Split to Folder] Segment '%s' no longer exists",
					     QT_TO_UTF8(source));
					break;
				}

				destination = UniqueDestination(folder, QFileInfo(source).fileName());

				/* QFile::rename falls back to copy + delete across
				 * volumes, and fails while the muxer still has the
				 * file open on Windows; retry until it is released. */
				QFile file(source);
				if (file.rename(destination)) {
					success = true;
					break;
				}

				if (timer.elapsed() >= MoveTimeoutMs) {
					blog(LOG_WARNING, "[Split to Folder] Failed to move '%s' to '%s': %s",
					     QT_TO_UTF8(source), QT_TO_UTF8(destination),
					     QT_TO_UTF8(file.errorString()));
					break;
				}

				QThread::msleep(RetryIntervalMs);
			}
		}

		if (success) {
			blog(LOG_INFO, "[Split to Folder] Saved recording segment to '%s'", QT_TO_UTF8(destination));
		}

		if (!*abortFlag) {
			emit Finished(source, destination, success);
		}
	});
}
