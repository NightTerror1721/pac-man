#include "common.h"

namespace utils::json
{
	Json read(std::istream& input)
	{
		try
		{
			Json json;
			input >> json;
			return json;
		}
		catch (const std::exception& ex) { throw JsonException{ ex.what() }; }
	}
	Json read(const Path& path)
	{
		std::fstream f{ path, std::ios::in };
		return read(f);
	}
	Json read(const String& path)
	{
		std::fstream f{ path, std::ios::in };
		return read(f);
	}

	void write(std::ostream& output, const Json& json)
	{
		try
		{
			output << json;
		}
		catch (const std::exception& ex) { throw JsonException{ ex.what() }; }
	}
	void write(const Path& path, const Json& json)
	{
		std::fstream f{ path, std::ios::out };
		write(f, json);
	}
	void write(const String& path, const Json& json)
	{
		std::fstream f{ path, std::ios::out };
		write(f, json);
	}
}

std::ostream& operator<< (std::ostream& left, const utils::json::JsonSerializable& right) { return utils::json::write(left, right), left; }
std::istream& operator>> (std::istream& left, utils::json::JsonSerializable& right) { return utils::json::read(left, right), left; }






namespace resource
{
	Folder::Folder(const Path& path) :
		_path{ path }
	{}
	Folder::Folder(const Folder& parent, const Path& path) :
		_path{ parent._path / path }
	{}

	bool Folder::_open(const String& filename, std::ifstream& stream) const
	{
		stream.open(_path / filename, std::ios::in);
		return !stream.fail();
	}
	bool Folder::_open(const Path& path, std::ifstream& stream) const
	{
		stream.open(_path / path, std::ios::in);
		return !stream.fail();
	}
	bool Folder::_open(const String& filename, std::ofstream& stream) const
	{
		stream.open(_path / filename, std::ios::out);
		return !stream.fail();
	}
	bool Folder::_open(const Path& path, std::ofstream& stream) const
	{
		stream.open(_path / path, std::ios::out);
		return !stream.fail();
	}

	bool Folder::openInput(const String& filename, std::ifstream& input) const { return _open(filename, input); }
	bool Folder::openInput(const Path& path, std::ifstream& input) const { return _open(path, input); }
	bool Folder::openInput(const String& filename, const Function<void(std::istream&)>& action) const
	{
		std::ifstream stream;
		if (_open(filename, stream))
			return action(stream), true;
		return false;
	}
	bool Folder::openInput(const Path& path, const Function<void(std::istream&)>& action) const
	{
		std::ifstream stream;
		if (_open(path, stream))
			return action(stream), true;
		return false;
	}

	bool Folder::openOutput(const String& filename, std::ofstream& output) const { return _open(filename, output); }
	bool Folder::openOutput(const Path& path, std::ofstream& output) const { return _open(path, output); }
	bool Folder::openOutput(const String& filename, const Function<void(std::ostream&)>& action) const
	{
		std::ofstream stream;
		if (_open(filename, stream))
			return action(stream), true;
		return false;
	}
	bool Folder::openOutput(const Path& path, const Function<void(std::ostream&)>& action) const
	{
		std::ofstream stream;
		if (_open(path, stream))
			return action(stream), true;
		return false;
	}

	bool Folder::readJson(const String& filename, Json& json) const { return openInput(filename, [&json](std::istream& is) { json = utils::json::read(is); }); }
	bool Folder::readJson(const Path& path, Json& json) const { return openInput(path, [&json](std::istream& is) { json = utils::json::read(is); }); }

	bool Folder::writeJson(const String& filename, const Json& json) const { return openOutput(filename, [&json](std::ostream& os) { utils::json::write(os, json); }); }
	bool Folder::writeJson(const Path& path, const Json& json) const { return openOutput(path, [&json](std::ostream& os) { utils::json::write(os, json); }); }
}
