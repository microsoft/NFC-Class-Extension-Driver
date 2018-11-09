//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "SimSequenceStep.h"

// A array view of SimSequenceStep.
// WARNING: This type does not own the memory that it references.
class SimSequenceView
{
public:
    SimSequenceView(
        _In_reads_(stepListSize) const SimSequenceStep* list,
        _In_ size_t listSize);

    template <size_t ArraySize>
    SimSequenceView(
        const SimSequenceStep (&list)[ArraySize])
        :
        SimSequenceView(list, ArraySize)
    {
    }

    const SimSequenceStep* GetList() const;
    size_t GetListSize() const;

private:
    const SimSequenceStep* _List = nullptr;
    size_t _ListSize = 0;
};
