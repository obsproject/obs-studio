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

#pragma once

#include <QObject>
#include <QString>
#include <QThreadPool>

#include <atomic>
#include <memory>

/* Moves a finished recording segment into another folder in the background.
 *
 * When a recording is split, the output signals the switch to the next file
 * before the previous one is necessarily closed (ffmpeg_muxer finalizes it in
 * a separate process). The mover therefore waits until the previous file is
 * finalized (the next file exists, or the recording stopped) and retries the
 * move while the file is still locked, so a half-written file is never moved. */
class RecordingSegmentMover : public QObject {
	Q_OBJECT

	QThreadPool pool;
	std::shared_ptr<std::atomic_bool> abort;

public:
	explicit RecordingSegmentMover(QObject *parent = nullptr);
	~RecordingSegmentMover();

	/* source:   finished segment to move
	 * nextFile: file the recording continues in, empty if the recording stopped
	 * folder:   destination folder (created if needed) */
	void Move(const QString &source, const QString &nextFile, const QString &folder);

	static QString UniqueDestination(const QString &folder, const QString &fileName);

signals:
	void Finished(const QString &source, const QString &destination, bool success);
};
