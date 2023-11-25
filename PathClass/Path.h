#pragma once

#include <iostream>

#include <assert.h>
#include <vector>
#include <array>

///////////////////////////////////////////////////////////////////////////////
//  Redefining useful macros, I dont want to include the whole stdlib.h
///////////////////////////////////////////////////////////////////////////////

#define USE_WIDE_CHAR 1

#define PLATFORM_WINDOWS
// #define PLATFORM_LINUX
// #define PLATFORM_MACOS

/** Used as a placeholder */
#define PATH_SEPARATOR_LENGTH 1
/** Used as a placeholder */
#define NULL_TERMINATOR_LENGTH 1

#ifdef PLATFORM_WINDOWS

/**
 * @brief The maximum length for a full path
 * @example "C:/FolderName1/Foldername2/Image.png" <= MAX_PATH_LENGTH characters
 */
#define MAX_PATH_LENGTH 259

/**
 * @brief The maximum length for a disk name
 * @note the trailing separator is not counted
 * @example "C:" <= PATH_DISK_NAME_LENGTH characters
 */
#define PATH_DISK_NAME_LENGTH 2

/**
 * @brief The maximum length for a directory path
 * @details "12" because of backward compatibility with 8.3 file names.
 *          More details here: https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#maximum-path-length-limitation
 *          In the macro calculation we don't add a PATH_TRAILING_NULL_LENGTH because it is already included in MAX_PATH_LENGTH
 * @example "C:/FolderName1/Foldername2" <= MAX_DIRECTORY_PATH_LENGTH characters
 */
#define MAX_DIRECTORY_PATH_LENGTH (MAX_PATH_LENGTH - 12)

/**
 * @brief The maximum length for a folder name
 * @example "FolderName1" <= PATH_MAX_FOLDER_NAME_LENGTH characters
 */
#define PATH_MAX_FOLDER_NAME_LENGTH (MAX_DIRECTORY_PATH_LENGTH - (PATH_DISK_NAME_LENGTH + PATH_SEPARATOR_LENGTH))
#define PATH_MIN_FOLDER_NAME_LENGTH 1

/**
 * @brief The maximum length for a file name
 * @details I added 1 but I dont know what it stand for, I just know that it is needed
 * @example "Image" <= PATH_MAX_FILE_NAME_LENGTH characters
 */
#define PATH_MAX_FILE_NAME_LENGTH (MAX_PATH_LENGTH - (PATH_DISK_NAME_LENGTH + PATH_SEPARATOR_LENGTH + 1))
#define PATH_MIN_FILE_NAME_LENGTH 1

/**
 * @brief The maximum length for an file extension
 * @note (Including a leading dot)
 * @example ".png" <= PATH_MAX_EXT_NAME_LENGTH characters
 */
#define PATH_MAX_EXT_NAME_LENGTH (PATH_MAX_FILE_NAME_LENGTH - 1)

#elif defined(PLATFORM_LINUX)

#define MAX_PATH_LENGTH 4096

#define PATH_DISK_NAME_LENGTH 2
// TODO: Verify this
#define MAX_DIRECTORY_PATH_LENGTH MAX_PATH_LENGTH

#define PATH_MAX_FOLDER_NAME_LENGTH 255
#define PATH_MIN_FOLDER_NAME_LENGTH 1

#define PATH_MAX_FILE_NAME_LENGTH 255
#define PATH_MIN_FILE_NAME_LENGTH 1

#define PATH_MAX_EXT_NAME_LENGTH (PATH_MAX_FILE_NAME_LENGTH - 1)

#elif defined(PLATFORM_MACOS)

#define MAX_PATH_LENGTH 1024

// TODO: Verify all the value bellow (I just assumed they were the same as Linux)

#define PATH_DISK_NAME_LENGTH 2
#define MAX_DIRECTORY_PATH_LENGTH MAX_PATH_LENGTH

#define PATH_MAX_FOLDER_NAME_LENGTH 255
#define PATH_MIN_FOLDER_NAME_LENGTH 1

#define PATH_MAX_FILE_NAME_LENGTH 255
#define PATH_MIN_FILE_NAME_LENGTH 1

#define PATH_MAX_EXT_NAME_LENGTH (PATH_MAX_FILE_NAME_LENGTH - 1)

#else
# error "Path limitations not defined for this platform"
#endif

#if USE_WIDE_CHAR == 1
# define TEXT(x) L##x
using TCHAR = wchar_t;
#else
# define TEXT(x) x
using TCHAR = char
#endif

namespace PathCore
{
	using SegmentSize = uint8_t;
	using PathSize = uint16_t;

#ifdef PLATFORM_LINUX
	// TODO: Some kind of SSO would be nice
	class LinuxPathBuffer : public std::vector<TCHAR>
	{
	public:
		LinuxPathBuffer()
		{
			this->resize(MAX_PATH_LENGTH + NULL_TERMINATOR_LENGTH, NULL);
		}

		void fill(const TCHAR character)
		{
			this->assign(MAX_PATH_LENGTH + NULL_TERMINATOR_LENGTH, character);
		}

	};

	using PathBuffer = LinuxPathBuffer;
#else
	using PathBuffer = std::array<TCHAR, MAX_PATH_LENGTH + 1>;
#endif

	constexpr TCHAR WindowsSeparator = TEXT('\\');
	constexpr TCHAR UnixSeparator = TEXT('/');
	constexpr TCHAR LinuxSeparator = UnixSeparator;
	constexpr TCHAR MacOsSeparator = UnixSeparator;
#if defined(PLATFORM_WINDOWS)
	constexpr TCHAR OsSeparator = WindowsSeparator;
#else
	constexpr TCHAR OsSeparator = UnixSeparator;
#endif

#if defined(PLATFORM_WINDOWS)
	constexpr std::array<TCHAR, 9> InvalidFolderNameChars = {
		WindowsSeparator, TEXT('/'), TEXT(':'), TEXT('*'), TEXT('?'), TEXT('"'), TEXT('<'), TEXT('>'), TEXT('|')
	};
#elif defined(PLATFORM_LINUX)
	constexpr std::array<TCHAR, 1> InvalidFolderNameChars = { UnixSeparator };
#elif defined(PLATFORM_MACOS)
	constexpr std::array<TCHAR, 2> InvalidFolderNameChars = { UnixSeparator, TEXT(':') };
#else
# error "Invalid folder name chars not defined for this platform"
#endif

	constexpr bool IsSeparator(const TCHAR c) { return (c == WindowsSeparator || c == UnixSeparator); }
	bool IsAValidFolderNameChar(const TCHAR character);
	SegmentSize IsFolderSegmentValid(const TCHAR* segmentStart);
	SegmentSize IsFolderSegmentValid(const TCHAR* segmentStart, PathSize size);
	bool IsDiskNameValid(const TCHAR* segmentStart);

	template<TCHAR c>
	class IsSeparatorClass
	{
		static_assert(IsSeparator(c), "C is not a separator");
	};

	// Forward declaration for iterators
	class IPath;
	template<TCHAR>
	class PathBase;

	/**
	 * Represent a read only segment in a path.
	 * eg: "C:/User" either "C:" or "User" are segments
	 *
	 * Segment is assumed to be valid (based on the current platform rules)
	 */
	class ConstSegmentIterator
	{
	public:
		using DataPtr = const TCHAR*;

	protected:
		ConstSegmentIterator(const IPath* path, PathSize index);

	public:
		ConstSegmentIterator(const ConstSegmentIterator& other);
		ConstSegmentIterator(ConstSegmentIterator&& other) noexcept;

	public:
		/* GET OPERATORS */

		DataPtr operator*() const;
		DataPtr operator->() const;
		DataPtr operator[](PathSize offset) const;

		/* ARITHMETIC OPERATORS */

		ConstSegmentIterator& operator++();
		ConstSegmentIterator operator+(PathSize offset);
		ConstSegmentIterator& operator+=(PathSize offset);

		ConstSegmentIterator& operator--();
		ConstSegmentIterator operator-(PathSize offset);
		ConstSegmentIterator& operator-=(PathSize offset);

		template<typename T>
		ConstSegmentIterator operator+(const T offset) { return (operator+(static_cast<PathSize>(offset))); }
		template<typename T>
		ConstSegmentIterator operator+=(const T offset) { return (operator+=(static_cast<PathSize>(offset))); }
		template<typename T>
		ConstSegmentIterator operator-(const T offset) { return (operator-(static_cast<PathSize>(offset))); }
		template<typename T>
		ConstSegmentIterator operator-=(const T offset) { return (operator-=(static_cast<PathSize>(offset))); }

		/* ASSIGMENT OPERATORS */

		ConstSegmentIterator& operator=(const PathSize index);
		ConstSegmentIterator& operator=(const ConstSegmentIterator& other);
		ConstSegmentIterator& operator=(ConstSegmentIterator&& other) noexcept;

		/* COMPARISON OPERATORS */

		operator bool() const;
		bool operator==(const ConstSegmentIterator& other) const;
		bool operator!=(const ConstSegmentIterator& other) const; // TODO: check is I can just do `!operator bool()`
		bool operator<(const ConstSegmentIterator& other) const;
		bool operator>(const ConstSegmentIterator& other) const;
		bool operator<=(const ConstSegmentIterator& other) const;
		bool operator>=(const ConstSegmentIterator& other) const;
		bool operator==(PathSize index) const;
		bool operator!=(PathSize index) const; // TODO: check is I can just do `!operator ==()`
		bool operator<(PathSize index) const;
		bool operator>=(PathSize index) const; // TODO: check is I can just do `!operator <()`
		bool operator>(PathSize index) const;
		bool operator<=(PathSize index) const; // TODO: check is I can just do `!operator >()`

	public:
		SegmentSize Size() const;
		PathSize Pos() const;
		bool BelongTo(const IPath* path) const;

	protected:
		/* A pointer to the path this iterator belong to */
		IPath* m_Path;
		/* The position of the iterator in the path */
		PathSize m_Pos;

		friend class IPath;
	};

	class SegmentIterator : public ConstSegmentIterator
	{
	public:
		using DataPtr = TCHAR*;

	private:
		SegmentIterator(const ConstSegmentIterator& other)
			: ConstSegmentIterator(other)
		{}

	protected:
		SegmentIterator(const IPath* path, PathSize index)
			: ConstSegmentIterator(path, index)
		{}

	public:
		SegmentIterator(const SegmentIterator& other)
			: ConstSegmentIterator(other)
		{}
		SegmentIterator(SegmentIterator&& other) noexcept
			: ConstSegmentIterator(std::move(other))
		{}

	public:
		/* GET OPERATORS */

		DataPtr operator*() const { return (const_cast<DataPtr>(ConstSegmentIterator::operator*())); }
		DataPtr operator->() const { return (const_cast<DataPtr>(ConstSegmentIterator::operator->())); }

		/* ARITHMETIC OPERATORS */

		SegmentIterator& operator++()
		{
			ConstSegmentIterator::operator++();
			return (*this);
		}
		SegmentIterator operator+(PathSize offset) { return (SegmentIterator(ConstSegmentIterator::operator+(offset))); }
		SegmentIterator& operator+=(PathSize offset)
		{
			ConstSegmentIterator::operator+=(offset);
			return (*this);
		}

		SegmentIterator& operator--()
		{
			ConstSegmentIterator::operator--();
			return (*this);
		}
		SegmentIterator operator-(PathSize offset) { return (SegmentIterator(ConstSegmentIterator::operator-(offset))); }
		SegmentIterator& operator-=(PathSize offset)
		{
			ConstSegmentIterator::operator-=(offset);
			return (*this);
		}

		template<typename T>
		SegmentIterator operator+(const T offset) { return (operator+(static_cast<PathSize>(offset))); }
		template<typename T>
		SegmentIterator operator+=(const T offset) { return (operator+=(static_cast<PathSize>(offset))); }
		template<typename T>
		SegmentIterator operator-(const T offset) { return (operator-(static_cast<PathSize>(offset))); }
		template<typename T>
		SegmentIterator operator-=(const T offset) { return (operator-=(static_cast<PathSize>(offset))); }

		/* ASSIGMENT OPERATORS */

		SegmentIterator& operator=(const PathSize index)
		{
			ConstSegmentIterator::operator=(index);
			return (*this);
		}
		SegmentIterator& operator=(const SegmentIterator& other)
		{
			ConstSegmentIterator::operator=(other);
			return (*this);
		}
		SegmentIterator& operator=(SegmentIterator&& other) noexcept
		{
			ConstSegmentIterator::operator=(std::move(other));
			return (*this);
		}

		/* COMPARISON OPERATORS */

		operator bool() const { return (ConstSegmentIterator::operator bool()); }
		bool operator==(const SegmentIterator& other) const { return (ConstSegmentIterator::operator==(other)); }
		bool operator!=(const SegmentIterator& other) const { return (ConstSegmentIterator::operator!=(other)); }
		bool operator<(const SegmentIterator& other) const { return (ConstSegmentIterator::operator<(other)); }
		bool operator>(const SegmentIterator& other) const { return (ConstSegmentIterator::operator>(other)); }
		bool operator<=(const SegmentIterator& other) const { return (ConstSegmentIterator::operator<=(other)); }
		bool operator>=(const SegmentIterator& other) const { return (ConstSegmentIterator::operator>=(other)); }
		bool operator==(PathSize index) const { return (ConstSegmentIterator::operator==(index)); }
		bool operator!=(PathSize index) const { return (ConstSegmentIterator::operator!=(index)); }
		bool operator<(PathSize index) const { return (ConstSegmentIterator::operator<(index)); }
		bool operator>=(PathSize index) const { return (ConstSegmentIterator::operator>=(index)); }
		bool operator>(PathSize index) const { return (ConstSegmentIterator::operator>(index)); }
		bool operator<=(PathSize index) const { return (ConstSegmentIterator::operator<=(index)); }
	
	public:
		void Rename(const TCHAR* rawPata);
		void Swap(const SegmentIterator& withSegment);

		template<TCHAR>
		friend class PathBase;
	};

	/**
	 * IPath is the base class for all the Path.
	 *
	 * Represent an array of TCHAR that is divided into segments by separators,
	 * and that can be iterated over using PathSegmentIterator.
	 */
	class IPath
	{
	public:
		TCHAR operator[](PathSize index) const { return (Data()[index]); }

	public:
		ConstSegmentIterator BeginSegment() const { return (ConstSegmentIterator(this, 0)); }
		ConstSegmentIterator EndSegment() const { return (ConstSegmentIterator(this, Size())); }

	public:
		virtual const TCHAR* Data() const = 0;
		virtual PathSize Size() const = 0;
	};

	/**
	 * The base class for all the Path that are meant to be manipulated.
	 *
	 * \tparam Separator The separator char that will be used to split the path into segments ('/' or '\\')
	 *
	 * This class will:
	 * - Create a buffer of MAX_PATH_SIZE. (To allow fast manipulation)
	 * - Check whether or not your path is valid.
	 *
	 * IMPORTANT: Once your path is finished please convert it to a StaticPath for long term storage.
	 */
	template<TCHAR Separator = OsSeparator>
	class PathBase : public IPath, private IsSeparatorClass<Separator>
	{
	public:
		PathBase()
		{
			Clear();
		}
		PathBase(const PathBase<Separator>& other)
			: m_Path(other.m_Path),
			m_Size(other.m_Size)
		{}
		PathBase(PathBase<Separator>&& other)
			: m_Path(std::move(other.m_Path)),
			m_Size(std::move(other.m_Size))
		{}
		PathBase(const TCHAR* rawPath)
			: PathBase(nullptr, rawPath)
		{}
		PathBase(const IPath* parent, const TCHAR* rawPath);
		PathBase(ConstSegmentIterator fromSegment, const ConstSegmentIterator& toSegment);

	public:
		operator bool() const { return (IsValid()); }

		PathBase<Separator>& operator=(const PathBase<Separator>& other);
		PathBase<Separator>& operator=(PathBase<Separator>&& other) noexcept;

		PathBase<Separator>& operator+=(const TCHAR* rawPath);
		PathBase<Separator>& operator+=(const ConstSegmentIterator& segment);
		PathBase<Separator>& operator/=(const TCHAR* rawPath) { return (operator+=(rawPath)); }
		PathBase<Separator>& operator/=(const ConstSegmentIterator& segment) { return (operator+=(segment)); }

		PathBase<Separator> operator+(const TCHAR* rawPath) const;
		PathBase<Separator> operator+(const ConstSegmentIterator& segment) const;
		PathBase<Separator> operator/(const TCHAR* rawPath) const { return (operator+(rawPath)); }
		PathBase<Separator> operator/(const ConstSegmentIterator& segment) const { return (operator+(segment)); }

	public:
		//~ Begin IPath Interface
		const TCHAR* Data() const override { return (m_Path.data()); }
		PathSize Size() const override { return (m_Size); }
		//~ End IPath Interface

	public:
		SegmentIterator BeginSegment() { return (SegmentIterator(this, 0)); }
		SegmentIterator EndSegment() { return (SegmentIterator(this, Size())); }

	public:
		bool IsValid() const { return (m_Size > 0); }

		void Append(const TCHAR* path);
		void Append(const ConstSegmentIterator& segment);
		void Append(ConstSegmentIterator fromSegment, const ConstSegmentIterator& toSegment);

		void Shrink(const ConstSegmentIterator& toSegment);
		void Shrink(ConstSegmentIterator& toSegment)
		{
			Shrink(const_cast<const ConstSegmentIterator&>(toSegment));
			toSegment = EndSegment();
		}
		void Insert(const ConstSegmentIterator& whereSegment, ConstSegmentIterator fromSegment, const ConstSegmentIterator& toSegment);
		void Clear();

	private:
		PathBuffer m_Path;
		PathSize m_Size;

		friend class StaticPathBase;
		friend SegmentIterator;
	};

	/**
	 * The base class for all the Path that are meant for long term storage.
	 *
	 * This class will:
	 * - Allocate the exact amount of memory to store the path. (on the HEAP)
	 * - Check if the path is valid. (DEBUG only)
	 */
	class StaticPathBase : public IPath
	{
	public:
		StaticPathBase() = default;
		template<TCHAR Separator = OsSeparator>
		StaticPathBase(const IPath& path);

		template<TCHAR Separator = OsSeparator>
		StaticPathBase(const TCHAR* rawPath);

		template<TCHAR Separator = OsSeparator>
		StaticPathBase(const IPath& parent, const TCHAR* rawPath);

	public:
		//~ Begin IPath Interface
		const TCHAR* Data() const override { return (m_Path.data()); }
		PathSize Size() const override { return (m_Path.size() - NULL_TERMINATOR_LENGTH); }
		//~ End IPath Interface

	private:
		std::vector<TCHAR> m_Path;
	};
}

using Path = PathCore::PathBase<PathCore::OsSeparator>;
using UnixPath = PathCore::PathBase<PathCore::UnixSeparator>;
using WindowsPath = PathCore::PathBase<PathCore::WindowsSeparator>;
using LinuxPath = UnixPath;
using MacPath = UnixPath;

using ConstPathSegmentIterator = PathCore::ConstSegmentIterator;
using PathSegmentIterator = PathCore::SegmentIterator;

using StaticPath = PathCore::StaticPathBase;
