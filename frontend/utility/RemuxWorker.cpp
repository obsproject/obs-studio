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

#include "RemuxWorker.hpp"

#include <media-io/media-remux.h>
#include <qt-wrappers.hpp>

void RemuxWorker::UpdateProgress(float percent)
{
	if (abs(lastProgress - percent) < 0.1f)
		return;

	emit updateProgress(percent);
	lastProgress = percent;
}

void RemuxWorker::remux(const QString &source, const QString &target)
{
	isWorking = true;

	auto callback = [](void *data, float percent) {
		RemuxWorker *rw = static_cast<RemuxWorker *>(data);

		QMutexLocker lock(&rw->updateMutex);

		rw->UpdateProgress(percent);

		return rw->isWorking;
	};

	bool stopped = false;
	bool success = false;

	media_remux_job_t mr_job = nullptr;
	if (media_remux_job_create(&mr_job, QT_TO_UTF8(source), QT_TO_UTF8(target))) {

		success = media_remux_job_process(mr_job, callback, this);

		media_remux_job_destroy(mr_job);

		stopped = !isWorking;
	}

	isWorking = false;

	emit remuxFinished(!stopped && success);
}
