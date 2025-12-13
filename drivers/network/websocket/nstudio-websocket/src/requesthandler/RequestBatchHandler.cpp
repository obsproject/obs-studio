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

#include <queue>
#include <condition_variable>
#include <util/profiler.hpp>

#include "RequestBatchHandler.h"
#include "../utils/Compat.h"
#include "../obs-websocket.h"

struct SerialFrameBatch {
	RequestHandler &requestHandler;
	std::queue<RequestBatchRequest> requests;
	std::vector<RequestResult> results;
	json &variables;
	bool haltOnFailure;

	size_t frameCount = 0;
	size_t sleepUntilFrame = 0;
	std::mutex conditionMutex;
	std::condition_variable condition;

	SerialFrameBatch(RequestHandler &requestHandler, json &variables, bool haltOnFailure)
		: requestHandler(requestHandler),
		  variables(variables),
		  haltOnFailure(haltOnFailure)
	{
	}
};

struct ParallelBatchResults {
	RequestHandler &requestHandler;
	std::vector<RequestResult> results;

	std::mutex conditionMutex;
	std::condition_variable condition;

	ParallelBatchResults(RequestHandler &requestHandler) : requestHandler(requestHandler) {}
};

// `{"inputName": "inputNameVariable"}` is essentially `inputName = inputNameVariable`
static void PreProcessVariables(const json &variables, RequestBatchRequest &request)
{
	if (variables.empty() || !request.InputVariables.is_object() || request.InputVariables.empty() ||
	    !request.RequestData.is_object())
		return;

	for (auto &[key, value] : request.InputVariables.items()) {
		if (!value.is_string()) {
			blog_debug(
				"[WebSocketServer::ProcessRequestBatch] Value of field `%s` in `inputVariables `is not a string. Skipping!",
				key.c_str());
			continue;
		}

		std::string valueString = value;
		if (!variables.contains(valueString)) {
			blog_debug(
				"[WebSocketServer::ProcessRequestBatch] `inputVariables` requested variable `%s`, but it does not exist. Skipping!",
				valueString.c_str());
			continue;
		}

		request.RequestData[key] = variables[valueString];
	}

	request.HasRequestData = !request.RequestData.empty();
}

// `{"sceneItemIdVariable": "sceneItemId"}` is essentially `sceneItemIdVariable = sceneItemId`
static void PostProcessVariables(json &variables, const RequestBatchRequest &request, const RequestResult &requestResult)
{
	if (!request.OutputVariables.is_object() || request.OutputVariables.empty() || requestResult.ResponseData.empty())
		return;

	for (auto &[key, value] : request.OutputVariables.items()) {
		if (!value.is_string()) {
			blog_debug(
				"[WebSocketServer::ProcessRequestBatch] Value of field `%s` in `outputVariables` is not a string. Skipping!",
				key.c_str());
			continue;
		}

		std::string valueString = value;
		if (!requestResult.ResponseData.contains(valueString)) {
			blog_debug(
				"[WebSocketServer::ProcessRequestBatch] `outputVariables` requested responseData field `%s`, but it does not exist. Skipping!",
				valueString.c_str());
			continue;
		}

		variables[key] = requestResult.ResponseData[valueString];
	}
}

static void ObsTickCallback(void *param, float)
{
	ScopeProfiler prof{"obs_websocket_request_batch_frame_tick"};

	auto serialFrameBatch = static_cast<SerialFrameBatch *>(param);

	// Increment frame count
	serialFrameBatch->frameCount++;

	if (serialFrameBatch->sleepUntilFrame) {
		if (serialFrameBatch->frameCount < serialFrameBatch->sleepUntilFrame)
			// Do not process any requests if in "sleep mode"
			return;
		else
			// Reset frame sleep until counter if not being used
			serialFrameBatch->sleepUntilFrame = 0;
	}

	// Begin recursing any unprocessed requests
	while (!serialFrameBatch->requests.empty()) {
		// Fetch first in queue
		RequestBatchRequest request = serialFrameBatch->requests.front();
		// Pre-process batch variables
		PreProcessVariables(serialFrameBatch->variables, request);
		// Process request and get result
		RequestResult requestResult = serialFrameBatch->requestHandler.ProcessRequest(request);
		// Post-process batch variables
		PostProcessVariables(serialFrameBatch->variables, request, requestResult);
		// Add to results vector
		serialFrameBatch->results.push_back(requestResult);
		// Remove from front of queue
		serialFrameBatch->requests.pop();

		// If haltOnFailure and the request failed, clear the queue to make the batch return early.
		if (serialFrameBatch->haltOnFailure && requestResult.StatusCode != RequestStatus::Success) {
			serialFrameBatch->requests = std::queue<RequestBatchRequest>();
			break;
		}

		// If the processed request tells us to sleep, do so accordingly
		if (requestResult.SleepFrames) {
			serialFrameBatch->sleepUntilFrame = serialFrameBatch->frameCount + requestResult.SleepFrames;
			break;
		}
	}

	// If request queue is empty, we can notify the paused worker thread
	if (serialFrameBatch->requests.empty())
		serialFrameBatch->condition.notify_one();
}

std::vector<RequestResult>
RequestBatchHandler::ProcessRequestBatch(QThreadPool &threadPool, SessionPtr session,
					 RequestBatchExecutionType::RequestBatchExecutionType executionType,
					 std::vector<RequestBatchRequest> &requests, json &variables, bool haltOnFailure)
{
	RequestHandler requestHandler(session);
	if (executionType == RequestBatchExecutionType::SerialRealtime) {
		std::vector<RequestResult> ret;

		// Recurse all requests in batch serially, processing the request then moving to the next one
		for (auto &request : requests) {
			PreProcessVariables(variables, request);

			RequestResult requestResult = requestHandler.ProcessRequest(request);

			PostProcessVariables(variables, request, requestResult);

			ret.push_back(requestResult);

			if (haltOnFailure && requestResult.StatusCode != RequestStatus::Success)
				break;
		}

		return ret;
	} else if (executionType == RequestBatchExecutionType::SerialFrame) {
		SerialFrameBatch serialFrameBatch(requestHandler, variables, haltOnFailure);

		// Create Request objects in the worker thread (avoid unnecessary processing in graphics thread)
		for (auto &request : requests)
			serialFrameBatch.requests.push(request);

		// Create a callback entry for the graphics thread to execute on each video frame
		obs_add_tick_callback(ObsTickCallback, &serialFrameBatch);

		// Wait until the graphics thread processes the last request in the queue
		std::unique_lock<std::mutex> lock(serialFrameBatch.conditionMutex);
		serialFrameBatch.condition.wait(lock, [&serialFrameBatch] { return serialFrameBatch.requests.empty(); });

		// Remove the created callback entry since we don't need it anymore
		obs_remove_tick_callback(ObsTickCallback, &serialFrameBatch);

		return serialFrameBatch.results;
	} else if (executionType == RequestBatchExecutionType::Parallel) {
		ParallelBatchResults parallelResults(requestHandler);

		// Acquire the lock early to prevent the batch from finishing before we're ready
		std::unique_lock<std::mutex> lock(parallelResults.conditionMutex);

		// Submit each request as a task to the thread pool to be processed ASAP
		for (auto &request : requests) {
			threadPool.start(Utils::Compat::CreateFunctionRunnable([&parallelResults, &request]() {
				RequestResult requestResult = parallelResults.requestHandler.ProcessRequest(request);

				std::unique_lock<std::mutex> lock(parallelResults.conditionMutex);
				parallelResults.results.push_back(requestResult);
				lock.unlock();
				parallelResults.condition.notify_one();
			}));
		}

		// Wait for the last request to finish processing
		size_t requestCount = requests.size();
		parallelResults.condition.wait(lock, [&parallelResults, requestCount] {
			return parallelResults.results.size() == requestCount;
		});

		return parallelResults.results;
	}

	// Return empty vector if not a batch somehow
	return std::vector<RequestResult>();
}
