//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "SimSequenceStep.h"

// 'SimSequenceView' is a variant type of either:
//     SimSequenceStep[] OR
//     SimSequenceView[]
//
// This type is intended to be used to compose lists of SimSequenceStep without copying memory.
//
// WARNING: This type does not own the memory that it references.
class SimSequenceView
{
public:
    enum class Type { StepList, SequenceList };

    SimSequenceView(
        _In_reads_(stepListSize) const SimSequenceStep* list,
        _In_ size_t listSize);

    template <size_t ListSize>
    SimSequenceView(
        const SimSequenceStep (&list)[ListSize])
        :
        SimSequenceView(list, ListSize)
    {
    }

    SimSequenceView(_In_ const SimSequenceStep& step);

    SimSequenceView(
        _In_reads_(stepListSize) const SimSequenceView* list,
        _In_ size_t listSize);

    template <size_t ListSize>
    SimSequenceView(
        const SimSequenceView (&list)[ListSize])
        :
        SimSequenceView(list, ListSize)
    {
    }

    Type GetType() const;
    const SimSequenceStep* GetStepList() const;
    size_t GetStepListSize() const;
    const SimSequenceView* GetSequenceList() const;
    size_t GetSequenceListSize() const;

private:
    union Data
    {
        struct
        {
            const SimSequenceStep* Data;
            size_t Size;
        } StepList;
        struct
        {
            const SimSequenceView* Data;
            size_t Size;
        } SequenceList;
    };
    Data _Data = {};
    Type _Type;
};
