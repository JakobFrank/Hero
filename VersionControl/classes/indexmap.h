// indexmap.h: Defines the indexmap class, which handles the translation of filenames to hashes (for directory support purposes)

#ifndef INDEXMAP_H
#define INDEXMAP_H
#pragma once

#include "../../PicoSHA2/picosha2.h"
#include "hero.h"

#include <string>
#include <utility>
#include <fstream>
#include <map>

class Commitmap;

class Indexmap {
public:
	using Filename = std::string;
	using Hash = std::string;

	using iterator = std::map<Filename, Hash>::iterator;
	using const_iterator = std::map<Filename, Hash>::const_iterator;

	Indexmap() {}
	Indexmap(const Indexmap& map):m_map(map.m_map) {}
	Indexmap(Indexmap&& map):m_map(std::move(map.m_map)) {}
	Indexmap(const Commitmap& map) {
		for (const auto& it : map) {
			m_map[it.second] = it.first;
		}
	}
	
	void add(const Filename& file) {
		m_map[file] = picosha2::hash256_hex_string(std::ifstream(file));
	}

	Hash getHash(const Filename& file) const {
		return m_map.at(file);
	}

	Filename getFile(const Hash& file) const {
		for (const auto& it : m_map) {
			if (it.second == file) {
				return it.first;
			}
		}
		return "";
	}

	iterator begin() {
		return m_map.begin();
	}

	const_iterator begin() const {
		return m_map.cbegin();
	}

	const_iterator cbegin() const {
		return m_map.cbegin();
	}

	iterator end() {
		return m_map.end();
	}

	const_iterator end() const {
		return m_map.cend();
	}

	const_iterator cend() const {
		return m_map.cend();
	}

	bool exists(const Filename& file) const {
		return m_map.count(file) > 0;
	}

	Hash& operator[] (const Filename& file) {
		return m_map[file];
	}

	const Hash& operator[] (const Filename& file) const {
		return m_map.at(file);
	}

	static Indexmap loadFrom(std::istream& stream) {
		Indexmap result;
		stream >> result;
		return result;
	}

	static Indexmap loadFrom(const std::string& file) {
		return loadFrom(std::ifstream(file));
	}

	static Indexmap loadFrom(const char* file) {
		return loadFrom(std::ifstream(file));
	}

	friend std::ostream& operator << (std::ostream& stream, const Indexmap& map) {
		for (const auto& it : map.m_map) {
			stream << it.first << ',' << it.second << '\n';
		}
		return stream;
	}

	friend std::istream& operator >> (std::istream& stream, Indexmap& map) {
		std::string buffer;
		std::getline(stream, buffer);
		while (stream) {
			std::string first, second;
			auto sep(buffer.find_first_of(','));
			first = buffer.substr(0, sep);
			second = buffer.substr(sep + 1);
			map.m_map[first] = second;
			std::getline(stream, buffer);
		}
		return stream;
	}
protected:
	std::map<Filename, Hash> m_map;
};

class Commitmap {
public:
	using Filename = std::string;
	using Hash = std::string;

	using iterator = std::map<Filename, Hash>::iterator;
	using const_iterator = std::map<Filename, Hash>::const_iterator;

	Commitmap() {}
	Commitmap(const Commitmap& map) :m_map(map.m_map) {}
	Commitmap(Commitmap&& map) :m_map(std::move(map.m_map)) {}
	Commitmap(const Indexmap& map) {
		for (const auto& it : map) {
			m_map[it.second] = it.first;
		}
	}

	void add(const Filename& file) {
		m_map[picosha2::hash256_hex_string(std::ifstream(file))] = file;
	}

	Hash getHash(const Filename& file) const {
		for (const auto& it : m_map) {
			if (it.second == file) {
				return it.first;
			}
		}
		return "";
	}

	Filename getFile(const Hash& file) const {
		return m_map.at(file);
	}

	iterator begin() {
		return m_map.begin();
	}

	const_iterator begin() const {
		return m_map.cbegin();
	}

	const_iterator cbegin() const {
		return m_map.cbegin();
	}

	iterator end() {
		return m_map.end();
	}

	const_iterator end() const {
		return m_map.cend();
	}

	const_iterator cend() const {
		return m_map.cend();
	}

	bool exists(const Hash& file) const {
		return m_map.count(file) > 0;
	}

	Filename& operator[] (const Hash& file) {
		return m_map[file];
	}

	const Filename& operator[] (const Hash& file) const {
		return m_map.at(file);
	}

	static Commitmap loadFrom(std::istream& stream) {
		Commitmap result;
		stream >> result;
		return result;
	}

	static Commitmap loadFrom(const std::string& file) {
		return loadFrom(std::ifstream(file));
	}

	static Commitmap loadFrom(const char* file) {
		return loadFrom(std::ifstream(file));
	}

	friend std::ostream& operator << (std::ostream& stream, const Commitmap& map) {
		for (const auto& it : map.m_map) {
			stream << it.second << ',' << it.first << '\n';
		}
		return stream;
	}

	friend std::istream& operator >> (std::istream& stream, Commitmap& map) {
		std::string buffer;
		std::getline(stream, buffer);
		while (stream) {
			std::string first, second;
			auto sep(buffer.find_first_of(','));
			first = buffer.substr(0, sep);
			second = buffer.substr(sep + 1);
			map.m_map[second] = first;
			std::getline(stream, buffer);
		}
		return stream;
	}
protected:
	std::map<Hash, Filename> m_map;
};

// A class which handles automatically loading and writing the indexmap in the chosen format
// The contained Indexmap is automatically loaded from disk when its constructed, and written to disk when the loader goes out of scope.
// T should be one of the above classes
// By default, the indexmap is loaded from the default indexmap path, but it can be loaded from alternate paths
// Cannot be copied. Move will enforce that the moved loader cannot write to disk
template <class T> class basic_indexmapLoader {
public:
	T map;

	basic_indexmapLoader(): m_location(repositoryPath(INDEXMAP_PATH)), map(T::loadFrom(m_location)) {}
	basic_indexmapLoader(const std::string& s) : m_location(s), map(T::loadFrom(m_location)) {}
	basic_indexmapLoader(basic_indexmapLoader&& il) : m_location(std::move(il.m_location)), map(std::move(il.map)) {
		il.m_location = "";
	}

	~basic_indexmapLoader() {
		write();
	}

	void write() {
		std::ofstream target(m_location, std::ios::out | std::ios::trunc);
		target << map;
		target.close();
	}
protected:
	std::string m_location;

private:
	basic_indexmapLoader(const basic_indexmapLoader&);
};

using IndexmapLoader = basic_indexmapLoader<Indexmap>;
using CommitmapLoader = basic_indexmapLoader<Commitmap>;
#endif // !INDEXMAP_H
