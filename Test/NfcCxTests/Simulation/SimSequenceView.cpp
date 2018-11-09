//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "SimSequenceView.h"

SimSequenceView::SimSequenceView(
    _In_reads_(stepListSize) const SimSequenceStep* list,
    _In_ size_t listSize)
    :
    _Type(Type::StepList)
{
    _Data.StepList.Data = list;
    _Data.StepList.Size = listSize;
}

SimSequenceView::SimSequenceView(_In_ const SimSequenceStep& step) :
    SimSequenceView(&step, 1)
{
}

SimSequenceView::SimSequenceView(
    _In_reads_(stepListSize) const SimSequenceView* list,
    _In_ size_t listSize)
    :
    _Type(Type::SequenceList)
{
    _Data.SequenceList.Data = list;
    _Data.SequenceList.Size = listSize;
}

SimSequenceView::Type
SimSequenceView::GetType() const
{
    return _Type;
}

const SimSequenceStep*
SimSequenceView::GetStepList() const
{
    if (_Type != Type::StepList)
    {
        return nullptr;
    }

    return _Data.StepList.Data;
}

size_t
SimSequenceView::GetStepListSize() const
{
    if (_Type != Type::StepList)
    {
        return 0;
    }

    return _Data.StepList.Size;
}

const SimSequenceView*
SimSequenceView::GetSequenceList() const
{
    if (_Type != Type::SequenceList)
    {
        return nullptr;
    }

    return _Data.SequenceList.Data;
}

size_t
SimSequenceView::GetSequenceListSize() const
{
    if (_Type != Type::SequenceList)
    {
        return 0;
    }

    return _Data.SequenceList.Size;
}
