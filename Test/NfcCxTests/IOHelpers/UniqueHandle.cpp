//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "UniqueHandle.h"

UniqueHandle::UniqueHandle(HANDLE handle) :
    _Handle(handle)
{
}

UniqueHandle::UniqueHandle(UniqueHandle&& other) :
    _Handle(other._Handle)
{
    other._Handle = nullptr;
}

UniqueHandle::~UniqueHandle()
{
    Reset();
}

UniqueHandle& UniqueHandle::operator=(UniqueHandle&& other)
{
    if (this != std::addressof(other))
    {
        Reset(other.Release());
    }

    return *this;
}

UniqueHandle& UniqueHandle::operator=(std::nullptr_t)
{
    Reset();
    return *this;
}

void UniqueHandle::Reset()
{
    if (_Handle && _Handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_Handle);
    }

    _Handle = nullptr;
}

void UniqueHandle::Reset(HANDLE handle)
{
    if (handle != _Handle)
    {
        Reset();
        _Handle = handle;
    }
}

HANDLE UniqueHandle::Release()
{
    HANDLE handle = _Handle;
    _Handle = nullptr;
    return handle;
}

HANDLE UniqueHandle::Get()
{
    return _Handle;
}
