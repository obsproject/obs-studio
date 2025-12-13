/*
obs-websocket
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <QThreadPool>

#include "RequestHandler.h"
#include "rpc/RequestBatchRequest.h"

namespace RequestBatchHandler {
	std::vector<RequestResult> ProcessRequestBatch(QThreadPool &threadPool, SessionPtr session,
						       RequestBatchExecutionType::RequestBatchExecutionType executionType,
						       std::vector<RequestBatchRequest> &requests, json &variables,
						       bool haltOnFailure);
}
