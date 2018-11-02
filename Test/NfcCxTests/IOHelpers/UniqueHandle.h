//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include <cstddef>

// An RAII type for Win32 HANDLEs.
class UniqueHandle
{
public:
    UniqueHandle() = default;
    UniqueHandle(HANDLE handle);
    UniqueHandle(const UniqueHandle& other) = delete;
    UniqueHandle(UniqueHandle&& other);

    ~UniqueHandle();

    UniqueHandle& operator=(const UniqueHandle& other) = delete;
    UniqueHandle& operator=(UniqueHandle&& other);
    UniqueHandle& operator=(std::nullptr_t);

    void Reset();
    void Reset(HANDLE handle);
    HANDLE Release();

    HANDLE Get();

private:
    HANDLE _Handle = nullptr;
};
