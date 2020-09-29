#pragma once

#include <unordered_map>
#include <type_traits>
#include <functional>
#include <filesystem>
#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <sstream>
#include <fstream>
#include <compare>
#include <utility>
#include <chrono>
#include <random>
#include <memory>
#include <vector>
#include <string>
#include <queue>
#include <cmath>
#include <list>
#include <map>
#include <new>

#ifndef __cpp_lib_concepts
#define __cpp_lib_concepts
#endif
#include <concepts>

#include <nlohmann/json.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>


typedef std::uint8_t UInt8;
typedef std::uint16_t UInt16;
typedef std::uint32_t UInt32;
typedef std::uint64_t UInt64;

typedef std::int8_t Int8;
typedef std::int16_t Int16;
typedef std::int32_t Int32;
typedef std::int64_t Int64;

typedef std::byte Byte;
typedef std::size_t Size;
typedef std::size_t Offset;

typedef std::string String;

typedef std::filesystem::path Path;

using sf::Vector2f;
using sf::Vector2i;
using sf::Vector2u;

using sf::IntRect;

using sf::Color;

using Json = nlohmann::json;

template<typename _Fty>
using Function = std::function<_Fty>;

namespace filesystem = std::filesystem;

inline Path operator"" _p(const char* str, Size size) { return { String{ str, size } }; }

namespace utils
{
	template<typename _Ty = void>
	inline _Ty* malloc(Size size) { reinterpret_cast<_Ty*>(::operator new(size)); }

	inline void free(void* ptr) { ::operator delete(ptr); }


	template<typename _Ty, typename... _Args>
	inline _Ty& construct(_Ty& object, _Args&&... args)
	{
		return new (&object) _Ty{ std::forward<_Args>(args)... }, object;
	}

	template<typename _Ty>
	inline _Ty& copy(_Ty& dst, const _Ty& src) { return construct<_Ty, const _Ty&>(dst, src); }

	template<typename _Ty>
	inline _Ty& move(_Ty& dst, _Ty&& src) { return construct<_Ty, _Ty&&>(dst, std::move(src)); }

	template<typename _Ty>
	inline _Ty& destroy(_Ty& object) { return object.~_Ty(), object; }


	template<typename _ValueTy, typename _MinTy, typename _MaxTy>
	constexpr _ValueTy clamp(_ValueTy value, _MinTy min, _MaxTy max)
	{
		if constexpr (std::same_as< _MinTy, _ValueTy>)
		{
			if constexpr (std::same_as< _MaxTy, _ValueTy>)
				return std::max(std::min(max, value), min);
			else return std::max(std::min(static_cast<_ValueTy>(max), value), min);
		}
		else
		{
			if constexpr (std::same_as< _MaxTy, _ValueTy>)
				return std::max(std::min(max, value), static_cast<_ValueTy>(min));
			else return std::max(std::min(static_cast<_ValueTy>(max), value), static_cast<_ValueTy>(min));
		}
	}

	template<Size _BufSize = 8192>
	void stream_copy(std::ostream& dst, std::istream& src, Size byte_count = 0)
	{
		bool limit = byte_count > 0;
		char buffer[_BufSize];
		while (src && dst && (!limit || byte_count > 0))
		{
			src.read(buffer, std::min(_BufSize, byte_count));
			const std::streamsize count = src.gcount();
			if (limit)
				byte_count = static_cast<Size>(count) > byte_count ? 0 : byte_count - static_cast<Size>(count);
			dst.write(buffer, count);
		}
	}

	inline Int64 system_time()
	{
		return std::chrono::system_clock::now().time_since_epoch().count();
	}


	template<typename _DstTy, typename _SrcTy>
	inline sf::Vector2<_DstTy> vector_cast(const sf::Vector2<_SrcTy>& v)
	{
		return { static_cast<_DstTy>(v.x), static_cast<_DstTy>(v.y) };
	}


	template<typename _Base, typename _Derived>
	concept BaseOf = std::is_base_of<_Base, _Derived>::value;

	template<typename _Ty>
	concept Hashable = requires(_Ty value) {
		{ std::hash<_Ty>{}(value) } -> std::convertible_to<std::size_t>;
	};

	template<typename _Base, typename _Ty>
	concept SameOrDerived = utils::BaseOf<_Base, _Ty> || std::same_as<_Base, _Ty>;
}



namespace utils::json
{
	class JsonException : std::exception
	{
	public:
		inline JsonException(const char* msg = "") : std::exception{ msg } {}
		inline JsonException(const String& msg) : std::exception{ msg.c_str() } {}
	};

	class JsonSerializable
	{
	public:
		virtual Json serialize() const = 0;
		virtual void deserialize(const Json& json) = 0;
	};

	template<typename _Ty>
	concept JsonSerializableOnly = utils::BaseOf<JsonSerializable, _Ty>;

	Json read(std::istream& input);
	Json read(const Path& path);
	Json read(const String& path);

	void write(std::ostream& output, const Json& json);
	void write(const Path& path, const Json& json);
	void write(const String& path, const Json& json);

	inline Json extract(const JsonSerializable& js) { return js.serialize(); }

	inline void inject(JsonSerializable& js, const Json& json) { js.deserialize(json); }

	inline void read(std::istream& input, JsonSerializable& js) { js.deserialize(read(input)); }
	inline void read(const Path& path, JsonSerializable& js) { js.deserialize(read(path)); }
	inline void read(const String& path, JsonSerializable& js) { js.deserialize(read(path)); }

	inline void write(std::ostream& output, const JsonSerializable& js) { write(output, js.serialize()); }
	inline void write(const Path& path, const JsonSerializable& js) { write(path, js.serialize()); }
	inline void write(const String& path, const JsonSerializable& js) { write(path, js.serialize()); }

	inline bool has(const Json& json, const String& name) { return json.find(name) != json.end(); }
	inline bool has(const Json& json, const char* name) { return json.find(name) != json.end(); }

	template<typename _Ty>
	const _Ty opt(const Json& json, const String& name, const _Ty& default_value)
	{
		auto it = json.find(name);
		return it == json.end() ? default_value : it.value().get<_Ty>();
	}

	template<typename _Ty>
	bool opt(const Json& json, const String& name, _Ty& dst)
	{
		auto it = json.find(name);
		if (it != json.end())
			return dst = it.value().get<_Ty>(), true;
		return false;
	}
}

std::ostream& operator<< (std::ostream& left, const utils::json::JsonSerializable& right);
std::istream& operator>> (std::istream& left, utils::json::JsonSerializable& right);

template<utils::json::JsonSerializableOnly _Ty>
_Ty& operator<< (_Ty& left, const Json& right) { return left.deserialize(right), left; }

template<utils::json::JsonSerializableOnly _Ty>
_Ty& operator>> (_Ty& left, Json& right) { return right = left.serialize(), left; }




namespace resource
{
	class Folder
	{
	private:
		Path _path;

	public:
		Folder() = default;
		Folder(const Folder&) = default;
		Folder(Folder&&) noexcept = default;
		~Folder() = default;

		Folder& operator= (const Folder&) = default;
		Folder& operator= (Folder&&) noexcept = default;

		bool operator== (const Folder&) const = default;
		auto operator<=> (const Folder&) const = default;

		Folder(const Path& path);
		Folder(const Folder& parent, const Path& path);

		bool openInput(const String& filename, std::ifstream& input) const;
		bool openInput(const Path& path, std::ifstream& input) const;
		bool openInput(const String& filename, const Function<void(std::istream&)>& action) const;
		bool openInput(const Path& path, const Function<void(std::istream&)>& action) const;

		bool openOutput(const String& filename, std::ofstream& output) const;
		bool openOutput(const Path& path, std::ofstream& output) const;
		bool openOutput(const String& filename, const Function<void(std::ostream&)>& action) const;
		bool openOutput(const Path& path, const Function<void(std::ostream&)>& action) const;

		bool readJson(const String& filename, Json& json) const;
		bool readJson(const Path& path, Json& json) const;

		bool writeJson(const String& filename, const Json& json) const;
		bool writeJson(const Path& path, const Json& json) const;

		inline bool openInput(const char* filename, std::ifstream& input) const { return openInput(String{ filename }, input); }
		inline bool openInput(const char* filename, const Function<void(std::istream&)>& action) const { return openInput(String{ filename }, action); }

		inline bool openOutput(const char* filename, std::ofstream& output) const { return openOutput(String{ filename }, output); }
		inline bool openOutput(const char* filename, const Function<void(std::ostream&)>& action) const { return openOutput(String{ filename }, action); }

		inline bool readJson(const char* filename, Json& json) const { return readJson(String{ filename }, json); }

		inline bool writeJson(const char* filename, const Json& json) const { return writeJson(String{ filename }, json); }

		template<utils::json::JsonSerializableOnly _Ty>
		inline _Ty& readAndInject(const String& filename, _Ty& obj) const
		{
			return openInput(filename, [&obj](std::istream& in) { utils::json::read(in, obj); }), obj;
		}

		template<utils::json::JsonSerializableOnly _Ty>
		inline _Ty& readAndInject(const Path& path, _Ty& obj) const
		{
			return openInput(path, [&obj](std::istream& in) { utils::json::read(in, obj); }), obj;
		}

		template<utils::json::JsonSerializableOnly _Ty>
		inline _Ty& readAndInject(const char* filename, _Ty& obj) const
		{
			return readAndInject(String{ filename }, obj);
		}

		template<utils::json::JsonSerializableOnly _Ty>
		inline void extractAndWrite(const String& filename, _Ty& obj) const
		{
			openOutput(filename, [&obj](std::ostream& os) { utils::json::write(os, obj); });
		}

		template<utils::json::JsonSerializableOnly _Ty>
		inline void extractAndWrite(const Path& path, _Ty& obj) const
		{
			openOutput(path, [&obj](std::ostream& os) { utils::json::write(os, obj); });
		}

		template<utils::json::JsonSerializableOnly _Ty>
		inline void extractAndWrite(const char* filename, _Ty& obj) const
		{
			extractAndWrite(String{ filename }, obj);
		}

		inline Path pathOf(const String& filename) const { return _path / filename; }
		inline Path pathOf(const Path& path) const { return _path / path; }
		inline Path pathOf(const char* filename) const { return _path / filename; }

		inline Folder folder(const String& filename) const { return { *this, filename }; }
		inline Folder folder(const Path& path) const { return { *this, path }; }
		inline Folder folder(const char* path) const { return { *this, String{ path } }; }

		inline const Path& path() const { return _path; }

	private:
		bool _open(const String& filename, std::ifstream& stream) const;
		bool _open(const Path& path, std::ifstream& stream) const;

		bool _open(const String& filename, std::ofstream& stream) const;
		bool _open(const Path& path, std::ofstream& stream) const;
	};
}

namespace resource
{
	static const Folder root = "data"_p;
}



namespace utils
{
	template<typename _Ty>
	class LinkedList
	{
	public:
		class iterator;
		class const_iterator;

	private:
		struct Node
		{
			_Ty data;
			Node* next;
			Node* prev;

			template<typename... _Args>
			Node(_Args&&... args) :
				data{ std::forward(args)... },
				next{ nullptr },
				prev{ nullptr }
			{}

			Node(const _Ty& data) :
				data{ data },
				next{ nullptr },
				prev{ nullptr }
			{}

			Node(_Ty&& data) :
				data{ std::move(data) },
				next{ nullptr },
				prev{ nullptr }
			{}

			Node(const Node&) = default;
			Node(Node&&) noexcept = default;
			~Node() = default;

			Node& operator= (const Node&) = default;
			Node& operator= (Node&&) noexcept = default;
		};

	public:
		class iterator;
		class const_iterator;

		class iterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = _Ty;
			using pointer = _Ty*;
			using reference = _Ty&;
			using const_pointer = const _Ty*;
			using const_reference = const _Ty&;

		private:
			Node* _node;

		public:
			iterator() : _node{ nullptr } {}
			iterator(Node* node) : _node{ node } {}
			iterator(const iterator&) = default;
			iterator(iterator&&) noexcept = default;
			~iterator() = default;

			iterator& operator= (const iterator&) = default;
			iterator& operator= (iterator&&) noexcept = default;

			bool operator== (const iterator&) const = default;

			operator bool() const { return _node; }
			bool operator! () const { return !_node; }

			iterator& operator++ () { if (_node) _node = _node->next; return *this; }
			iterator operator++ (int) { auto it{ *this }; return ++(*this), it; }

			iterator operator+ (Offset offset) const
			{
				iterator it{ *this };
				if (offset > 0)
				{
					if (offset == 1)
						++it;
					else for (; it._node && offset > 0; --offset)
						it._node = it._node->next;
				}
				return it;
			}

			reference operator* () const { return _node->data; }

			pointer operator-> () const { return &_node->data; }

			friend class LinkedList;
			friend class const_iterator;
		};

		class const_iterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = _Ty;
			using pointer = _Ty*;
			using reference = _Ty&;
			using const_pointer = const _Ty*;
			using const_reference = const _Ty&;

		private:
			const Node* _node;

		public:
			const_iterator() : _node{ nullptr } {}
			const_iterator(const Node* node) : _node{ node } {}
			const_iterator(const iterator& it) : _node{ it._node } {}
			const_iterator(const const_iterator&) = default;
			const_iterator(const_iterator&&) noexcept = default;
			~const_iterator() = default;

			const_iterator& operator= (const iterator& right) { return _node = right._node, *this; };
			const_iterator& operator= (const const_iterator&) = default;
			const_iterator& operator= (const_iterator&&) noexcept = default;

			bool operator== (const const_iterator&) const = default;

			operator bool() const { return _node; }
			bool operator! () const { return !_node; }

			const_iterator& operator++ () { if (_node) _node = _node->next; return *this; }
			const_iterator operator++ (int) { auto it{ *this }; return ++(*this), it; }

			const_reference operator* () const { return _node->data; }

			const_pointer operator-> () const { return &_node->data; }

			friend class LinkedList;
			friend class iterator;
		};

		inline iterator begin() { return iterator(_head); }
		inline const_iterator begin() const { return const_iterator(_head); }
		inline const_iterator cbegin() const { return const_iterator(_head); }

		inline iterator end() { return iterator(); }
		inline const_iterator end() const { return const_iterator(); }
		inline const_iterator cend() const { return const_iterator(); }

	private:
		Node* _head = nullptr;
		Node* _tail = nullptr;
		Size _size = 0;

	private:
		void _destroy()
		{
			if (_head)
			{
				for (Node* node = _head, *next; node; node = next)
				{
					next = node->next;
					delete node;
				}
			}

			_head = _tail = nullptr;
			_size = 0;
		}
		LinkedList& _copy(const LinkedList& list, bool reset)
		{
			if (reset)
				_destroy();

			for (Node* node = list._head; node; node = node->next)
			{
				Node* newnode = new Node{ node.data };
				if (!_head)
					_head = _tail = newnode;
				else
				{
					newnode->prev = _tail;
					_tail->next = newnode;
					_tail = newnode;
				}
			}

			return _size = list._size, *this;
		}
		LinkedList& _move(LinkedList&& list, bool reset) noexcept
		{
			if (reset)
				_destroy();

			_head = list._head;
			_tail = list._tail;
			_size = list._size;
		}

		iterator _push_back(Node* node)
		{
			if (!_head)
				_head = _tail = node;
			else
			{
				node->prev = _tail;
				_tail->next = node;
				_tail = node;
			}

			return ++_size, node;
		}

		iterator _push_front(Node* node)
		{
			if (!_head)
				_head = _tail = node;
			else
			{
				node->next = _head;
				_head->prev = node;
				_head = node;
			}

			return ++_size, node;
		}

		iterator _insert(Node* node, Node* prev)
		{
			node->prev = node;
			node->next = prev->next;

			prev->next->prev = node;
			prev->next = node;

			return ++_size, node;
		}

		iterator _insert(Offset index, Node* node)
		{
			if (index == 0)
				return _push_front(node);
			if (index >= _size)
				return _push_back(node);

			Node* prev = _head;
			while (index > 0)
				prev = prev->next, --index;

			return _insert(node, prev);
		}

		iterator _erase(Node* node)
		{
			Node* next = node->next;

			if (node->next)
				node->next->prev = node->prev;
			if (node->prev)
				node->prev->next = node->next;

			if (node == _head)
				_head = node->next;
			else if (node == _tail)
				_tail = node->prev;

			delete node;
			return --_size, next;
		}

	public:
		LinkedList() = default;
		LinkedList(const LinkedList& list) : LinkedList{} { _copy(list, false); }
		LinkedList(LinkedList&& list) noexcept : LinkedList{} { _move(std::move(list), false); }
		~LinkedList() { _destroy(); }

		LinkedList& operator= (const LinkedList& right) { return _copy(right, true); }
		LinkedList& operator= (LinkedList&& right) noexcept { return _move(std::move(right), true); }

		inline bool empty() const { return !_head; }
		inline Size size() const { return _size; }

		inline operator bool() const { return _head; }
		inline bool operator! () const { return !_head; }

		template<typename... _Args>
		iterator emplace_back(_Args&&... args) { return _push_back(new Node{ std::forward(args) }); }

		template<typename... _Args>
		iterator emplace_front(_Args&&... args) { return _push_front(new Node{ std::forward(args) }); }

		template<typename... _Args>
		iterator emplace(Offset index, _Args&&... args) { return _insert(index, new Node{ std::forward(args) }); }

		template<typename... _Args>
		iterator emplace(const iterator& it, _Args&&... args)
		{
			if (!it)
				return emplace_back(std::forward(args));
			return _insert(new Node{ std::forward(args) }, it._node);
		}

		iterator push_back(const _Ty& elem) { return _push_back(new Node{ elem }); }
		iterator push_back(_Ty&& elem) { return _push_back(new Node{ std::move(elem) }); }

		iterator push_front(const _Ty& elem) { return _push_front(new Node{ elem }); }
		iterator push_front(_Ty&& elem) { return _push_front(new Node{ std::move(elem) }); }

		iterator insert(Offset index, const _Ty& elem) { return _insert(index, new Node{ elem }); }
		iterator insert(Offset index, _Ty&& elem) { return _insert(index, new Node{ std::move(elem) }); }

		iterator insert(const iterator& it, const _Ty& elem)
		{
			if (!it)
				return _push_back(new Node{ elem });
			return _insert(new Node{ elem }, it._node);
		}
		iterator insert(const iterator& it, _Ty&& elem)
		{
			if (!it)
				return _push_back(new Node{ std::move(elem) });
			return _insert(new Node{ std::move(elem) }, it._node);
		}

		iterator get_iterator(const _Ty* elem_ptr) const
		{
			for (Node* node = _head; node; node = node->next)
				if (&node->data == elem_ptr)
					return node;
			return end();
		}

		iterator get_iterator(const _Ty& elem) const
		{
			for (Node* node = _head; node; node = node->next)
				if (&node->data == &elem)
					return node;
			return end();
		}

		iterator get_iterator(Offset index) const
		{
			Node* node = _head;
			while (node && index > 0)
				node = node->next;
			return node;
		}

		iterator erase(const iterator& it)
		{
			if (it._node)
				return _erase(it._node);
			return end();
		}

		iterator erase(const iterator& from, const iterator& to)
		{
			iterator it = from;
			while (it != to)
				it = erase(it);
			return it;
		}

		inline iterator erase(Offset index) { return erase(get_iterator(index)); }

		inline iterator erase(const _Ty* elem_ptr) { return erase(get_iterator(elem_ptr)); }

		inline iterator erase(const _Ty& elem) { return erase(get_iterator(elem)); }

		inline void clear() { _destroy(); }

		void clear(const Function<void(_Ty&)>& onDestroyAction)
		{
			if (_head)
			{
				for (Node* node = _head, *next; node; node = next)
				{
					next = node->next;
					onDestroyAction(node->data);
					delete node;
				}
			}

			_head = _tail = nullptr;
			_size = 0;
		}

		inline _Ty& at(Offset index) { return *get_iterator(index); }
		inline const _Ty& at(Offset index) const { return *get_iterator(index); }

		inline _Ty& front() { return _head->data; }
		inline const _Ty& front() const { return _head->data; }

		inline _Ty& back() { return _tail->data; }
		inline const _Ty& back() const { return _tail->data; }

		inline LinkedList& operator<< (const _Ty& right) { return push_back(right), * this; }
		inline LinkedList& operator<< (_Ty&& right) { return push_back(std::move(right)), * this; }

		LinkedList& operator+= (const LinkedList& right)
		{
			for (Node* node = right._head; node; node = node->next)
				_push_back(new Node{ node->data });
			return *this;
		}

		LinkedList& operator+= (LinkedList&& right)
		{
			_tail->next = right._head;
			if (right._head)
			{
				right._head->prev = _tail;
				_tail = right._tail;
			}
			_size += right._size;

			right._head = right._tail = nullptr;
			right._size = 0;

			return *this;
		}

		LinkedList operator+ (const LinkedList& right)
		{
			LinkedList list = *this;
			return list += right;
		}

		LinkedList operator+ (LinkedList&& right)
		{
			LinkedList list = *this;
			return list += std::move(right);
		}

		inline _Ty& operator[] (Offset index) { return *get_iterator(index); }
		inline const _Ty& operator[] (Offset index) const { return *get_iterator(index); }
	};
}
