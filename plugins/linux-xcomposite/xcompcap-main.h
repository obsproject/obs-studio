#pragma once

#include <obs-plugin.hpp>

struct XCompcapMain_private;

class XCompcapMain : public IObsVideoSourcePlugin
{
	public:
	static const char *getId();
	static const char *getName();

	static bool initialize();
	static void deinitialize();

	static obs_properties_t properties();
	static void defaults(obs_data_t settings);

	XCompcapMain(obs_data_t settings, obs_source_t source);
	~XCompcapMain();

	void update(obs_data_t settings);

	void tick(float seconds);
	void render(effect_t effect);

	uint32_t getWidth();
	uint32_t getHeight();

	private:
	XCompcapMain_private *p;
};
