#include "stdafx.h"
#include <string.h>
#include "Key.h"
#include "manager\KeyManager.h"

/////////////////////////////////////////////////////////////////////////
// public:
/////////////////////////////////////////////////////////////////////////
Key::Key(int vkCode, int scanCode) {
	_vkCode = vkCode;
	_scanCode = scanCode;
	strncpy_s(_name, "", sizeof(""));
	const char * name = getName();
	int minLength = min(NAME_SIZE - 1, strlen(name));
	strncpy_s(_name, name, minLength);
	_name[minLength] = '\0';
}

Key::Key(int vkCode, int scanCode, const char * name) {
	_vkCode = vkCode;
	_scanCode = scanCode;
	int minLength = min(NAME_SIZE - 1, strlen(name));
	strncpy_s(_name, name, minLength);
	_name[minLength] = '\0';
}

Key::Key(const Key & key) {
	_vkCode = key._vkCode;
	_scanCode = key._scanCode;
	strcpy_s(_name, key._name);
}

const char * Key::getName() const {
	if (strcmp(_name, "") == 0) {
		return KeyManager::getInstance()->getNameByKey(*this);
	}
	return _name;
}

bool Key::isNumber() const {
	std::string name = getName();
	if (name.size() == 1 && std::isdigit(name[0]))
		return true;
	return false;
}

bool Key::isNumPadNumber() const {
	std::string name = getName();
	std::string num("Num ");
	auto it = std::find_first_of(name.cbegin(), name.cend(), num.cbegin(), num.cend());
	if (name.size() == 5 && it == name.cbegin() && std::isdigit(name[4]))
		return true;
	return false;
}

bool Key::isAlpha() const {
	std::string name = getName();
	if (name.size() == 1 && std::isupper(name[0]))
		return true;
	return false;
}

bool Key::isCombinedKey() const {
	auto keyManager = KeyManager::getInstance();
	if (keyManager->getCombinedKeyStates().count(getName()))
		return true;
	return false;
}

char Key::getNumberChar() const {
	std::string name = getName();
	if (isNumber())
		return name[0];
	else if (isNumPadNumber())
		return name[4];
	else
		throw "ERROR: not a number!";
}

char Key::getAlphaChar() const {
	std::string name = getName();
	if (isAlpha())
		return name[0];
	else
		throw "ERROR: not a alpha!";
}

bool Key::operator==(const Key & b) const {
	if (_vkCode == b._scanCode && _scanCode == b._scanCode)
		return true;
	return false;
}

bool Key::operator==(const char * keyName) const {
	if (strcmp(getName(), keyName) == 0)
		return true;
	return false;
}

const Key & Key::operator=(const Key & key) {
	if (this == &key) {
		return *this;
	}
	_vkCode = key._vkCode;
	_scanCode = key._scanCode;
	strcpy_s(_name, key._name);
	return *this;
}
