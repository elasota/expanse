#pragma once

namespace expanse
{
	namespace cc
	{
		struct FileCoordinate
		{
			FileCoordinate();
			FileCoordinate(size_t m_fileOffset, unsigned int m_lineNumber, unsigned int m_column);

			size_t m_fileOffset;
			unsigned int m_lineNumber;
			unsigned int m_column;

			bool operator==(const FileCoordinate &other) const;
			bool operator!=(const FileCoordinate &other) const;
		};
	}
}

namespace expanse
{
	namespace cc
	{
		inline FileCoordinate::FileCoordinate()
			: m_fileOffset(0)
			, m_lineNumber(0)
			, m_column(0)
		{
		}

		inline FileCoordinate::FileCoordinate(size_t fileOffset, unsigned int lineNumber, unsigned int column)
			: m_fileOffset(fileOffset)
			, m_lineNumber(lineNumber)
			, m_column(column)
		{
		}

		inline bool FileCoordinate::operator==(const FileCoordinate &other) const
		{
			return m_fileOffset == other.m_fileOffset && m_lineNumber == other.m_lineNumber && m_column == other.m_column;
		}

		inline bool FileCoordinate::operator!=(const FileCoordinate &other) const
		{
			return !((*this) == other);
		}
	}
}
