// -----------------------------------------------------------------------------------------------------
// Copyright (c) 2006-2023, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2023, Knut Reinert & MPI für molekulare Genetik
// This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
// shipped with this file.
// -----------------------------------------------------------------------------------------------------
#pragma once

#include "../BiFMIndexCursor.h"
#include "../FMIndexCursor.h"
#include "../ReverseFMIndexCursor.h"


namespace fmindex_collection {

template <typename Index>
struct SelectIndexCursor;

template <typename OccTable, typename TCSA>
struct SelectIndexCursor<BiFMIndex<OccTable, TCSA>> {
    using cursor_t = BiFMIndexCursor<BiFMIndex<OccTable, TCSA>>;
};

template <typename OccTable, typename TCSA>
struct SelectIndexCursor<FMIndex<OccTable, TCSA>> {
    using cursor_t = FMIndexCursor<FMIndex<OccTable, TCSA>>;
};

template <typename OccTable, typename TCSA>
struct SelectIndexCursor<ReverseFMIndex<OccTable, TCSA>> {
    using cursor_t = ReverseFMIndexCursor<ReverseFMIndex<OccTable, TCSA>>;
};

template <typename Index>
struct SelectLeftIndexCursor;

template <typename OccTable, typename TCSA>
struct SelectLeftIndexCursor<BiFMIndex<OccTable, TCSA>> {
    using cursor_t = LeftBiFMIndexCursor<BiFMIndex<OccTable, TCSA>>;
};

template <typename OccTable, typename TCSA>
struct SelectLeftIndexCursor<FMIndex<OccTable, TCSA>> {
    using cursor_t = FMIndexCursor<FMIndex<OccTable, TCSA>>;
};

template <typename Index>
using select_cursor_t      = typename SelectIndexCursor<Index>::cursor_t;

template <typename Index>
using select_left_cursor_t = typename SelectLeftIndexCursor<Index>::cursor_t;

}
