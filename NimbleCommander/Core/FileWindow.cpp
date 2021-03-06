// Copyright (C) 2013-2018 Michael Kazakov. Subject to GNU General Public License version 3.
#include "FileWindow.h"

bool FileWindow::FileOpened() const
{
    return m_Window.get() != nullptr;
}

int FileWindow::OpenFile(const std::shared_ptr<VFSFile> &_file, int _window_size)
{
    if(!_file->IsOpened())
        return VFSError::InvalidCall;
    
    if(_file->GetReadParadigm() == VFSFile::ReadParadigm::NoRead)
        return VFSError::InvalidCall;

    m_File = _file;
    m_WindowSize = std::min(m_File->Size(), (ssize_t)_window_size);
    m_Window.reset(new uint8_t[m_WindowSize]);
    m_WindowPos = 0;
    
    if(m_File->GetReadParadigm() == VFSFile::ReadParadigm::Random)
    {
        int ret = ReadFileWindowRandomPart(0, m_WindowSize);
        if(ret < 0)
            return ret;
    }
    else
    {
        int ret = ReadFileWindowSeqPart(0, m_WindowSize);
        if(ret < 0)
            return ret;
    }
    
    return VFSError::Ok;
}

int FileWindow::CloseFile()
{
    m_File.reset();
    m_Window.reset();
    m_WindowPos = -1;
    m_WindowSize = -1;
    return VFSError::Ok;
}

size_t FileWindow::FileSize() const
{
    assert(FileOpened());
    return m_File->Size();
}

void *FileWindow::Window() const
{
    assert(FileOpened());
    return m_Window.get();
}

size_t FileWindow::WindowSize() const
{
    assert(FileOpened());
    return m_WindowSize;
}

size_t FileWindow::WindowPos() const
{
    assert(FileOpened());
    return m_WindowPos;
}

int FileWindow::ReadFileWindowRandomPart(size_t _offset, size_t _len)
{
    if(_len == 0)
        return VFSError::Ok;
    
    if(_offset + _len > m_WindowSize)
        return VFSError::InvalidCall;
    
    ssize_t readret = m_File->ReadAt(m_WindowPos + _offset, (unsigned char*)m_Window.get() + _offset, _len);
    if(readret < 0)
        return (int)readret;

    if(readret == 0)
        return VFSError::UnexpectedEOF;
    
    if((size_t)readret < _len) // whatif readret is 0 (EOF) - we may fall into recursion here
        return ReadFileWindowRandomPart(_offset + readret, _len - readret);
    
    return VFSError::Ok;
}

int FileWindow::ReadFileWindowSeqPart(size_t _offset, size_t _len)
{
    if(_len == 0)
        return VFSError::Ok;
    
    if(_offset + _len > m_WindowSize)
        return VFSError::InvalidCall;

    ssize_t readret = m_File->Read((unsigned char*)m_Window.get() + _offset, _len);
    if(readret < 0)
        return (int)readret;
    
    if(readret == 0)
        return VFSError::UnexpectedEOF;
    
    if((size_t)readret < _len)
        return ReadFileWindowSeqPart(_offset + readret, _len - readret);
    
    return VFSError::Ok;
}

int FileWindow::MoveWindow(size_t _offset)
{
    if(!FileOpened())
        return VFSError::InvalidCall;
    
    if(_offset == m_WindowPos)
        return VFSError::Ok;
    
    if(_offset + m_WindowSize > (size_t)m_File->Size())
        return VFSError::InvalidCall;
    
    if(m_File->GetReadParadigm() == VFSFile::ReadParadigm::Random)
    {
        // check for overlapping window movements
        if((_offset >= m_WindowPos && _offset <= m_WindowPos + m_WindowSize) ||
           (_offset + m_WindowSize >= m_WindowPos && _offset <= m_WindowPos) )
        {
            // read only unknown data
            if(_offset >= m_WindowPos && _offset <= m_WindowPos + m_WindowSize)
            {
                memmove(m_Window.get(),
                        (const unsigned char*)m_Window.get() + _offset - m_WindowPos,
                        m_WindowSize - (_offset - m_WindowPos)
                        );
                size_t off = m_WindowSize - (_offset - m_WindowPos);
                size_t len = _offset - m_WindowPos;
                m_WindowPos = _offset;
                return ReadFileWindowRandomPart(off, len);
            }
            else
            {
                memmove( (unsigned char*)m_Window.get() + m_WindowSize - (_offset + m_WindowSize - m_WindowPos),
                        m_Window.get(),
                        _offset + m_WindowSize - m_WindowPos
                        );
                size_t off = 0;
                size_t len = m_WindowPos - _offset;
                m_WindowPos = _offset;
                return ReadFileWindowRandomPart(off, len);
            }
        }
        else
        {
            // no overlapping - just move and read all window
            m_WindowPos = _offset;
            return ReadFileWindowRandomPart(0, m_WindowSize);
        }
    }
    else if(m_File->GetReadParadigm() == VFSFile::ReadParadigm::Seek)
    {
        // TODO: not efficient implementation, update me
        ssize_t ret = m_File->Seek(_offset, VFSFile::Seek_Set);
        if(ret < 0)
            return (int)ret;
        
        m_WindowPos = _offset;
        return ReadFileWindowSeqPart(0, m_WindowSize);
    }
    else if(m_File->GetReadParadigm() == VFSFile::ReadParadigm::Sequential)
    {
        // check possible for variants
        if(_offset >= m_WindowPos && _offset <= m_WindowPos + m_WindowSize)
        { // overlapping
            memmove(m_Window.get(),
                    (const unsigned char*)m_Window.get() + _offset - m_WindowPos,
                    m_WindowSize - (_offset - m_WindowPos)
                    );
            size_t off = m_WindowSize - (_offset - m_WindowPos);
            size_t len = _offset - m_WindowPos;
            m_WindowPos = _offset;
            int ret = ReadFileWindowSeqPart(off, len);
            if(ret == 0) assert(ssize_t(m_WindowPos + m_WindowSize) ==  m_File->Pos());
            return ret;
        }
        else if(_offset >= m_WindowPos)
        { // need to move forward
            assert(m_File->Pos() < ssize_t(_offset));
            size_t to_skip = _offset - m_File->Pos();
            
            int ret = (int)m_File->Skip(to_skip);
            if(ret < 0)
                return ret;
            
            m_WindowPos = _offset;
            ret = ReadFileWindowSeqPart(0, m_WindowSize);
            return ret;
        }
        else // invalid case - moving back was requested
            return VFSError::InvalidCall;
    }
    
    return VFSError::InvalidCall;
}

const VFSFilePtr& FileWindow::File() const
{
    return m_File;
}
