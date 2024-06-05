#pragma once

#include <functional>
#include <QFuture>
#include <QtGlobal>

template<typename T> struct FutureHolder {
	std::function<void()> cancelAll;
	QFuture<T> future;
};

QFuture<void> CreateFuture();

template<typename T> inline QFuture<T> PreventFutureDeadlock(QFuture<T> future)
{
	/*
	* QFutures deadlock if there are continuations on the same thread that
	* need to wait for the previous continuation to finish, see
	* https://github.com/qt/qtbase/commit/59e21a536f7f81625216dc7a621e7be59919da33
	*
	* related bugs:
	* https://bugreports.qt.io/browse/QTBUG-119406
	* https://bugreports.qt.io/browse/QTBUG-119103
	* https://bugreports.qt.io/browse/QTBUG-117918
	* https://bugreports.qt.io/browse/QTBUG-119579
	* https://bugreports.qt.io/browse/QTBUG-119810
	* @RytoEX's summary:
	* QTBUG-119406 and QTBUG-119103 affect Qt 6.6.0 and are fixed in Qt 6.6.2 and 6.7.0+.
	* QTBUG-119579 and QTBUG-119810 affect Qt 6.6.1 and are fixed in Qt 6.6.2 and 6.7.0+.
	* QTBUG-117918 is the only strange one that seems to possibly affect all Qt 6.x versions
	* until 6.6.2, but only in Debug builds.
	*
	* To fix this, move relevant QFutures to another thread before resuming
	* on main thread for affected Qt versions
	*/
#if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)) && \
	(QT_VERSION < QT_VERSION_CHECK(6, 6, 2))
	if (future.isFinished()) {
		return future;
	}

	return future.then(QtFuture::Launch::Async, [](T val) { return val; });
#else
	return future;
#endif
}
