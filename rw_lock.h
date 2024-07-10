#pragma once
#ifndef _RWLockImpl_Header
#define _RWLockImpl_Header
#include <Windows.h>
#include <assert.h>
#include <iostream>
#include <process.h>

using namespace std;

class RWLockImpl {
  protected:
    RWLockImpl();
    ~RWLockImpl();
    void ReadLockImpl();
    bool TryReadLockImpl();
    void WriteLockImpl();
    bool TryWriteLockImpl();
    void UnlockImpl();

  private:
    void AddWriter();
    void RemoveWriter();
    DWORD TryReadLockOnce();

    HANDLE m_mutex;
    HANDLE m_readEvent;
    HANDLE m_writeEvent;
    unsigned m_readers;
    unsigned m_writersWaiting;
    unsigned m_writers;
};

class Lock : private RWLockImpl {
  public:
    Lock(){};
    ~Lock(){};
    void ReadLock();
    bool TryReadLock();
    void WriteLock();
    bool TryWriteLock();
    void Unlock();

  private:
    Lock(const Lock &);
};

inline void Lock::ReadLock() {
    ReadLockImpl();
}

inline bool Lock::TryReadLock() {
    return TryReadLockImpl();
}

inline void Lock::WriteLock() {
    WriteLockImpl();
}

inline bool Lock::TryWriteLock() {
    return TryWriteLockImpl();
}

inline void Lock::Unlock() {
    UnlockImpl();
}
#endif
