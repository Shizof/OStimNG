#ifndef UTILITY_HPP
#define UTILITY_HPP

#pragma once

#include <filesystem>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <codecvt>
#include <locale>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <stack>
#include <deque>
#include <random>
#include <regex>
#include <iomanip>

#define TOLERANCE   0.00000001f

#pragma warning(push)
#pragma warning(disable : 4244)

// Helper function object to check if a character is whitespace
struct IsWhitespace
{
	bool operator()(int c) const
	{
		return std::isspace(static_cast<unsigned char>(c));
	}
};

static inline void ltrim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {
		return !std::isspace(static_cast<unsigned char>(c));
		}));
}

// trim from end (in place)
static inline void rtrim(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) {
		return !std::isspace(static_cast<unsigned char>(c));
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s)
{
	ltrim(s);
	rtrim(s);
}

static inline void ltrim(std::wstring& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t c) {
		return !std::iswspace(c);
		}));
}

// trim from end (in place)
static inline void rtrim(std::wstring& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t c) {
		return !std::iswspace(c);
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::wstring& s)
{
	ltrim(s);
	rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s)
{
	ltrim(s);
	return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s)
{
	rtrim(s);
	return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s)
{
	trim(s);
	return s;
}

static inline size_t randomGenerator(size_t min, size_t max)
{
	std::mt19937 rng;
	rng.seed(std::random_device()());
	std::uniform_int_distribution<std::mt19937::result_type> dist(
		static_cast<std::mt19937::result_type>(min),
		static_cast<std::mt19937::result_type>(max)
	);
	return dist(rng);
}



static inline std::vector<std::wstring> split(const std::wstring& s, wchar_t delimiter)
{
	std::vector<std::wstring> tokens;
	std::wstring token;
	std::wstringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.emplace_back(token);
	}
	return tokens;
}

static inline std::vector<std::string> split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.emplace_back(token);
	}
	return tokens;
}

static inline std::vector<std::string> splitMulti(const std::string& s, std::string delimiters)
{
	std::string str = trim_copy(s);

	std::vector<std::string> tokens;
	std::stringstream stringStream(str);
	std::string line;
	while (std::getline(stringStream, line))
	{
		std::size_t prev = 0, pos;
		while ((pos = line.find_first_of(delimiters, prev)) != std::string::npos)
		{
			if (pos > prev)
			{
				std::string token = line.substr(prev, pos - prev);
				trim(token);
				tokens.emplace_back(token);
			}

			prev = pos + 1;
		}
		if (prev < line.length())
		{
			std::string token = line.substr(prev, std::string::npos);
			trim(token);
			tokens.emplace_back(token);
		}
	}
	return tokens;
}

static inline std::vector<std::string> splitMultiNonEmpty(const std::string& s, std::string delimiters)
{
	std::string str = trim_copy(s);

	std::vector<std::string> tokens;
	std::stringstream stringStream(str);
	std::string line;
	while (std::getline(stringStream, line))
	{
		std::size_t prev = 0, pos;
		while ((pos = line.find_first_of(delimiters, prev)) != std::string::npos)
		{
			if (pos > prev)
			{
				std::string token = line.substr(prev, pos - prev);
				trim(token);
				if (token.size() > 0)
					tokens.emplace_back(token);
			}

			prev = pos + 1;
		}
		if (prev < line.length())
		{
			std::string token = line.substr(prev, std::string::npos);
			trim(token);
			if (token.size() > 0)
				tokens.emplace_back(token);
		}
	}
	return tokens;
}

static inline std::vector<std::string> split(const std::string& s, std::string delimiter)
{
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
	{
		token = s.substr(pos_start, pos_end - pos_start);
		trim(token);
		pos_start = pos_end + delim_len;
		res.emplace_back(token);
	}

	std::string lasttoken = s.substr(pos_start);
	trim(lasttoken);
	res.emplace_back(lasttoken);
	return res;
}


static inline std::vector<std::string> splitTrimmed(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		trim(token);
		tokens.emplace_back(token);
	}
	return tokens;
}

static inline void skipComments(std::wstring& str)
{
	auto pos = str.find(L"#");
	if (pos != std::wstring::npos)
	{
		str.erase(pos);
	}
}

static inline void skipComments(std::string& str)
{
	auto pos = str.find("#");
	if (pos != std::string::npos)
	{
		str.erase(pos);
	}
}

static inline void skipCommentsComma(std::wstring& str)
{
	auto pos = str.find(L";");
	if (pos != std::wstring::npos)
	{
		str.erase(pos);
	}
}

static inline void skipCommentsComma(std::string& str)
{
	auto pos = str.find(";");
	if (pos != std::string::npos)
	{
		str.erase(pos);
	}
}

static inline std::wstring GetConfigSettingsValue(std::wstring line, std::wstring& variable)
{
	std::wstring valuestr = L"";
	std::vector<std::wstring> splittedLine = split(line, L'=');
	variable = L"";
	if (splittedLine.size() > 1)
	{
		variable = splittedLine[0];
		trim(variable);

		valuestr = splittedLine[1];
		trim(valuestr);
	}
	else
	{
		variable = line;
		trim(variable);
		valuestr = L"0";
	}

	return valuestr;
}

static inline int GetConfigSettingsValueInt(std::string line, std::string& variable)
{
	int value = 0;
	std::vector<std::string> splittedLine = split(line, '=');
	variable = "";
	if (splittedLine.size() > 1)
	{
		variable = splittedLine[0];
		trim(variable);

		std::string valuestr = splittedLine[1];
		trim(valuestr);
		value = std::stoi(valuestr);
	}

	return value;
}


static inline float GetConfigSettingsFloatValue(std::string line, std::string& variable)
{
	float value = 0;
	std::vector<std::string> splittedLine = split(line, '=');
	variable = "";
	if (splittedLine.size() > 1)
	{
		variable = splittedLine[0];
		trim(variable);

		std::string valuestr = splittedLine[1];
		trim(valuestr);
		value = strtof(valuestr.c_str(), 0);
	}

	return value;
}

static inline std::string GetConfigSettingsValue(std::string line, std::string& variable)
{
	if (!line.empty() && line.back() == '=') 
	{
		line.push_back(' ');
	}
	std::string valuestr = "";
	std::vector<std::string> splittedLine = split(line, '=');
	variable = "";
	if (splittedLine.size() > 1)
	{
		variable = splittedLine[0];
		trim(variable);

		valuestr = splittedLine[1];
		trim(valuestr);
	}
	else
	{
		variable = line;
		trim(variable);
		valuestr = "0";
	}

	return valuestr;
}

static inline std::string GetConfigSettingsValue3(std::string line, std::string& variable, std::string& variable2)
{
	std::string valuestr = "";
	std::vector<std::string> splittedLine = split(line, '=');
	variable = "";
	if (splittedLine.size() > 1)
	{
		variable = splittedLine[0];
		trim(variable);

		valuestr = splittedLine[1];
		trim(valuestr);

		if (splittedLine.size() > 2)
		{
			variable2 = splittedLine[2];
			trim(variable2);
		}
	}
	else
	{
		variable = line;
		trim(variable);
		valuestr = "0";
	}

	return valuestr;
}

static inline bool Contains(std::string str, std::string ministr)
{
	if (str.find(ministr) != std::string::npos) {
		return true;
	}
	else
		return false;
}

static inline bool ContainsNoCase(const std::string& str, const std::string& ministr)
{
	std::string lowercaseStr = str;
	std::string lowercaseMinistr = ministr;
	std::transform(lowercaseStr.begin(), lowercaseStr.end(), lowercaseStr.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
	std::transform(lowercaseMinistr.begin(), lowercaseMinistr.end(), lowercaseMinistr.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

	if (lowercaseStr.find(lowercaseMinistr) != std::string::npos) {
		return true;
	}
	else {
		return false;
	}
}

//static inline std::vector<std::string> get_all_files_names_within_folder(std::string folder)
//{
//	std::vector<std::string> names;
//
//	DIR* directory = opendir(folder.c_str());
//	struct dirent* direntStruct;
//
//	if (directory != nullptr) {
//		while (direntStruct = readdir(directory)) {
//			names.emplace_back(direntStruct->d_name);
//		}
//	}
//	closedir(directory);
//
//	return names;
//}

static inline bool stringStartsWith(std::string str, std::string prefix)
{
	std::for_each(str.begin(), str.end(), [](char& c)
		{
			c = ::tolower(static_cast<unsigned char>(c));
		});
	if (str.find(prefix) == 0)
		return true;
	else
		return false;
}

static inline bool stringEndsWith(std::string str, std::string const& suffix)
{
	std::for_each(str.begin(), str.end(), [](char& c)
		{
			c = ::tolower(static_cast<unsigned char>(c));
		});
	if (str.length() >= suffix.length())
	{
		return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
	}
	else
	{
		return false;
	}
}


static inline uint32_t getHex(std::string hexstr)
{
	return (uint32_t)strtoul(hexstr.c_str(), 0, 16);
}

template <typename I> static inline std::string num2hex(I w, size_t hex_len = sizeof(I) << 1) {
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

static inline void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

static inline std::string removeWhiteSpaces(std::string s)
{
	std::string s2;
	s2.reserve(s.size());
	std::remove_copy_if(begin(s), end(s), std::back_inserter(s2), [l = std::locale{}](auto ch) { return std::isspace(ch, l); });
	return s2;
}

static inline void removeSubstrings(std::string& s, std::string& p)
{
	std::string::size_type n = p.length();
	for (std::string::size_type i = s.find(p);
		i != std::string::npos;
		i = s.find(p))
	{
		s.erase(i, n);
	}
}

static inline bool has_suffix(const std::string& str, const std::string& suffix)
{
	return str.size() >= suffix.size() &&
		str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
static inline void ConvertToCamelCase(char * &str)
{
	bool active = true;
	for (int i = 0; str[i] != '\0'; i++) {
		if (std::isalpha(str[i])) {
			if (active) {
				str[i] = std::toupper(str[i]);
				active = false;
			}
			else {
				str[i] = std::tolower(str[i]);
			}
		}
		else if (str[i] == ' ') {
			active = true;
		}
	}
}

static inline uint64_t GetButtonMaskFromId(int id)
{
	return 1ull << id;
}

// get mod index from a normal form ID 32 bit unsigned
static inline uint32_t GetModIndex(uint32_t formId)
{
	return formId >> 24;
}

// get base formID (without mod index)
static inline uint32_t GetBaseFormID(uint32_t formId)
{
	return formId & 0x00FFFFFF;
}

// get base formID (without mod index)
static inline uint32_t GetModFormID(uint32_t formId)
{
	return formId & 0xFF000000;
}

// check if mod index is valid (mod index is the upper 8 bits of form ID)
static inline bool IsValidModIndex(uint32_t modIndex)
{
	return modIndex > 0 && modIndex != 0xFF;
}

static inline RE::NiPoint3 GetPointFromRatio(RE::NiPoint3 low, RE::NiPoint3 high, float ratio)
{
	return low + ((high - low) * ratio);
}

static inline float distance2D(RE::NiPoint3 po1, RE::NiPoint3 po2) {
    float x = po1.x - po2.x;
    float y = po1.y - po2.y;
    return sqrtf(x * x + y * y);
}

static inline float distance2DNoSqrt(RE::NiPoint3 po1, RE::NiPoint3 po2) {
    float x = po1.x - po2.x;
    float y = po1.y - po2.y;
    return (x * x + y * y);
}

static inline float distanceNoSqrt(RE::NiPoint3 po1, RE::NiPoint3 po2)
{
	/*LARGE_INTEGER startingTime, endingTime, elapsedMicroseconds;
	LARGE_INTEGER frequency;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&startingTime);
	_MESSAGE("distanceNoSqrt Start");*/

	float x = po1.x - po2.x;
	float y = po1.y - po2.y;
	float z = po1.z - po2.z;
	float result = x * x + y * y + z * z;

	/*QueryPerformanceCounter(&endingTime);
	elapsedMicroseconds.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
	elapsedMicroseconds.QuadPart *= 1000000000LL;
	elapsedMicroseconds.QuadPart /= frequency.QuadPart;
	_MESSAGE("distanceNoSqrt Update Time = %lld ns\n", elapsedMicroseconds.QuadPart);*/
	return result;
}

static inline bool CompareNiPoints(RE::NiPoint3 collisionVector, RE::NiPoint3 emptyPoint)
{
	return collisionVector.x == emptyPoint.x && collisionVector.y == emptyPoint.y && collisionVector.z == emptyPoint.z;
}

// Calculates a dot product
static inline float dot(RE::NiPoint3 a, RE::NiPoint3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Calculates a cross product
static inline RE::NiPoint3 cross(RE::NiPoint3 a, RE::NiPoint3 b)
{
	return RE::NiPoint3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

static inline void GetAttitudeAndHeadingFromTwoPoints(RE::NiPoint3 source, RE::NiPoint3 target, float& attitude, float& heading)
{
	RE::NiPoint3 vector = target - source;

	const float sqr = vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
	if (sqr != 0)
	{
		vector = vector * (1.0f / sqrtf(sqr));

		attitude = (float)atan2(vector.z, sqrtf(vector.y * vector.y + vector.x * vector.x)) * -1;

		heading = (float)atan2(vector.x, vector.y);
	}
	else
	{
		attitude = 0;
		heading = 0;
	}
}

//returns true if clockwise, false if counter clockwise
static inline bool AreAnglesTurningClockwise(float angle1, float angle2, float angle3)
{
	if (angle1 < 0)
		angle1 = 360.0f - abs(angle1);

	if (angle2 < 0)
		angle2 = 360.0f - abs(angle2);

	if (angle3 < 0)
		angle3 = 360.0f - abs(angle3);

	//written by my sister
	if (angle2 > angle1 && angle1 > 180.0f)
	{
		return true;
	}
	else if (angle2 > angle1 && angle1 < 180.0f)
	{
		if ((angle1 + angle2) < 180.0f)
		{
			return true;
		}
		else
		{
			if (angle3 < angle2)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
	}
	else
	{
		return false;
	}
}


//returns true if clockwise, false if counter clockwise
static inline bool AreAnglesTurningClockwiseOLD(float angle1, float angle2, float angle3)
{
	if (angle1 < 0)
		angle1 = 360.0f - abs(angle1);

	if (angle2 < 0)
		angle2 = 360.0f - abs(angle2);

	if (angle3 < 0)
		angle3 = 360.0f - abs(angle3);

	//written by me, hate this code
	if (angle2 - angle1 < 0 && angle3 - angle2 < 0)
	{
		//CCW
		return false;
	}
	else if (angle1 - angle2 < 0 && angle2 - angle3 < 0)
	{
		//CW
		return true;
	}
	else
	{
		if (angle1 - angle2 > 0 && angle2 - angle3 < -180.0f)
		{
			if (angle1 - angle2 > 0 && angle2 - (angle3 - 360.0f) > 0)
			{
				//CCW
				return false;
			}
			else
				return true;
		}
		else if (angle1 - angle2 < -180.0f && angle2 - angle3 > 0)
		{
			if ((angle1 + 360.0f) - angle2 > 0 && angle2 - angle3 > 0)
			{
				//CCW
				return false;
			}
			else
				return true;
		}
		else if (angle1 - angle2 > 180.0f && angle2 - angle3 < 0)
		{
			if ((angle1 - 360.0f) - angle2 < 0 && angle2 - angle3 < 0)
			{
				//CW
				return true;
			}
			else
				return false;
		}
		else if (angle1 - angle2 < 0 && angle2 - angle3 > 180.0f)
		{
			if (angle1 - angle2 < 0 && angle2 - (angle3 + 360.0f) < 0)
			{
				//CW
				return true;
			}
			else
				return false;
		}
		else
			return true;
	}
}

static inline float AngleDifference(float angle1, float angle2)
{
	if ((angle1 < -90 && angle2 > 90) || (angle2 < -90 && angle1 > 90))
	{
		return (180 - abs(angle1)) + (180 - abs(angle2));
	}
	else if ((angle1 > -90 && 0 >= angle1 && angle2 < 90 && 0 <= angle2) || (angle2 > -90 && 0 >= angle2 && angle1 < 90 && 0 <= angle1))
	{
		return abs(angle1) + abs(angle2);
	}
	else
	{
		return abs(angle1 - angle2);
	}
}

template <typename T, int MaxLen, typename Container = std::deque<T>>
class FixedQueue : public std::queue<T, Container> {
public:
	void push(const T& value) {
		if (this->size() == MaxLen) {
			this->c.pop_front();
		}
		std::queue<T, Container>::push(value);
	}
};

//// Names should be the full INI setting name followed by a colon and it's category (Prog's code)
//// For example: bAlwaysShowHands:VR
//static inline float vlibGetSetting(const char * name) {
//	RE::Setting * setting = GetINISetting(name);
//	float value;
//	if (!setting)
//		return -1;
//
//	return setting->GetFloat();
//}


//static inline RE::NiPoint3 ConvertRotation(RE::NiMatrix33 mat)
//{
//	float heading;
//	float attitude;
//	float bank;
//	mat.GetEulerAngles(&heading, &attitude, &bank);
//	return RE::NiPoint3(heading, attitude, bank);
//}

// Check if a pointer is bad by following it, reading memory, and catching any exception (Prog's code)
static inline bool IsBadPtr(const void* p, size_t bytes)
{
	try
	{
		memcmp(p, p, bytes);
	}
	catch (...)
	{
		return true;
	}
	return false;
}

// Controls visibility visibility of an NiNode (Prog's code)
static inline void setVisible(RE::NiAVObject* node, bool isVisible) {
	if (node) {
		if (isVisible)
			*(uint8_t*)(((uintptr_t)node) + 0x10C) &= 0xFE;
		else
			*(uint8_t*)(((uintptr_t)node) + 0x10C) |= 0x01;
	}
}

static inline bool isVisible(RE::NiAVObject* node) {
	if (!node)
		return false;
	bool isHidden = (*(uint8_t*)(((uintptr_t)node) + 0x10C) & 0x01);
	return !isHidden;
}

//// Interpolate between two rotation matrices using quaternion math (Prog's code)
//static inline NiMatrix33 slerpMatrix(float interp, RE::NiMatrix33 mat1, RE::NiMatrix33 mat2) {
//	// Convert mat1 to a quaternion
//	float q1w = sqrt(max(0, 1 + mat1.data[0][0] + mat1.data[1][1] + mat1.data[2][2])) / 2;
//	float q1x = sqrt(max(0, 1 + mat1.data[0][0] - mat1.data[1][1] - mat1.data[2][2])) / 2;
//	float q1y = sqrt(max(0, 1 - mat1.data[0][0] + mat1.data[1][1] - mat1.data[2][2])) / 2;
//	float q1z = sqrt(max(0, 1 - mat1.data[0][0] - mat1.data[1][1] + mat1.data[2][2])) / 2;
//	q1x = _copysign(q1x, mat1.data[2][1] - mat1.data[1][2]);
//	q1y = _copysign(q1y, mat1.data[0][2] - mat1.data[2][0]);
//	q1z = _copysign(q1z, mat1.data[1][0] - mat1.data[0][1]);
//
//	// Convert mat2 to a quaternion
//	float q2w = sqrt(max(0, 1 + mat2.data[0][0] + mat2.data[1][1] + mat2.data[2][2])) / 2;
//	float q2x = sqrt(max(0, 1 + mat2.data[0][0] - mat2.data[1][1] - mat2.data[2][2])) / 2;
//	float q2y = sqrt(max(0, 1 - mat2.data[0][0] + mat2.data[1][1] - mat2.data[2][2])) / 2;
//	float q2z = sqrt(max(0, 1 - mat2.data[0][0] - mat2.data[1][1] + mat2.data[2][2])) / 2;
//	q2x = _copysign(q2x, mat2.data[2][1] - mat2.data[1][2]);
//	q2y = _copysign(q2y, mat2.data[0][2] - mat2.data[2][0]);
//	q2z = _copysign(q2z, mat2.data[1][0] - mat2.data[0][1]);
//
//	// Take the dot product, inverting q2 if it is negative
//	double dot = q1w*q2w + q1x*q2x + q1y*q2y + q1z*q2z;
//	if (dot < 0.0f) {
//		q2w = -q2w;
//		q2x = -q2x;
//		q2y = -q2y;
//		q2z = -q2z;
//		dot = -dot;
//	}
//
//	// Linearly interpolate and normalize if the dot product is too close to 1
//	float q3w, q3x, q3y, q3z;
//	if (dot > 0.9995) {
//		q3w = q1w + interp * (q2w - q1w);
//		q3x = q1x + interp * (q2x - q1x);
//		q3y = q1y + interp * (q2y - q1y);
//		q3z = q1z + interp * (q2z - q1z);
//		float length = sqrtf(q3w*q3w + q3x + q3x + q3y*q3y + q3z*q3z);
//		q3w /= length;
//		q3x /= length;
//		q3y /= length;
//		q3z /= length;
//
//		// Otherwise do a spherical linear interpolation normally
//	}
//	else {
//		float theta_0 = acosf(dot);        // theta_0 = angle between input vectors
//		float theta = theta_0 * interp;    // theta = angle between q1 and result
//		float sin_theta = sinf(theta);     // compute this value only once
//		float sin_theta_0 = sinf(theta_0); // compute this value only once
//		float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
//		float s1 = sin_theta / sin_theta_0;
//		q3w = (s0 * q1w) + (s1 * q2w);
//		q3x = (s0 * q1x) + (s1 * q2x);
//		q3y = (s0 * q1y) + (s1 * q2y);
//		q3z = (s0 * q1z) + (s1 * q2z);
//	}
//
//	// Convert the new quaternion back to a matrix
//	RE::NiMatrix33 result;
//	result.data[0][0] = 1 - (2 * q3y*q3y) - (2 * q3z*q3z);
//	result.data[0][1] = (2 * q3x*q3y) - (2 * q3z*q3w);
//	result.data[0][2] = (2 * q3x*q3z) + (2 * q3y*q3w);
//	result.data[1][0] = (2 * q3x*q3y) + (2 * q3z*q3w);
//	result.data[1][1] = 1 - (2 * q3x*q3x) - (2 * q3z*q3z);
//	result.data[1][2] = (2 * q3y*q3z) - (2 * q3x*q3w);
//	result.data[2][0] = (2 * q3x*q3z) - (2 * q3y*q3w);
//	result.data[2][1] = (2 * q3y*q3z) + (2 * q3x*q3w);
//	result.data[2][2] = 1 - (2 * q3x*q3x) - (2 * q3y*q3y);
//	return result;
//}

static inline float distance(RE::NiPoint3 po1, RE::NiPoint3 po2)
{
	float x = po1.x - po2.x;
	float y = po1.y - po2.y;
	float z = po1.z - po2.z;
	return std::sqrtf(x * x + y * y + z * z);
}

static inline float magnitude(RE::NiPoint3 p)
{
	return sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
}

static inline float magnitude2d(RE::NiPoint3 p)
{
	return sqrtf(p.x * p.x + p.y * p.y);
}

static inline float magnitudePwr2(RE::NiPoint3 p)
{
	return p.x * p.x + p.y * p.y + p.z * p.z;
}

static inline RE::NiPoint3 crossProduct(RE::NiPoint3 A, RE::NiPoint3 B)
{
	return RE::NiPoint3(A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x);
}

static inline float GetPercentageValue(float number1, float number2, float division)
{
	if (division == 1.0f)
		return number2;
	else if (division == 0)
		return number1;
	else
	{
		return number1 + ((number2 - number1) * (division));
	}
}

static inline float CalculateCollisionAmount(const RE::NiPoint3& a, const RE::NiPoint3& b, float Wradius, float Bradius)
{
	float distPwr2 = distanceNoSqrt(a, b);

	float totalRadius = Wradius + Bradius;
	if (distPwr2 < totalRadius * totalRadius)
	{
		return totalRadius - sqrtf(distPwr2);
	}
	else
		return 0;
}

//static inline bool invert(RE::NiMatrix33 matIn, RE::NiMatrix33 & matOut)
//{
//	float inv[9];
//	inv[0] = matIn.data[1][1] * matIn.data[2][2] - matIn.data[2][1] * matIn.data[1][2]; // = (A(1,1)*A(2,2)-A(2,1)*A(1,2))*invdet;
//	inv[1] = matIn.data[1][2] * matIn.data[2][0] - matIn.data[1][0] * matIn.data[2][2]; // = (A(1,2)*A(2,0)-A(1,0)*A(2,2))*invdet;
//	inv[2] = matIn.data[1][0] * matIn.data[2][1] - matIn.data[2][0] * matIn.data[1][1]; // = (A(1,0)*A(2,1)-A(2,0)*A(1,1))*invdet;
//	inv[3] = matIn.data[0][2] * matIn.data[2][1] - matIn.data[0][1] * matIn.data[2][2]; // = (A(0,2)*A(2,1)-A(0,1)*A(2,2))*invdet;
//	inv[4] = matIn.data[0][0] * matIn.data[2][2] - matIn.data[0][2] * matIn.data[2][0]; // = (A(0,0)*A(2,2)-A(0,2)*A(2,0))*invdet;
//	inv[5] = matIn.data[2][0] * matIn.data[0][1] - matIn.data[0][0] * matIn.data[2][1]; // = (A(2,0)*A(0,1)-A(0,0)*A(2,1))*invdet;
//	inv[6] = matIn.data[0][1] * matIn.data[1][2] - matIn.data[0][2] * matIn.data[1][1]; // = (A(0,1)*A(1,2)-A(0,2)*A(1,1))*invdet;
//	inv[7] = matIn.data[1][0] * matIn.data[0][2] - matIn.data[0][0] * matIn.data[1][2]; // = (A(1,0)*A(0,2)-A(0,0)*A(1,2))*invdet;
//	inv[8] = matIn.data[0][0] * matIn.data[1][1] - matIn.data[1][0] * matIn.data[0][1]; // = (A(0,0)*A(1,1)-A(1,0)*A(0,1))*invdet;
//	double determinant =
//		+matIn.data[0][0] * (matIn.data[1][1] * matIn.data[2][2] - matIn.data[2][1] * matIn.data[1][2])  //+A(0,0)*(A(1,1)*A(2,2)-A(2,1)*A(1,2))
//		- matIn.data[0][1] * (matIn.data[1][0] * matIn.data[2][2] - matIn.data[1][2] * matIn.data[2][0])  //-A(0,1)*(A(1,0)*A(2,2)-A(1,2)*A(2,0))
//		+ matIn.data[0][2] * (matIn.data[1][0] * matIn.data[2][1] - matIn.data[1][1] * matIn.data[2][0]); //+A(0,2)*(A(1,0)*A(2,1)-A(1,1)*A(2,0));
//
//																										  // Can't get the inverse if determinant = 0 (divide by zero)
//	if (determinant >= -0.001 || determinant <= 0.001)
//		return false;
//
//	// Invert and return the matrix
//	for (int i = 0; i<9; i++)
//		matOut.data[i / 3][i % 3] = inv[i] / determinant;
//	return true;
//}

static inline float determinant(RE::NiPoint3 a, RE::NiPoint3 b, RE::NiPoint3 c)
{
	float det = 0;

	det = det + ((a.x * b.y * c.z) - (b.z * c.y));
	det = det + ((a.y * b.z * c.x) - (b.x * c.z));
	det = det + ((a.z * b.x * c.y) - (b.y * c.x));

	return det;
}

// Dot product of 2 vectors 
static inline float Dot(RE::NiPoint3 A, RE::NiPoint3 B)
{
	float x1, y1, z1;
	x1 = A.x * B.x;
	y1 = A.y * B.y;
	z1 = A.z * B.z;
	return (x1 + y1 + z1);
}

static inline float clamp(float val, float min, float max) {
	if (val < min) return min;
	else if (val > max) return max;
	return val;
}

static inline RE::NiPoint3 normalize(const RE::NiPoint3& v)
{
	const float length_of_v = sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
	return RE::NiPoint3(v.x / length_of_v, v.y / length_of_v, v.z / length_of_v);
}

//// Gets a rotation matrix to transform one vector to another
//static inline RE::NiMatrix33 getRotation(RE::NiPoint3 a, RE::NiPoint3 b)
//{
//	// Normalize the inputs
//	float l1, l2;
//	l1 = sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
//	l2 = sqrtf(b.x*b.x + b.y*b.y + b.z*b.z);
//	a /= l1;
//	b /= l2;
//
//	// Get the dot product and return an identity matrix if there's not much of an angle
//	RE::NiMatrix33 mat; // mat[row][column]
//	float dotP = a.x*b.x + a.y*b.y + a.z*b.z;
//	if (dotP >= 0.99999) {
//		mat.Identity();
//		return mat;
//	}
//
//	// Get the normalized cross product
//	RE::NiPoint3 crossP = RE::NiPoint3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
//	float cpLen = sqrtf(crossP.x*crossP.x + crossP.y*crossP.y + crossP.z*crossP.z);
//	crossP /= cpLen;
//
//	// Get the angles
//	float phi = acosf(dotP);
//	float rcos = cos(phi);
//	float rsin = sin(phi);
//
//	// Build the matrix
//	mat.data[0][0] = rcos + crossP.x * crossP.x * (1.0 - rcos);
//	mat.data[0][1] = -crossP.z * rsin + crossP.x * crossP.y * (1.0 - rcos);
//	mat.data[0][2] = crossP.y * rsin + crossP.x * crossP.z * (1.0 - rcos);
//	mat.data[1][0] = crossP.z * rsin + crossP.y * crossP.x * (1.0 - rcos);
//	mat.data[1][1] = rcos + crossP.y * crossP.y * (1.0 - rcos);
//	mat.data[1][2] = -crossP.x * rsin + crossP.y * crossP.z * (1.0 - rcos);
//	mat.data[2][0] = -crossP.y * rsin + crossP.z * crossP.x * (1.0 - rcos);
//	mat.data[2][1] = crossP.x * rsin + crossP.z * crossP.y * (1.0 - rcos);
//	mat.data[2][2] = rcos + crossP.z * crossP.z * (1.0 - rcos);
//	return mat;
//}
//
//// Gets a rotation matrix from an axis and an angle
//static inline RE::NiMatrix33 getRotationAxisAngle(RE::NiPoint3 axis, float theta)
//{
//	RE::NiMatrix33 result;
//	// This math was found online http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/
//	double c = cosf(theta);
//	double s = sinf(theta);
//	double t = 1.0 - c;
//	axis = normalize(axis);
//	result.data[0][0] = c + axis.x * axis.x * t;
//	result.data[1][1] = c + axis.y * axis.y * t;
//	result.data[2][2] = c + axis.z * axis.z * t;
//	double tmp1 = axis.x * axis.y * t;
//	double tmp2 = axis.z * s;
//	result.data[1][0] = tmp1 + tmp2;
//	result.data[0][1] = tmp1 - tmp2;
//	tmp1 = axis.x * axis.z * t;
//	tmp2 = axis.y * s;
//	result.data[2][0] = tmp1 - tmp2;
//	result.data[0][2] = tmp1 + tmp2;
//	tmp1 = axis.y * axis.z * t;
//	tmp2 = axis.x * s;
//	result.data[2][1] = tmp1 + tmp2;
//	result.data[1][2] = tmp1 - tmp2;
//	return result;
//}

static inline RE::NiPoint3 NearestPointOnLine(RE::NiPoint3 start, RE::NiPoint3 end, RE::NiPoint3 pnt)
{
	RE::NiPoint3 line = (end - start);
	float len = magnitude(line);
	line = normalize(line);

	RE::NiPoint3 v = pnt - start;
	float d = Dot(v, line);
	d = clamp(d, 0.0f, len);

	return start + line * d;
}

static inline RE::NiPoint3 rotate(const RE::NiPoint3& v, const RE::NiPoint3& axis, float theta)
{
	const float cos_theta = cosf(theta);

	return (v * cos_theta) + (crossProduct(axis, v) * sinf(theta)) + (axis * Dot(axis, v)) * (1 - cos_theta);
}

static inline float angleBetweenVectors(const RE::NiPoint3& v1, const RE::NiPoint3& v2)
{
    return std::acos(dot(v1, v2) / (magnitude(v1) * magnitude(v2))) * 57.2957795131f;
}

static inline float angleBetweenVectorsRad(const RE::NiPoint3& v1, const RE::NiPoint3& v2)
{
	return std::acos(dot(v1, v2) / (magnitude(v1) * magnitude(v2)));
}

static inline bool cmpf(float A, float B, float epsilon = 0.0001f)
{
	return (fabs(A - B) < epsilon);
}

static inline bool HighArch(const std::deque<RE::NiPoint3> points)
{
	if (points.size() > 3)
	{
		int aboveCount = 0;
		int belowCount = 0;
		for (int i = 1; i < points.size() - 2; i++)
		{
			RE::NiPoint3 nearest = NearestPointOnLine(points[0], points[points.size() - 1], points[i]);

			if (nearest.z > points[i].z)
				belowCount++;
			else if (nearest.z < points[i].z)
				aboveCount++;
		}
		//_MESSAGE("--->Above:%d Below:%d", aboveCount, belowCount);
		return aboveCount > belowCount;
	}
	else
		return true;
}

static inline RE::NiPoint3 InterpolateBetweenVectors(const RE::NiPoint3& from, const RE::NiPoint3& to, float percentage)
{
	return normalize((normalize(to) * percentage) + (normalize(from) * (100.0f - percentage))) * magnitude(to);
}


static inline RE::NiPoint3 NoZ(RE::NiPoint3 input)
{
	return RE::NiPoint3(input.x, input.y, 0);
}

static inline RE::NiPoint3 CalculateNormalFromThreePoints(RE::NiPoint3 a, RE::NiPoint3 b, RE::NiPoint3 c)
{
	RE::NiPoint3 dir = cross(b - a, c - a);
	RE::NiPoint3 norm = normalize(dir);
	return norm;
}

//static inline NiMatrix33 MatrixFromAxisAngle(RE::NiPoint3 axis, float theta)
//{
//	RE::NiPoint3 a = axis;
//	float cosTheta = cosf(theta);
//	float sinTheta = sinf(theta);
//	RE::NiMatrix33 result;
//
//	result.data[0][0] = cosTheta + a.x * a.x * (1 - cosTheta);
//	result.data[0][1] = a.x * a.y * (1 - cosTheta) - a.z * sinTheta;
//	result.data[0][2] = a.x * a.z * (1 - cosTheta) + a.y * sinTheta;
//
//	result.data[1][0] = a.y * a.x * (1 - cosTheta) + a.z * sinTheta;
//	result.data[1][1] = cosTheta + a.y * a.y * (1 - cosTheta);
//	result.data[1][2] = a.y * a.z * (1 - cosTheta) - a.x * sinTheta;
//
//	result.data[2][0] = a.z * a.x * (1 - cosTheta) - a.y * sinTheta;
//	result.data[2][1] = a.z * a.y * (1 - cosTheta) + a.x * sinTheta;
//	result.data[2][2] = cosTheta + a.z * a.z * (1 - cosTheta);
//
//	return result;
//}

static inline int GetMaxValueFromArray(int myarray[], int length)
{
	return *std::max_element(myarray, myarray + length);
}

static inline std::vector<std::string> get_all_files_names_within_folder(std::string folder)
{
	std::vector<std::string> names;

	for (const auto& entry : std::filesystem::directory_iterator(folder))
	{
		if (entry.is_regular_file())
		{
			names.push_back(entry.path().filename().string());
		}
	}

	return names;
}

template <typename Value>
static inline std::string describe(Value value)
{
	std::ostringstream os;
	os << value;
	return os.str();
}

static inline std::string GetRuntimePath()
{
	static char appPath[4096] = { 0 };

	if (appPath[0])
		return appPath;

	assert(GetModuleFileNameA(GetModuleHandle(NULL), appPath, sizeof(appPath)));

	return appPath;
}

static inline std::string GetRuntimeName()
{
	std::string appPath = GetRuntimePath();

	std::string::size_type slashOffset = appPath.rfind('\\');
	if (slashOffset == std::string::npos)
		return appPath;

	return appPath.substr(slashOffset + 1);
}

static inline const std::string& GetRuntimeDirectory()
{
	static std::string s_runtimeDirectory;

	if (s_runtimeDirectory.empty())
	{
		std::string	runtimePath = GetRuntimePath();

		// truncate at last slash
		std::string::size_type	lastSlash = runtimePath.rfind('\\');
		if (lastSlash != std::string::npos)	// if we don't find a slash something is VERY WRONG
		{
			s_runtimeDirectory = runtimePath.substr(0, lastSlash + 1);
		}
	}

	return s_runtimeDirectory;
}


static inline RE::NiPoint3 GetForwardVector(float rotationAngleRadians) 
{
    return RE::NiPoint3(std::cos(rotationAngleRadians), std::sin(rotationAngleRadians), 0.0f);
}

// Function to calculate sideways vector from forward vector
static inline RE::NiPoint3 GetSidewaysVector(const RE::NiPoint3& forwardVector) 
{
    return RE::NiPoint3(-forwardVector.y, forwardVector.x, 0.0f); 
}


// Fetches information about a node
inline std::string getNodeDesc(RE::NiAVObject* node) {
    // Use periods to indicate the depth in the scene graph
    if (!node) {
        return "Missing node";
    }

    // Include the node's type followed by its name
    std::string text;
    if (node->GetRTTI() && node->GetRTTI()->name)
        text = text + node->GetRTTI()->name + " ";
    else
        text = text + "UnknownType ";
    if (node->name.empty() == false)
        text = text + node->name.c_str();
    else
        text = text + "Unnamed";

    return text;
}

// Writes information about a node to the log file
inline void logNode(int depth, RE::NiAVObject* node) {
    auto text = std::string(depth, '.') + getNodeDesc(node);
    float h6, a6, b6;
    node->world.rotate.ToEulerAnglesXYZ(h6, a6, b6);
    float h6l, a6l, b6l;
    node->local.rotate.ToEulerAnglesXYZ(h6l, a6l, b6l);
    h6 = h6 * 57.2957795131f;
    a6 = a6 * 57.2957795131f;
    b6 = b6 * 57.2957795131f;
    h6l = h6l * 57.2957795131f;
    a6l = a6l * 57.2957795131f;
    b6l = b6l * 57.2957795131f;
    logger::info("{}: {} -Rot: {}|{}|{} - {}|{}|{}", depth, text.c_str(), h6, a6, b6, h6l, a6l, b6l);
}

// Lists all parents of a bone to the log file
inline void logParents(RE::NiAVObject* bone) {
    RE::NiNode* node = bone ? bone->AsNode() : 0;
    int depth = 1;
    while (node) {
        // auto blanks = std::string(depth, '.');
        // log("%sNode name = %s, RTTI = %s\n", blanks.c_str(), node->m_name, node->GetRTTI()->name);
        logNode(depth, node);
        node = node->parent;
        ++depth;
    }
}

// Lists all children of a bone to the log file, filtering by RTTI type name
inline void logChildren(RE::NiAVObject* bone, int depth, int maxDepth, const char* filter) {
    if (!bone) return;

    if (filter != 0 && filter[0]) {
        if (bone->GetRTTI()->name != nullptr && strcmp(bone->GetRTTI()->name, filter) != 0) {
            return;
        }
    }
    // auto blanks = std::string(depth, '.');
    // log("%sNode name = %s, RTTI = %s\n", blanks.c_str(), bone->m_name, bone->GetRTTI()->name);
    logNode(depth, bone);
    RE::NiNode* node = bone ? bone->AsNode() : 0;
    if (!node) return;

    for (auto& child : node->GetChildren()) {
        if (depth < maxDepth || maxDepth < 0) {
            logChildren(child.get(), depth + 1, maxDepth, filter);
        }
    }
}
inline RE::NiAVObject* getplayerSkeletonRoot(bool firstPerson) {
    auto player = RE::PlayerCharacter::GetSingleton();
    return player->Get3D1(firstPerson);
}

static inline void GetPlayerNodeList() {
    logger::info("------------Player first person skeleton-------------");
    logChildren(getplayerSkeletonRoot(true), 0, 100, "");
    logger::info("------------Player third person skeleton-------------");
    logChildren(getplayerSkeletonRoot(false), 0, 100, "");
}	



static inline std::string floatToStr(float& number, int precision) 
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << number;
    std::string mystring = ss.str();
    return mystring;
}

template <typename T>
static inline std::string join(const T& v, const std::string& delim) {
    std::ostringstream s;
    for (const auto& i : v) {
        if (&i != &v[0]) {
            s << delim;
        }
        s << i;
    }
    return s.str();
}

static inline std::string removeDigits(std::string& str)
{
    std::string newStr = str;
    newStr.erase(std::remove_if(std::begin(newStr), std::end(newStr), [](auto ch) { return std::isdigit(ch); }), newStr.end());
    return newStr;
}

#pragma warning(pop)
#endif