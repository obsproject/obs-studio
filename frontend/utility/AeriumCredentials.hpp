#pragma once

#include <QByteArray>

namespace AeriumCredentials {
struct Result {
	bool success;
	QByteArray token;
};

Result Read();
Result Write(const QByteArray &token);
Result Remove();
} // namespace AeriumCredentials
