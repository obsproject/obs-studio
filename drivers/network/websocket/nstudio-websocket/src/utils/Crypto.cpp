/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
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

#include <QByteArray>
#include <QCryptographicHash>
#include <QRandomGenerator>

#include "Crypto.h"
#include "plugin-macros.generated.h"

static const char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const int allowedCharsCount = static_cast<int>(sizeof(allowedChars) - 1);

std::string Utils::Crypto::GenerateSalt()
{
	// Get OS seeded random number generator
	QRandomGenerator *rng = QRandomGenerator::global();

	// Generate 32 random chars
	const size_t randomCount = 32;
	QByteArray randomChars;
	for (size_t i = 0; i < randomCount; i++)
		randomChars.append((char)rng->bounded(255));

	// Convert the 32 random chars to a base64 string
	return randomChars.toBase64().toStdString();
}

std::string Utils::Crypto::GenerateSecret(std::string password, std::string salt)
{
	// Create challenge hash
	auto challengeHash = QCryptographicHash(QCryptographicHash::Algorithm::Sha256);
	// Add password bytes to hash
	challengeHash.addData(QByteArray::fromStdString(password));
	// Add salt bytes to hash
	challengeHash.addData(QByteArray::fromStdString(salt));

	// Generate SHA256 hash then encode to Base64
	return challengeHash.result().toBase64().toStdString();
}

bool Utils::Crypto::CheckAuthenticationString(std::string secret, std::string challenge, std::string authenticationString)
{
	// Concatenate auth secret with the challenge sent to the user
	QString secretAndChallenge = "";
	secretAndChallenge += QString::fromStdString(secret);
	secretAndChallenge += QString::fromStdString(challenge);

	// Generate a SHA256 hash of secretAndChallenge
	auto hash = QCryptographicHash::hash(secretAndChallenge.toUtf8(), QCryptographicHash::Algorithm::Sha256);

	// Encode the SHA256 hash to Base64
	std::string expectedAuthenticationString = hash.toBase64().toStdString();

	return (authenticationString == expectedAuthenticationString);
}

std::string Utils::Crypto::GeneratePassword(size_t length)
{
	// Get OS random number generator
	QRandomGenerator *rng = QRandomGenerator::system();

	// Fill string with random alphanumeric
	std::string ret;
	for (size_t i = 0; i < length; i++)
		ret += allowedChars[rng->bounded(0, allowedCharsCount)];

	return ret;
}
