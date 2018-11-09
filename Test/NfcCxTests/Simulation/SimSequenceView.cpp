//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include "SimSequenceView.h"

SimSequenceView::SimSequenceView(
    _In_reads_(stepListSize) const SimSequenceStep* list,
    _In_ size_t listSize)
    :
    _List(list),
    _ListSize(listSize)
{
}

const SimSequenceStep*
SimSequenceView::GetList() const
{
    return _List;
}

size_t
SimSequenceView::GetListSize() const
{
    return _ListSize;
}
