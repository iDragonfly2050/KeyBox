#pragma once
#include <cctype>
#include <string>
#include <set>
#include <algorithm>
#include <boost/serialization/access.hpp>

static const size_t NAME_SIZE = 20;

class Key {
public:
	Key() : Key(-1, -1, "") { }
	Key(int vkCode, int scanCode);
	Key(int vkCode, int scanCode, const char * name);
	Key(const Key & key);

	int getVkCode() const { return _vkCode; }
	int getScanCode() const { return _scanCode; }
	const char * getName() const;

	bool isNumber() const;
	bool isNumPadNumber() const;
	bool isAlpha() const;
	bool isCombinedKey() const;

	char getNumberChar() const;
	char getAlphaChar() const;

	bool operator==(const Key & b) const;
	bool operator==(const char * keyName) const;
	const Key & operator=(const Key & key);

private:
	int _vkCode;
	int _scanCode;
	char _name[NAME_SIZE];

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & _vkCode;
		ar & _scanCode;
		ar & _name;
	}
};
