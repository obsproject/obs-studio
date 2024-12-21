#pragma once

#include <QObject>

class Auth : public QObject {
	Q_OBJECT

protected:
	virtual void SaveInternal() = 0;
	virtual bool LoadInternal() = 0;

	bool firstLoad = true;

	struct ErrorInfo {
		std::string message;
		std::string error;

		ErrorInfo(std::string message_, std::string error_) : message(message_), error(error_) {}
	};

public:
	enum class Type {
		None,
		OAuth_StreamKey,
		OAuth_LinkedAccount,
	};

	struct Def {
		std::string service;
		Type type;
		bool externalOAuth;
		bool usesBroadcastFlow;
	};

	typedef std::function<std::shared_ptr<Auth>()> create_cb;

	inline Auth(const Def &d) : def(d) {}
	virtual ~Auth() {}

	inline Type type() const { return def.type; }
	inline const char *service() const { return def.service.c_str(); }
	inline bool external() const { return def.externalOAuth; }
	inline bool broadcastFlow() const { return def.usesBroadcastFlow; }

	virtual void LoadUI() {}

	virtual void OnStreamConfig() {}

	static std::shared_ptr<Auth> Create(const std::string &service);
	static Type AuthType(const std::string &service);
	static bool External(const std::string &service);
	static void Load();
	static void Save();

protected:
	static void RegisterAuth(const Def &d, create_cb create);

private:
	Def def;
};
