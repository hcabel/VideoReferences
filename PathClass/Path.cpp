

#include "Path.h"
#include <cstring>

#define cout std::cout
#define endl std::endl

static int ALLOCATED_COUNT = 0;
static size_t ALLOCATED_SIZE = 0;
static int DEALLOCATED_COUNT = 0;

static bool ACCUMULATE = true;
static bool PRINT = true;

// Override the default new operator to track memory allocation
void* operator new(size_t size)
{
	void* ptr = malloc(size);
	if (ACCUMULATE)
	{
		if (PRINT)
			cout << "Allocated: " << size << endl;
		ALLOCATED_COUNT++;
		ALLOCATED_SIZE += size;
	}
	return (ptr);
}

// Override the default delete operator to track memory allocation
void operator delete(void* ptr)
{
	if (ACCUMULATE)
	{
		if (PRINT)
			cout << "Deallocated" << endl;
		DEALLOCATED_COUNT++;
	}
	free(ptr);
}

namespace std {
	ostream& operator<< (ostream& os, wchar_t wc)
	{
		if(unsigned(wc) < 256) // or another upper bound
			return os << (unsigned char)wc;
	}

	ostream& operator<< (ostream& os, const wchar_t* wc)
	{
		while (*wc)
			os << *wc++;
		return (os);
	}
}

// Allo me to print string without null terminator
template<typename CharType>
class TextSegment
{
public:
	TextSegment(const CharType* data, size_t size)
		: m_Data(data),
		m_Size(size)
	{}

public:
	const CharType* m_Data;
	size_t m_Size;
};

template<typename CharType>
std::ostream& operator<<(std::ostream& os, const TextSegment<CharType>& segment)
{
	for (size_t index = 0; index < segment.m_Size; index++)
		os << segment.m_Data[index];
	return (os);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// REALY START HERE
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

namespace PathCore
{
	bool IsAValidFolderNameChar(const TCHAR character)
	{
		for (uint8_t invalidCharIndex = 0; invalidCharIndex < InvalidFolderNameChars.size(); invalidCharIndex++)
		{
			if (character == InvalidFolderNameChars[invalidCharIndex])
				return (false);
		}
		return (true);
	}

	SegmentSize IsFolderSegmentValid(const TCHAR* segmentStart)
	{
		if (segmentStart == nullptr)
			return (0); // Invalid segment

		SegmentSize segmentIndex = 0;
		while (segmentStart[segmentIndex] != NULL && IsSeparator(segmentStart[segmentIndex]) == false)
		{
			if (IsAValidFolderNameChar(segmentStart[segmentIndex]) == false)
				return (0); // The segment contains an invalid character
			segmentIndex++;
		}
		return (segmentIndex);
	}

	SegmentSize IsFolderSegmentValid(const TCHAR* segmentStart, PathSize size)
	{
		if (size == 0 || size > PATH_MAX_FOLDER_NAME_LENGTH || segmentStart == nullptr)
			return (0); // Invalid size or segment

		SegmentSize segmentIndex = 0;
		while (segmentIndex < size && segmentStart[segmentIndex] != NULL && IsSeparator(segmentStart[segmentIndex]) == false)
		{
			if (IsAValidFolderNameChar(segmentStart[segmentIndex]) == false)
				return (0); // The segment contains an invalid character
			segmentIndex++;
		}
		return (segmentIndex);
	}

	bool IsDiskNameValid(const TCHAR* segmentStart)
	{
		assert(segmentStart && "Invalid segmentStart pointer");

		// First char should be an upper case letter
		if (segmentStart[0] < TEXT('A') || segmentStart[0] > TEXT('Z'))
			return (false);
		// Second char should always be a ':'
		if (segmentStart[1] != TEXT(':'))
			return (false);
		// Third char should always be a separator
		if (IsSeparator(segmentStart[2]) == false)
			return (false);
		return (true);
	}

	///////////////////////////////////////////////////////////////////////////
	// CONST SEGMENT ITERATOR
	///////////////////////////////////////////////////////////////////////////

	ConstSegmentIterator::ConstSegmentIterator(const IPath* path, PathSize index)
		: m_Path(const_cast<IPath*>(path)), // breaking the const to allow the non const iterator to inherit from this path class
		m_Pos(0)
	{
		*this = index;
	}

	ConstSegmentIterator::ConstSegmentIterator(const ConstSegmentIterator& other)
		: m_Path(other.m_Path),
		m_Pos(other.m_Pos)
	{}


	ConstSegmentIterator::ConstSegmentIterator(ConstSegmentIterator&& other) noexcept
		: m_Path(std::move(other.m_Path)),
		m_Pos(std::move(other.m_Pos))
	{
		other.m_Pos = std::numeric_limits<PathSize>::max();
		other.m_Path = nullptr;
	}

	ConstSegmentIterator::DataPtr ConstSegmentIterator::operator*() const
	{
		return (m_Path->Data() + m_Pos);
	}
	ConstSegmentIterator::DataPtr ConstSegmentIterator::operator->() const
	{
		return (m_Path->Data() + m_Pos);
	}
	ConstSegmentIterator::DataPtr ConstSegmentIterator::operator[](PathSize offset) const
	{
		return (m_Path->Data() + m_Pos + offset);
	}

	/* ARITHMETIC OPERATORS */

	ConstSegmentIterator& ConstSegmentIterator::operator++()
	{
		DataPtr data = m_Path->Data();

		// Move to the end of the segment
		while (m_Pos < m_Path->Size() && IsSeparator(data[m_Pos]) == false)
			m_Pos++;

		// If not at the end of the path, add 1 to skip the separator
		if (m_Pos < m_Path->Size())
			m_Pos++;
		return (*this);
	}
	ConstSegmentIterator ConstSegmentIterator::operator+(const PathSize offset)
	{
		ConstSegmentIterator copy(*this);

		// Move offset segment forward
		for (PathSize index = 0; index < offset; index++)
		{
			++copy;

			// Stop if you reach the end of the path
			if (m_Pos == m_Path->Size())
				break;
		}
		return (std::move(copy));
	}
	ConstSegmentIterator& ConstSegmentIterator::operator+=(PathSize offset)
	{
		// Move offset segment forward
		for (PathSize index = 0; index < offset; index++)
		{
			++*this;

			// Stop if you reach the end of the path
			if (m_Pos == m_Path->Size())
				break;
		}
		return (*this);
	}

	ConstSegmentIterator& ConstSegmentIterator::operator--()
	{
		DataPtr data = m_Path->Data();

		// move back until you find the start of this segment
		while (m_Pos > 0 && IsSeparator(data[m_Pos - 1]) == false)
			m_Pos--;
		return (*this);
	}
	ConstSegmentIterator ConstSegmentIterator::operator-(PathSize offset)
	{
		ConstSegmentIterator copy(*this);

		// move offset segment backward
		for (PathSize index = 0; index < offset; index++)
		{
			--copy;

			// Stop if you reach the start of the path
			if (m_Pos == 0)
				break;
		}

		return (std::move(copy));
	}
	ConstSegmentIterator& ConstSegmentIterator::operator-=(PathSize offset)
	{
		for (PathSize index = 0; index < offset; index++)
		{
			--*this;
			
			// Stop if you reach the start of the path
			if (m_Pos == 0)
				break;
		}
		return (*this);
	}

	/* ASSIGMENT OPERATORS */

	ConstSegmentIterator& ConstSegmentIterator::operator=(const PathSize index)
	{
		if (index >= m_Path->Size())
		{
			m_Pos = m_Path->Size();
			return (*this); // End segment iterator value
		}
		else if (index < 0)
		{
			m_Pos = 0;
			return (*this); // Begin segment
		}

		// Set pos whatever the index is, then move to the start of the segment
		m_Pos = index;
		--*this;

		return (*this);
	}

	ConstSegmentIterator& ConstSegmentIterator::operator=(const ConstSegmentIterator& other)
	{
		m_Path = other.m_Path;
		m_Pos = other.m_Pos;
		return (*this);
	}
	ConstSegmentIterator& ConstSegmentIterator::operator=(ConstSegmentIterator&& other) noexcept
	{
		m_Path = std::move(other.m_Path);
		m_Pos = std::move(other.m_Pos);

		// Invalidate other
		other.m_Pos = std::numeric_limits<PathSize>::max();
		other.m_Path = nullptr;

		return (*this);
	}

	/* COMPARISON OPERATORS */

	ConstSegmentIterator::operator bool() const
	{
		// Note < not <= so we can use iterator like so: while (it) { ... }
		return (m_Path != nullptr && m_Pos < m_Path->Size()); 
	}
	bool ConstSegmentIterator::operator==(const ConstSegmentIterator& other) const
	{
		return (other.BelongTo(m_Path) && m_Pos == other.m_Pos);
	}
	bool ConstSegmentIterator::operator!=(const ConstSegmentIterator& other) const // TODO: check is I can just do `!operator bool()`
	{
		return (other.BelongTo(m_Path) == false || m_Pos != other.m_Pos);
	}
	bool ConstSegmentIterator::operator<(const ConstSegmentIterator& other) const
	{
		return (other.BelongTo(m_Path) && m_Pos < other.m_Pos);
	}
	bool ConstSegmentIterator::operator>(const ConstSegmentIterator& other) const
	{
		return (other.BelongTo(m_Path) && m_Pos > other.m_Pos);
	}
	bool ConstSegmentIterator::operator<=(const ConstSegmentIterator& other) const
	{
		return (other.BelongTo(m_Path) && m_Pos <= other.m_Pos);
	}
	bool ConstSegmentIterator::operator>=(const ConstSegmentIterator& other) const
	{
		return (other.BelongTo(m_Path) && m_Pos >= other.m_Pos);
	}
	bool ConstSegmentIterator::operator==(PathSize index) const
	{
		return (m_Pos == index);
	}
	bool ConstSegmentIterator::operator!=(PathSize index) const // TODO: check is I can just do `!operator ==()`
	{
		return (m_Pos != index);
	}
	bool ConstSegmentIterator::operator<(PathSize index) const
	{
		return (m_Pos < index);
	}
	bool ConstSegmentIterator::operator>=(PathSize index) const // TODO: check is I can just do `!operator <()`
	{
		return (m_Pos >= index);
	}
	bool ConstSegmentIterator::operator>(PathSize index) const
	{
		return (m_Pos > index);
	}
	bool ConstSegmentIterator::operator<=(PathSize index) const // TODO: check is I can just do `!operator >()`
	{
		return (m_Pos <= index);
	}

	SegmentSize ConstSegmentIterator::Size() const
	{
		if (m_Pos == m_Path->Size())
			return (0); // End iterator

		DataPtr data = m_Path->Data();

		SegmentSize size = 0;
		for (PathSize index = m_Pos; index < m_Path->Size(); index++)
		{
			if (IsSeparator(data[index]) || index == m_Path->Size())
				break;
			size++;
		}
		return (size);
	}
	PathSize ConstSegmentIterator::Pos() const { return (m_Pos); }
	bool ConstSegmentIterator::BelongTo(const IPath* path) const { return (m_Path == path); }

	///////////////////////////////////////////////////////////////////////////
	// SEGMENT ITERATOR
	///////////////////////////////////////////////////////////////////////////
	
	void SegmentIterator::Rename(const TCHAR* newName)
	{
		SegmentSize newNameSize = 0;
		while (newName[newNameSize] != NULL)
			newNameSize++;

		SegmentSize segmentIndex = 0;
		PathSize bufferIndex = m_Pos;

		PathBase<OsSeparator>* basePathPtr = static_cast<PathBase<OsSeparator>*>(this->m_Path);

		// Copy as much as you can, until you reach the end of the segment
		while (segmentIndex < newNameSize && IsSeparator(basePathPtr->Data()[bufferIndex]) == false)
		{
			basePathPtr->m_Path[bufferIndex] = newName[segmentIndex];
			segmentIndex++;
			bufferIndex++;
		}

		// If you didn't finish copying because you reached the end of the segment, make space for the rest of the new name
		if (segmentIndex < newNameSize && IsSeparator(basePathPtr->Data()[bufferIndex]))
		{
			// Move the rest of the path to the right
			SegmentSize offset = newNameSize - segmentIndex;
			for (PathSize index = basePathPtr->Size(); index >= bufferIndex; index--)
				basePathPtr->m_Path[index + offset] = basePathPtr->Data()[index];

			// Copy the rest of the new name
			for (SegmentSize index = segmentIndex; index < newNameSize; index++)
			{
				basePathPtr->m_Path[bufferIndex] = newName[index];
				bufferIndex++;
			}

			// Update size to reflect new change
			basePathPtr->m_Size += offset;
		}
		// If you finished copying and you didn't reach the end of the segment, we need to bring the rest of the path closer
		else if (segmentIndex == newNameSize && IsSeparator(basePathPtr->Data()[bufferIndex]) == false)
		{
			PathSize segmentEndPos = bufferIndex;
			while (IsSeparator(basePathPtr->Data()[segmentEndPos]) == false)
				segmentEndPos++;

			// Move the rest of the path to the left
			SegmentSize offset = segmentEndPos - bufferIndex;
			for (PathSize index = segmentEndPos; index < basePathPtr->Size(); index++)
				basePathPtr->m_Path[index - offset] = basePathPtr->Data()[index];

			// Update size to reflect new change
			basePathPtr->m_Size -= offset;

			// Terminate the path to the new size
			basePathPtr->m_Path[basePathPtr->Size()] = NULL;
		}
	}

	void SegmentIterator::Swap(const SegmentIterator& withSegment)
	{

	}

	///////////////////////////////////////////////////////////////////////////
	// PATH BASE
	///////////////////////////////////////////////////////////////////////////

	template<TCHAR Separator>
	PathBase<Separator>::PathBase(const IPath* parent, const TCHAR* rawPath)
		: m_Path(),
		m_Size(0)
	{
		if (parent)
		{
			// Copy the parent path, w/o checking because we trust the parent
			while (m_Size < parent->Size())
			{
				m_Path[m_Size] = parent->Data()[m_Size];
				m_Size++;
			}
		}
		Append(rawPath);
	}
	template<TCHAR Separator>
	PathBase<Separator>::PathBase(ConstSegmentIterator fromSegment, const ConstSegmentIterator& toSegment)
		: m_Path(),
		m_Size(0)
	{
		assert(fromSegment.BelongTo(this) == false); // 'fromSegment' is not from this path
		assert(toSegment <= fromSegment); // 'fromSegment' is after 'toSegment' (implicitly check if they are both targeting the same path)

		// Copy each segment from fromSegment (included) to toSegment (not included)
		while (fromSegment != toSegment)
		{
			Append(fromSegment);
			++fromSegment;
		}
	}

	template<TCHAR Separator>
	PathBase<Separator>& PathBase<Separator>::operator=(const PathBase<Separator>& other)
	{
		m_Path = other.m_Path;
		m_Size = other.m_Size;
		return (*this);
	}
	template<TCHAR Separator>
	PathBase<Separator>& PathBase<Separator>::operator=(PathBase<Separator>&& other) noexcept
	{
		m_Path = std::move(other.m_Path);
		m_Size = std::move(other.m_Size);
		return (*this);
	}

	template<TCHAR Separator>
	PathBase<Separator>& PathBase<Separator>::operator+=(const TCHAR* rawPath)
	{
		Append(rawPath);
		return (*this);
	}
	template<TCHAR Separator>
	PathBase<Separator>& PathBase<Separator>::operator+=(const ConstSegmentIterator& segment)
	{
		Append(segment);
		return (*this);
	}

	template<TCHAR Separator>
	PathBase<Separator> PathBase<Separator>::operator+(const TCHAR* rawPath) const
	{
		return (PathBase(this, rawPath));
	}
	template<TCHAR Separator>
	PathBase<Separator> PathBase<Separator>::operator+(const ConstSegmentIterator& segment) const
	{
		PathBase pathCopy(this);
		pathCopy.Append(segment);

		return (pathCopy);
	}

	template<TCHAR Separator>
	void PathBase<Separator>::Append(const TCHAR* rawPath)
	{
		PathSize rawPathIndex = 0;

		if (m_Size == 0)
		{
			assert(IsDiskNameValid(rawPath) && "Invalid Disk segment (TODO add rawPath to this message)");

			// Copy the disk segment
			while (m_Size < PATH_DISK_NAME_LENGTH)
			{
				m_Path[m_Size] = rawPath[m_Size];
				m_Size++;
			}
			rawPathIndex = PATH_DISK_NAME_LENGTH;
		}
		else
			assert(m_Size + PATH_SEPARATOR_LENGTH + PATH_MIN_FOLDER_NAME_LENGTH <= MAX_PATH_LENGTH && "Unable to append segment \'TODO\' path to long!");
		
		// Add separator
		if (IsSeparator(rawPath[rawPathIndex]))
			rawPathIndex++;

		// Copy rawPath folder segments
		while (rawPath[rawPathIndex] != NULL)
		{
			// Add separator
			m_Path[m_Size] = Separator;
			m_Size++;

			// Copy folder segment name
			while (rawPath[rawPathIndex] != NULL)
			{
				assert(IsAValidFolderNameChar(rawPath[rawPathIndex]) && "Invalid folder name char found!");

				m_Path[m_Size] = rawPath[rawPathIndex];
				m_Size++;
				rawPathIndex++;

				assert(m_Size <= MAX_PATH_LENGTH && "Path is too long");

				if (IsSeparator(rawPath[rawPathIndex]))
				{
					rawPathIndex++;
					break;
				}
			}
		}
		m_Path[m_Size] = NULL;
	}

	template<TCHAR Separator>
	void PathCore::PathBase<Separator>::Append(const ConstSegmentIterator& segment)
	{
		// Check if their is enough space to append the segment
		assert(m_Size + PATH_SEPARATOR_LENGTH + PATH_MIN_FOLDER_NAME_LENGTH <= MAX_PATH_LENGTH && "Cannot append segment, path will be too long");

		// If were appending after already existing data, add a separator
		if (m_Size != 0)
		{
			m_Path[m_Size] = Separator;
			m_Size++;
		}

		// Copy data char by char
		SegmentSize segmentSize = segment.Size();
		for (PathSize index = 0; index < segmentSize; index++)
		{
			m_Path[m_Size] = *segment[index];
			m_Size++;
			assert(m_Size <= MAX_PATH_LENGTH && "Cannot append segment, path is too long");
		}

		// Add null terminator
		m_Path[m_Size] = NULL;
	}
	template<TCHAR Separator>
	void PathBase<Separator>::Append(ConstSegmentIterator fromSegment, const ConstSegmentIterator& toSegment)
	{
		assert(fromSegment <= toSegment); // 'from' is after 'to' (also check if they are from the same path)

		while (fromSegment != toSegment)
		{
			Append(fromSegment);
			++fromSegment;
		}
	}

	template<TCHAR Separator>
	void PathBase<Separator>::Shrink(const ConstSegmentIterator& toSegment)
	{
		assert(toSegment.BelongTo(this)); // 'toSegment' is not from this path

		if (toSegment == IPath::EndSegment())
			return; // Nothing to do
		if (toSegment == IPath::BeginSegment())
		{
			Clear();
			return;
		}

		// Terminal the path, by replacing the separator before the toSegment segment by a NULL terminator
		m_Path[toSegment.Pos() - 1] = NULL;
		m_Size = toSegment.Pos() - 1;
	};

	template<TCHAR Separator>
	void PathBase<Separator>::Insert(const ConstSegmentIterator& whereSegment, ConstSegmentIterator fromSegment, const ConstSegmentIterator& toSegment)
	{
		assert(whereSegment.BelongTo(this)); // 'whereSegment' is not from this path
		assert(fromSegment <= toSegment); // 'fromSegment' is after 'toSegment' (also check if they are both from the same path)

		if (whereSegment == IPath::EndSegment())
		{
			Append(fromSegment, toSegment);
			return;
		}
		else if (fromSegment == toSegment)
			return; // Nothing to do

		assert(whereSegment.Pos() + (toSegment.Pos() - fromSegment.Pos()) + PATH_SEPARATOR_LENGTH <= MAX_PATH_LENGTH && "Unable to insert 'TODO', path will be too long");

		// Use a new buffer to compute the new path, because from and to segments can be from this path
		PathSize newPathBufferIndex = 0;
		PathBuffer newPathBuffer;

		// Copy until whereSegment position
		while (newPathBufferIndex < whereSegment.Pos() - 1)
		{
			newPathBuffer[newPathBufferIndex] = m_Path[newPathBufferIndex];
			newPathBufferIndex++;
		}

		// Copy the data from the insert segment
		for (; fromSegment != toSegment; ++fromSegment)
		{
			newPathBuffer[newPathBufferIndex] = Separator;
			newPathBufferIndex++;

			SegmentSize segmentSize = fromSegment.Size();
			for (SegmentSize index = 0; index < segmentSize; index++)
			{
				newPathBuffer[newPathBufferIndex] = *fromSegment[index];
				newPathBufferIndex++;
			}
		}

		// Add separator
		newPathBuffer[newPathBufferIndex] = Separator;
		newPathBufferIndex++;

		// Copy the rest of m_Path from where to the end
		for (PathSize index = whereSegment.Pos(); index < m_Size; index++)
		{
			newPathBuffer[newPathBufferIndex] = m_Path[index];
			newPathBufferIndex++;
		}

		// Copy newPathBuffer to m_Path
		for (PathSize index = 0; index < newPathBufferIndex; index++)
			m_Path[index] = newPathBuffer[index];

		// add null terminator
		m_Path[newPathBufferIndex] = NULL;

		m_Size = newPathBufferIndex;
	}

	template<TCHAR Separator>
	void PathBase<Separator>::Clear()
	{
		m_Size = 0;
		m_Path.fill(NULL);
	}

	///////////////////////////////////////////////////////////////////////////
	// STATIC PATH BASE
	///////////////////////////////////////////////////////////////////////////

	template<TCHAR Separator>
	StaticPathBase::StaticPathBase(const IPath& path)
		: m_Path(path.Size() + NULL_TERMINATOR_LENGTH)
	{
		for (PathSize index = 0; index < path.Size(); index++)
			m_Path[index] = path.Data()[index];
	}

#ifdef DEBUG
	template<TCHAR Separator>
	StaticPathBase::StaticPathBase(const TCHAR* rawPath)
		: StaticPathBase(PathBase<Separator>(rawPath))
	{}
#else
	template<TCHAR Separator>
	StaticPathBase::StaticPathBase(const TCHAR* rawPath)
		: m_Path(std::wcslen(rawPath) + NULL_TERMINATOR_LENGTH)
	{
		for (PathSize index = 0; rawPath[index] != NULL; index++)
			m_Path[index] = rawPath[index];
	}
#endif

#ifdef DEBUG
	template<TCHAR Separator>
	StaticPathBase::StaticPathBase(const IPath& parent, const TCHAR* rawPath)
		: StaticPathBase(PathBase<Separator>(parent, rawPath))
	{}
#else
	template<TCHAR Separator>
	StaticPathBase::StaticPathBase(const IPath& parent, const TCHAR* rawPath)
		: m_Path()
	{
		PathSize rawPathSize = 0;
		while (rawPath[rawPathSize] != NULL)
			rawPathSize++;

		PathSize parentSize = parent.Size();
		m_Path.reserve(parentSize + rawPathSize + PATH_SEPARATOR_LENGTH);

		// Copy parent path
		PathSize index = 0;
		while (index < parentSize)
		{
			m_Path[index] = parent.Data()[index];
			index++;
		}

		// Add separator
		m_Path[index] = Separator;
		index++;

		// Copy rawPath
		for (PathSize rawPathIndex = 0; rawPathIndex < rawPathSize; rawPathIndex++)
		{
			m_Path[index] = rawPath[rawPathIndex];
			index++;
		}

	}
#endif

	std::ostream& operator<<(std::ostream& os, const IPath& path)
	{
		os << path.Data();
		return (os);
	}
}

void DoWork()
{
	Path path = Path(TEXT("C:/Users/FolderName1/FolderName2/FolderName3/FolderName4"));
	StaticPath staticPath = StaticPath(path);

	cout << "path: \"" << path.Data() << "\" sizeof: " << sizeof(path) << endl;
	cout << "staticPath: \"" << staticPath.Data() << "\" sizeof: " << sizeof(staticPath) << endl << endl;

	cout << "path: \"" << path << "\"" << endl;
	path += TEXT("\\FolderName5\\FolderName6");
	cout << "+= path: \"" << path << "\"" << endl << endl;

	path = TEXT("C:/Users");
	cout << "= path: \"" << path << "\"" << endl;
	path = path / TEXT("FolderName1") / TEXT("FolderName2");
	cout << "/ path: \"" << path << "\"" << endl;
	path /= TEXT("FolderName3");
	cout << "/= path: \"" << path << "\"" << endl << endl;

	PathCore::SegmentIterator newEnd = path.BeginSegment() + 2;
	path.Shrink(newEnd);
	cout << "EndSeg ? " << (newEnd == path.EndSegment() ? "true" : "false") << endl;
	cout << "= path: \"" << path << "\"" << endl;
	path = path / TEXT("FolderName1") / TEXT("FolderName2");
	cout << "/ path: \"" << path << "\"" << endl;
	path /= TEXT("FolderName3");
	cout << "/= path: \"" << path << "\"" << endl << endl;

	StaticPath staticPath2(path);

	for (auto it = staticPath2.BeginSegment(); it != staticPath2.EndSegment(); ++it)
	{
		cout << "\tSegment: \"" << TextSegment<TCHAR>(*it, it.Size()) << "\"" << endl;
	}
	path.Shrink(path.BeginSegment() + 2);
	cout << "Shrink path: \"" << path << "\"" << endl;
	path.Insert(path.BeginSegment() + 1, staticPath2.BeginSegment() + 2, staticPath2.EndSegment());
	cout << "Insert middle path: \"" << path << "\"" << endl;
	path.Shrink(path.BeginSegment() + 1);
	cout << "Shrink path: \"" << path << "\"" << endl;
	path.Insert(path.BeginSegment() + 1, staticPath2.BeginSegment() + 1, staticPath2.BeginSegment() + 4);
	cout << "Insert begin path: \"" << path << "\"" << endl;

	auto it = path.BeginSegment() + 2;
	it.Rename(TEXT("IveJustBeenRename"));
	cout << "Rename Bigger \"" << path << "\"" << endl;
	it.Rename(TEXT("YepWorkingFine"));
	cout << "Rename smaller \"" << path << "\"" << endl;
	it.Rename(TEXT(".AFolderHidden"));
	cout << "Rename same \"" << path << "\"" << endl;
}

int main()
{
	DoWork();

	cout << "Total memory allocated: " << ALLOCATED_SIZE << " in " << ALLOCATED_COUNT << endl;
	cout << "Memory deallocated count " << DEALLOCATED_COUNT << endl;
}
