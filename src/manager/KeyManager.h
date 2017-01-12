#pragma once

#include <vector>
#include <chrono>
#include <map>
#include <fstream>
#include <boost/serialization/access.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include "event/EventQueue.h"

using namespace std::chrono;

class KeyManager {
public:
	static KeyManager * getInstance() {
		static KeyManager * instance = new KeyManager();
		return instance;
	}

	const Key & getKeyByName(const char * name) const;
	const char * getNameByKey(const Key & key) const;

	EventQueue & getRegister(const char * name);
	bool getCombinedKeyState(const std::string & key) const;
	std::map<std::string, bool> & getCombinedKeyStates() { return _combinedKeyStates; }

	void processCombinedKey(const Key & key, KeyEventState state);

	/////////////////////////////////////////////////////////////
	// 配置保存与载入
	/////////////////////////////////////////////////////////////
	void saveSettings() const {
		std::ofstream ofile("settings.dat", std::ofstream::out | std::ofstream::binary);
		if (ofile.is_open() && ofile.good()) {
			try {
				boost::archive::binary_oarchive oa(ofile);
				oa << *this;
			}
			catch (const std::exception & err) {
				std::cerr << err.what() << std::endl;
			}
		}
	}
	void loadSettings() {
		std::ifstream ifile("settings.dat", std::ifstream::in | std::ifstream::binary);
		if (ifile.is_open() && ifile.good()) {
			try {
				boost::archive::binary_iarchive ia(ifile);
				ia >> *this;
			}
			catch (const std::exception & err) {
				std::cerr << err.what() << std::endl;
			}
		}
	}

	/////////////////////////////////////////////////////////////
	// 寄存器的保存与载入
	/////////////////////////////////////////////////////////////
	void saveRegisters() const {
		std::ofstream ofile("registers.dat", std::ofstream::out | std::ofstream::binary);
		if (ofile.is_open() && ofile.good()) {
			try {
				boost::archive::binary_oarchive oa(ofile);
				oa << _registers;
			}
			catch (const std::exception & err) {
				std::cerr << err.what() << std::endl;
			}
		}
	}

	void loadRegisters() {
		std::ifstream ifile("registers.dat", std::ifstream::in | std::ifstream::binary);
		if (ifile.is_open() && ifile.good()) {
			try {
				decltype(_registers) registers;
				boost::archive::binary_iarchive ia(ifile);
				ia >> registers;
				for (auto & val : registers)
					_registers[val.first] = val.second;
			}
			catch (const std::exception & err) {
				std::cerr << err.what() << std::endl;
			}
		}
	}


	/////////////////////////////////////////////////////////////
	// 录制与播放状态
	/////////////////////////////////////////////////////////////
	bool isOnRecord() const { return _onRecord; }
	void setOnRecord(bool val);

	bool isOnPlay() const { return _onPlay; }
	void setOnPlay(bool val);

	bool isOnQuickRecord() const { return _onQuickRecord; }
	void setOnQuickRecord(bool val);

	bool isOnQuickPlay() const { return _onQucikPlay; }
	void setOnQucikPlay(bool val);

	bool isUserPressStopPlay();
	void setUserPressStopPlay(bool val);

	/////////////////////////////////////////////////////////////
	// 请求寄存器状态
	/////////////////////////////////////////////////////////////
	bool isOnRequestRegister() const { return _onRequestRegister; }
	void setOnRequestRegister(bool val);

	bool isOnRequestQucikRegister() const { return _onRequestQucikRegister; }
	void setOnRequestQucikRegister(bool val) { _onRequestQucikRegister = val; }

	bool isOnRequestRecordRegister() const { return _onRequestRecordRegister; }
	void setOnRequestRecordRegister(bool val) { _onRequestRecordRegister = val; }

	bool isOnRequestPlayRegister() const { return _onRequestPlayRegister; }
	void setOnRequestPlayRegister(bool val) { _onRequestPlayRegister = val; }

	/////////////////////////////////////////////////////////////
	// 寄存器
	/////////////////////////////////////////////////////////////
	EventQueue & getCurrentQuickRegister() { return getRegister(_currentQuickRegister.c_str()); }
	void setCurrentQuickRegister(const std::string & val);

	EventQueue & getCurrentRecordRegister() { return getRegister(_currentRecordRegister.c_str()); }
	void setCurrentRecordRegister(const std::string & val) { _currentRecordRegister = val; }

	EventQueue & getCurrentPlayRegister() { return getRegister(_currentPlayRegister.c_str()); }
	void setCurrentPlayRegister(std::string val) { _currentPlayRegister = val; }

	/////////////////////////////////////////////////////////////
	// 设置录制选项状态
	/////////////////////////////////////////////////////////////
	bool isOnRecordSetting() const { return _onRecordSetting; }
	void setOnRecordSetting(bool val);

	/////////////////////////////////////////////////////////////
	// 设置播放选项状态
	/////////////////////////////////////////////////////////////
	bool isOnPlaySetting() const { return _onPlaySetting; }
	void setOnPlaySetting(bool val);

	bool isOnPlayCountSetting() const { return _onPlayCountSetting; }
	void setOnPlayCountSetting(bool val) { _onPlayCountSetting = val; }

	bool isOnSleepTimeSetting() const { return _onSleepTimeSetting; }
	void setOnSleepTimeSetting(bool val) { _onSleepTimeSetting = val; }

	bool isOnLoopSleepTimeSetting() const { return _onLoopSleepTimeSetting; }
	void setOnLoopSleepTimeSetting(bool val) { _onLoopSleepTimeSetting = val; }

	/////////////////////////////////////////////////////////////
	// 录制选项
	/////////////////////////////////////////////////////////////
	int getMouseMode() const { return _mouseMode; }
	void setMouseMode(int val);

	int getRecordDoubleClick() const { return _recordDoubleClick; }
	void setRecordDoubleClick(int val);

	/////////////////////////////////////////////////////////////
	// 播放选项
	/////////////////////////////////////////////////////////////
	int getPlayCount() const { return _playCount; }
	void setPlayCount(int val);

	int getSleepTime() const { return _sleepTime; }
	void setSleepTime(int val);

	int getLoopSleepTime() const { return _loopSleepTime; }
	void setLoopSleepTime(int val);

	/////////////////////////////////////////////////////////////
	// 记录时间
	/////////////////////////////////////////////////////////////
	std::chrono::time_point<std::chrono::system_clock> getRecordTime() const { return _recordTime; }
	void setRecordTime(std::chrono::time_point<std::chrono::system_clock> val) { _recordTime = val; }

	/////////////////////////////////////////////////////////////
	// 屏幕大小
	/////////////////////////////////////////////////////////////
	int getScreenX() const { return _screenX; }
	int getScreenY() const { return _screenY; }

	/////////////////////////////////////////////////////////////
	// 鼠标坐标最后位置
	/////////////////////////////////////////////////////////////
	int getLastX() const { return _lastX; }
	void setLastX(int val) { _lastX = val; }
	int getLastY() const { return _lastY; }
	void setLastY(int val) { _lastY = val; }

	/////////////////////////////////////////////////////////////
	// 菜单设置档次
	/////////////////////////////////////////////////////////////
	int getSleepTimeByLevel(int level) { return _sleepTimeLevels[level]; }
	int getLoopTimeByLevel(int level) { return _loopTimeLevels[level]; }

private:
	KeyManager();

	// 组合键状态
	std::map<std::string, bool> _combinedKeyStates;
	// 寄存器
	std::map<std::string, EventQueue> _registers;
	// 所有按键
	std::vector<Key> _keys;

	/////////////////////////////////////////////////////////////
	// 录制与播放状态
	/////////////////////////////////////////////////////////////
	bool _onRecord;
	bool _onPlay;
	bool _onQuickRecord;
	bool _onQucikPlay;
	bool _userPressStopPlay;

	/////////////////////////////////////////////////////////////
	// 请求寄存器状态
	/////////////////////////////////////////////////////////////
	bool _onRequestRegister;
	bool _onRequestQucikRegister;
	bool _onRequestRecordRegister;
	bool _onRequestPlayRegister;

	/////////////////////////////////////////////////////////////
	// 寄存器
	/////////////////////////////////////////////////////////////
	std::string _currentQuickRegister;
	std::string _currentRecordRegister;
	std::string _currentPlayRegister;

	/////////////////////////////////////////////////////////////
	// 设置录制选项状态
	/////////////////////////////////////////////////////////////
	bool _onRecordSetting;

	/////////////////////////////////////////////////////////////
	// 设置播放选项状态
	/////////////////////////////////////////////////////////////
	bool _onPlaySetting;
	bool _onPlayCountSetting;
	bool _onSleepTimeSetting;
	bool _onLoopSleepTimeSetting;

	/////////////////////////////////////////////////////////////
	// 录制选项
	/////////////////////////////////////////////////////////////
	int _mouseMode;
	int _recordDoubleClick;

	/////////////////////////////////////////////////////////////
	// 播放选项
	/////////////////////////////////////////////////////////////
	int _playCount;
	int _sleepTime;
	int _loopSleepTime;

	/////////////////////////////////////////////////////////////
	// 记录时间
	/////////////////////////////////////////////////////////////
	time_point<system_clock> _recordTime;

	/////////////////////////////////////////////////////////////
	// 屏幕大小
	/////////////////////////////////////////////////////////////
	int _screenX;
	int _screenY;

	/////////////////////////////////////////////////////////////
	// 鼠标坐标最后位置
	/////////////////////////////////////////////////////////////
	int _lastX;
	int _lastY;

	/////////////////////////////////////////////////////////////
	// 菜单设置档次
	/////////////////////////////////////////////////////////////
	const int _sleepTimeLevels[10] = { 0, 1, 10, 25, 50, 75, 100, 300, 500, 1000 };
	const int _loopTimeLevels[10] = { 0, 10, 50, 100, 300, 500, 750, 1000, 3000, 5000 };

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & _currentQuickRegister;
		ar & _mouseMode;
		ar & _recordDoubleClick;

		ar & _playCount;
		ar & _sleepTime;
		ar & _loopSleepTime;
	}
};
