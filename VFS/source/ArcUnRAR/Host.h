// Copyright (C) 2014-2018 Michael Kazakov. Subject to GNU General Public License version 3.
#pragma once

#include <sys/stat.h>
#include <Habanero/SerialQueue.h>
#include "../../include/VFS/Host.h"
#include "../../include/VFS/VFSFile.h"
#include <map>
#include <list>

namespace nc::vfs {

namespace unrar {
struct Entry;
struct Directory;
struct SeekCache;
}

class UnRARHost final : public Host
{
public:
    static const char *UniqueTag;
    UnRARHost(const std::string &_path);
    UnRARHost(const VFSHostPtr &_parent, const VFSConfiguration &_config);
    ~UnRARHost();
    
    virtual bool IsImmutableFS() const noexcept override;
    virtual VFSConfiguration Configuration() const override;
    static VFSMeta Meta();
    

    static bool IsRarArchive(const char *_archive_native_path);
    
    
    // core VFSHost methods
    virtual int FetchDirectoryListing(const char *_path,
                                      std::shared_ptr<VFSListing> &_target,
                                      unsigned long _flags,
                                      const VFSCancelChecker &_cancel_checker) override;
    
    virtual int IterateDirectoryListing(const char *_path,
                                        const std::function<bool(const VFSDirEnt &_dirent)> &_handler) override;
    
    virtual int StatFS(const char *_path, VFSStatFS &_stat, const VFSCancelChecker &_cancel_checker) override;
    
    virtual int Stat(const char *_path,
                     VFSStat &_st,
                     unsigned long _flags,
                     const VFSCancelChecker &_cancel_checker) override;

    virtual int CreateFile(const char* _path,
                           std::shared_ptr<VFSFile> &_target,
                           const VFSCancelChecker &_cancel_checker) override;
    
    
    virtual bool ShouldProduceThumbnails() const override;
    
    
    // internal UnRAR stuff
    
    /**
     * Return zero on not found.
     */
    uint32_t ItemUUID(const std::string& _filename) const;

    /**
     * Returns UUID of a last item in archive.
     */
    uint32_t LastItemUUID() const;

    /**
     * Return nullptr on not found.
     */
    const unrar::Entry *FindEntry(const std::string &_full_path) const;
    
    /**
     * Inserts opened rar handle into host's seek cache.
     */
    void CommitSeekCache(std::unique_ptr<unrar::SeekCache> _sc);
    
    /**
     * if there're no appropriate caches, host will try to open a new RAR handle.
     * If can't satisfy this call - zero ptr is returned.
     */
    std::unique_ptr<unrar::SeekCache> SeekCache(uint32_t _requested_item);
    
    
    std::shared_ptr<const UnRARHost> SharedPtr() const {return std::static_pointer_cast<const UnRARHost>(Host::SharedPtr());}
    std::shared_ptr<UnRARHost> SharedPtr() {return std::static_pointer_cast<UnRARHost>(Host::SharedPtr());}
    
private:
    int DoInit(); // flags will be added later
    
    int InitialReadFileList(void *_rar_handle);

    unrar::Directory *FindOrBuildDirectory(const std::string& _path_with_tr_sl);
    const unrar::Directory *FindDirectory(const std::string& _path) const;
    
    std::map<std::string, unrar::Directory> m_PathToDir; // path to dir with trailing slash -> directory contents
    uint32_t                                m_LastItemUID = 0;
    std::list<std::unique_ptr<unrar::SeekCache>>m_SeekCaches;
    dispatch_queue_t                        m_SeekCacheControl;
    uint64_t                                m_PackedItemsSize = 0;
    uint64_t                                m_UnpackedItemsSize = 0;
    bool                                    m_IsSolidArchive = false;
    struct stat                             m_ArchiveFileStat;
    VFSConfiguration                        m_Configuration;

    // TODO: int m_FD for exclusive lock?
};

}
