// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/nacl_host/pnacl_translation_cache.h"

#include <string>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_checker.h"
#include "components/nacl/common/pnacl_types.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"

using base::IntToString;
using content::BrowserThread;

namespace {

void CloseDiskCacheEntry(disk_cache::Entry* entry) { entry->Close(); }

}  // namespace

namespace pnacl {
// These are in pnacl namespace instead of static so they can be used
// by the unit test.
const int kMaxDiskCacheSize = 1000 * 1024 * 1024;
const int kMaxMemCacheSize = 100 * 1024 * 1024;

//////////////////////////////////////////////////////////////////////
// Handle Reading/Writing to Cache.

// PnaclTranslationCacheEntry is a shim that provides storage for the
// 'key' and 'data' strings as the disk_cache is performing various async
// operations. It also tracks the open disk_cache::Entry
// and ensures that the entry is closed.
class PnaclTranslationCacheEntry
    : public base::RefCounted<PnaclTranslationCacheEntry> {
 public:
  static PnaclTranslationCacheEntry* GetReadEntry(
      base::WeakPtr<PnaclTranslationCache> cache,
      const std::string& key,
      const GetNexeCallback& callback);
  static PnaclTranslationCacheEntry* GetWriteEntry(
      base::WeakPtr<PnaclTranslationCache> cache,
      const std::string& key,
      net::DrainableIOBuffer* write_nexe,
      const CompletionCallback& callback);

  void Start();

  // Writes:                                ---
  //                                        v  |
  // Start -> Open Existing --------------> Write ---> Close
  //                          \              ^
  //                           \             /
  //                            --> Create --
  // Reads:
  // Start -> Open --------Read ----> Close
  //                       |  ^
  //                       |__|
  enum CacheStep {
    UNINITIALIZED,
    OPEN_ENTRY,
    CREATE_ENTRY,
    TRANSFER_ENTRY,
    CLOSE_ENTRY
  };

 private:
  friend class base::RefCounted<PnaclTranslationCacheEntry>;
  PnaclTranslationCacheEntry(base::WeakPtr<PnaclTranslationCache> cache,
                             const std::string& key,
                             bool is_read);
  ~PnaclTranslationCacheEntry();

  // Try to open an existing entry in the backend
  void OpenEntry();
  // Create a new entry in the backend (for writes)
  void CreateEntry();
  // Write |len| bytes to the backend, starting at |offset|
  void WriteEntry(int offset, int len);
  // Read |len| bytes from the backend, starting at |offset|
  void ReadEntry(int offset, int len);
  // If there was an error, doom the entry. Then post a task to the IO
  // thread to close (and delete) it.
  void CloseEntry(int rv);
  // Call the user callback, and signal to the cache to delete this.
  void Finish(int rv);
  // Used as the callback for all operations to the backend. Handle state
  // transitions, track bytes transferred, and call the other helper methods.
  void DispatchNext(int rv);

  base::WeakPtr<PnaclTranslationCache> cache_;
  std::string key_;
  disk_cache::Entry* entry_;
  CacheStep step_;
  bool is_read_;
  GetNexeCallback read_callback_;
  CompletionCallback write_callback_;
  scoped_refptr<net::DrainableIOBuffer> io_buf_;
  base::ThreadChecker thread_checker_;
  DISALLOW_COPY_AND_ASSIGN(PnaclTranslationCacheEntry);
};

// static
PnaclTranslationCacheEntry* PnaclTranslationCacheEntry::GetReadEntry(
    base::WeakPtr<PnaclTranslationCache> cache,
    const std::string& key,
    const GetNexeCallback& callback) {
  PnaclTranslationCacheEntry* entry(
      new PnaclTranslationCacheEntry(cache, key, true));
  entry->read_callback_ = callback;
  return entry;
}

// static
PnaclTranslationCacheEntry* PnaclTranslationCacheEntry::GetWriteEntry(
    base::WeakPtr<PnaclTranslationCache> cache,
    const std::string& key,
    net::DrainableIOBuffer* write_nexe,
    const CompletionCallback& callback) {
  PnaclTranslationCacheEntry* entry(
      new PnaclTranslationCacheEntry(cache, key, false));
  entry->io_buf_ = write_nexe;
  entry->write_callback_ = callback;
  return entry;
}

PnaclTranslationCacheEntry::PnaclTranslationCacheEntry(
    base::WeakPtr<PnaclTranslationCache> cache,
    const std::string& key,
    bool is_read)
    : cache_(cache),
      key_(key),
      entry_(NULL),
      step_(UNINITIALIZED),
      is_read_(is_read) {}

PnaclTranslationCacheEntry::~PnaclTranslationCacheEntry() {
  // Ensure we have called the user's callback
  DCHECK(read_callback_.is_null());
  DCHECK(write_callback_.is_null());
}

void PnaclTranslationCacheEntry::Start() {
  DCHECK(thread_checker_.CalledOnValidThread());
  step_ = OPEN_ENTRY;
  OpenEntry();
}

// OpenEntry, CreateEntry, WriteEntry, ReadEntry and CloseEntry are only called
// from DispatchNext, so they know that cache_ is still valid.
void PnaclTranslationCacheEntry::OpenEntry() {
  int rv = cache_->backend()
      ->OpenEntry(key_,
                  &entry_,
                  base::Bind(&PnaclTranslationCacheEntry::DispatchNext, this));
  if (rv != net::ERR_IO_PENDING)
    DispatchNext(rv);
}

void PnaclTranslationCacheEntry::CreateEntry() {
  int rv = cache_->backend()->CreateEntry(
      key_,
      &entry_,
      base::Bind(&PnaclTranslationCacheEntry::DispatchNext, this));
  if (rv != net::ERR_IO_PENDING)
    DispatchNext(rv);
}

void PnaclTranslationCacheEntry::WriteEntry(int offset, int len) {
  DCHECK(io_buf_->BytesRemaining() == len);
  int rv = entry_->WriteData(
      1,
      offset,
      io_buf_.get(),
      len,
      base::Bind(&PnaclTranslationCacheEntry::DispatchNext, this),
      false);
  if (rv != net::ERR_IO_PENDING)
    DispatchNext(rv);
}

void PnaclTranslationCacheEntry::ReadEntry(int offset, int len) {
  int rv = entry_->ReadData(
      1,
      offset,
      io_buf_.get(),
      len,
      base::Bind(&PnaclTranslationCacheEntry::DispatchNext, this));
  if (rv != net::ERR_IO_PENDING)
    DispatchNext(rv);
}

void PnaclTranslationCacheEntry::CloseEntry(int rv) {
  DCHECK(entry_);
  if (rv < 0)
    entry_->Doom();
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE, base::Bind(&CloseDiskCacheEntry, entry_));
  Finish(rv);
}

void PnaclTranslationCacheEntry::Finish(int rv) {
  if (is_read_) {
    if (!read_callback_.is_null()) {
      read_callback_.Run(rv, io_buf_);
      read_callback_.Reset();
    }
  } else {
    if (!write_callback_.is_null()) {
      write_callback_.Run(rv);
      write_callback_.Reset();
    }
  }
  cache_->OpComplete(this);
}

void PnaclTranslationCacheEntry::DispatchNext(int rv) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!cache_)
    return;

  switch (step_) {
    case UNINITIALIZED:
      LOG(ERROR) << "Unexpected step in DispatchNext";
      break;

    case OPEN_ENTRY:
      if (rv == net::OK) {
        step_ = TRANSFER_ENTRY;
        if (is_read_) {
          int bytes_to_transfer = entry_->GetDataSize(1);
          io_buf_ = new net::DrainableIOBuffer(
              new net::IOBuffer(bytes_to_transfer), bytes_to_transfer);
          ReadEntry(0, bytes_to_transfer);
        } else {
          WriteEntry(0, io_buf_->size());
        }
      } else {
        if (is_read_) {
          // Just a cache miss, not necessarily an error.
          entry_ = NULL;
          Finish(rv);
        } else {
          step_ = CREATE_ENTRY;
          CreateEntry();
        }
      }
      break;

    case CREATE_ENTRY:
      if (rv == net::OK) {
        step_ = TRANSFER_ENTRY;
        WriteEntry(io_buf_->BytesConsumed(), io_buf_->BytesRemaining());
      } else {
        LOG(ERROR) << "Failed to Create a PNaCl Translation Cache Entry";
        Finish(rv);
      }
      break;

    case TRANSFER_ENTRY:
      if (rv < 0) {
        // We do not call DispatchNext directly if WriteEntry/ReadEntry returns
        // ERR_IO_PENDING, and the callback should not return that value either.
        LOG(ERROR)
            << "Failed to complete write to PNaCl Translation Cache Entry: "
            << rv;
        step_ = CLOSE_ENTRY;
        CloseEntry(rv);
        break;
      } else if (rv > 0) {
        io_buf_->DidConsume(rv);
        if (io_buf_->BytesRemaining() > 0) {
          is_read_
              ? ReadEntry(io_buf_->BytesConsumed(), io_buf_->BytesRemaining())
              : WriteEntry(io_buf_->BytesConsumed(), io_buf_->BytesRemaining());
          break;
        }
      }
      // rv == 0 or we fell through (i.e. we have transferred all the bytes)
      step_ = CLOSE_ENTRY;
      DCHECK(io_buf_->BytesConsumed() == io_buf_->size());
      if (is_read_)
        io_buf_->SetOffset(0);
      CloseEntry(0);
      break;

    case CLOSE_ENTRY:
      step_ = UNINITIALIZED;
      break;
  }
}

//////////////////////////////////////////////////////////////////////
void PnaclTranslationCache::OpComplete(PnaclTranslationCacheEntry* entry) {
  open_entries_.erase(entry);
}

//////////////////////////////////////////////////////////////////////
// Construction and cache backend initialization
PnaclTranslationCache::PnaclTranslationCache() : in_memory_(false) {}

PnaclTranslationCache::~PnaclTranslationCache() {}

int PnaclTranslationCache::InitWithDiskBackend(
    const base::FilePath& cache_dir,
    int cache_size,
    const CompletionCallback& callback) {
  return Init(net::DISK_CACHE, cache_dir, cache_size, callback);
}

int PnaclTranslationCache::InitWithMemBackend(
    int cache_size,
    const CompletionCallback& callback) {
  return Init(net::MEMORY_CACHE, base::FilePath(), cache_size, callback);
}

int PnaclTranslationCache::Init(net::CacheType cache_type,
                                const base::FilePath& cache_dir,
                                int cache_size,
                                const CompletionCallback& callback) {
  int rv = disk_cache::CreateCacheBackend(
      cache_type,
      net::CACHE_BACKEND_DEFAULT,
      cache_dir,
      cache_size,
      true /* force_initialize */,
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::CACHE).get(),
      NULL, /* dummy net log */
      &disk_cache_,
      base::Bind(&PnaclTranslationCache::OnCreateBackendComplete, AsWeakPtr()));
  init_callback_ = callback;
  if (rv != net::ERR_IO_PENDING) {
    OnCreateBackendComplete(rv);
  }
  return rv;
}

void PnaclTranslationCache::OnCreateBackendComplete(int rv) {
  // Invoke our client's callback function.
  if (!init_callback_.is_null()) {
    init_callback_.Run(rv);
    init_callback_.Reset();
  }
}

//////////////////////////////////////////////////////////////////////
// High-level API

void PnaclTranslationCache::StoreNexe(const std::string& key,
                                      net::DrainableIOBuffer* nexe_data) {
  StoreNexe(key, nexe_data, CompletionCallback());
}

void PnaclTranslationCache::StoreNexe(const std::string& key,
                                      net::DrainableIOBuffer* nexe_data,
                                      const CompletionCallback& callback) {
  PnaclTranslationCacheEntry* entry = PnaclTranslationCacheEntry::GetWriteEntry(
      AsWeakPtr(), key, nexe_data, callback);
  open_entries_[entry] = entry;
  entry->Start();
}

void PnaclTranslationCache::GetNexe(const std::string& key,
                                    const GetNexeCallback& callback) {
  PnaclTranslationCacheEntry* entry =
      PnaclTranslationCacheEntry::GetReadEntry(AsWeakPtr(), key, callback);
  open_entries_[entry] = entry;
  entry->Start();
}

int PnaclTranslationCache::InitCache(const base::FilePath& cache_directory,
                                     bool in_memory,
                                     const CompletionCallback& callback) {
  int rv;
  in_memory_ = in_memory;
  if (in_memory_) {
    rv = InitWithMemBackend(kMaxMemCacheSize, callback);
  } else {
    rv = InitWithDiskBackend(cache_directory, kMaxDiskCacheSize, callback);
  }

  return rv;
}

int PnaclTranslationCache::Size() {
  if (!disk_cache_)
    return -1;
  return disk_cache_->GetEntryCount();
}

// static
std::string PnaclTranslationCache::GetKey(const nacl::PnaclCacheInfo& info) {
  if (!info.pexe_url.is_valid() || info.abi_version < 0 || info.opt_level < 0)
    return std::string();
  std::string retval("ABI:");
  retval += IntToString(info.abi_version) + ";" +
      "opt:" + IntToString(info.opt_level) + ";" +
      "URL:";
  // Filter the username, password, and ref components from the URL
  GURL::Replacements replacements;
  replacements.ClearUsername();
  replacements.ClearPassword();
  replacements.ClearRef();
  GURL key_url(info.pexe_url.ReplaceComponents(replacements));
  retval += key_url.spec() + ";";
  // You would think that there is already code to format base::Time values
  // somewhere, but I haven't found it yet. In any case, doing it ourselves
  // here means we can keep the format stable.
  base::Time::Exploded exploded;
  info.last_modified.UTCExplode(&exploded);
  if (info.last_modified.is_null() || !exploded.HasValidValues()) {
    memset(&exploded, 0, sizeof(exploded));
  }
  retval += "modified:" + IntToString(exploded.year) + ":" +
      IntToString(exploded.month) + ":" +
      IntToString(exploded.day_of_month) + ":" +
      IntToString(exploded.hour) + ":" + IntToString(exploded.minute) + ":" +
      IntToString(exploded.second) + ":" +
      IntToString(exploded.millisecond) + ":UTC;";
  retval += "etag:" + info.etag;
  return retval;
}
}  // namespace pnacl
