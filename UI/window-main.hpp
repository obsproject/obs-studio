#pragma once

#include <QMainWindow>
#include <QEvent>

#include <util/config-file.h>

class OBSMainWindow : public QMainWindow {
	Q_OBJECT

public:
	inline OBSMainWindow(QWidget *parent) : QMainWindow(parent) {}

	virtual config_t *Config() const=0;
	virtual void OBSInit()=0;

	virtual int GetProfilePath(char *path, size_t size, const char *file)
		const=0;
};

class QMidiEvent : public QEvent {
	std::vector<uint8_t> _message;
	double _deltaTime;
public:
	static const QEvent::Type midiType = static_cast<QEvent::Type>(QEvent::User + 0x4D49);
	QMidiEvent(std::vector<uint8_t> message, double deltatime) :
			QEvent(midiType)
	{
		_message = message;
		_deltaTime = deltatime;
	}

	std::vector<uint8_t> getMessage()
	{
		return _message;
	}

	double getDeltaTime()
	{
		return _deltaTime;
	}

	int dataMask(int message)
	{
		return message & 0x7F;
	}
	obs_key_t getKey()
	{
		int m = _message.at(0);
		int h = m & 0xF0;
		int l = m & 0x0F;
		switch (h) {
		case 0x80: // note off
		case 0x90: // note on
		case 0xA0: // polymorphic pressure
			return (obs_key_t)(OBS_MIDI_KEY_CN1 + dataMask(_message.at(1)) );
		case 0xB0: // control change
			return (obs_key_t)(OBS_MIDI_CONTROL0 + dataMask(_message.at(1)) );
		case 0xC0:
			return (obs_key_t)(OBS_MIDI_PROGRAM0 + dataMask(_message.at(1)) );
		case 0xE0:
			return (obs_key_t)(OBS_MIDI_PITCH_WHEEL0 + l);
		default:
			return OBS_KEY_NONE;
		}
	}

	bool noteOff()
	{
		int m = _message.at(0);
		int h = m & 0xF0;
		return h == 0x80;
	}

	bool noteOn()
	{
		int m = _message.at(0);
		int h = m & 0xF0;
		return h == 0x90;
	}

	bool notePressed()
	{
		return noteOn() && _message.at(2) > 0;
	}

	bool controlPressed()
	{
		int m = _message.at(0);
		int h = m & 0xF0;
		int l = m & 0x0F;
		if (h == 0xB0) {
			int m1 = _message.at(1);
			if (m1 < 64)
				return _message.at(2) > 0;
			if (m1 < 70)
				return _message.at(2) > 63;
			if (m1 < 96)
				return _message.at(2) > 0;
			return false;

		} else {
			return false;
		}
			
	}

	int getChannel()
	{
		int m = _message.at(0);
		int h = m & 0xF0;
		h = h >> 4;
		int l = m & 0x0F;
		if (h >= 8 && h < 15)
			return l;
		else
			return -1;
	}
};
