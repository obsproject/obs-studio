#pragma once

class Auth {
protected:
	virtual void SaveInternal()=0;
	virtual bool LoadInternal()=0;
	const char *typeName();

	bool firstLoad = true;

public:
	virtual ~Auth() {}

	enum class Type {
		Twitch
	};

	virtual Type type() const=0;
	virtual void LoadUI() {}

	static bool Load();
	static void Save();
};
