//------------------------------------------------------------------------------
/*
    This file is part of clio: https://github.com/XRPLF/clio
    Copyright (c) 2024, the clio developers.

    Permission to use, copy, modify, and distribute this software for any
    purpose with or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL,  DIRECT,  INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#pragma once

#include "data/BackendInterface.hpp"

#include <boost/asio/spawn.hpp>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Issue.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/UintTypes.h>

#include <cstdint>
#include <utility>

namespace rpc {

/**
 * @brief getAmmPoolHolds returns the balances of the amm asset pair
 *
 * @param backend The backend to use
 * @param sequence The sequence number to use
 * @param ammAccountID The amm account
 * @param issue1 The first issue
 * @param issue2 The second issue
 * @param freezeHandling Whether to return zeroes for frozen accounts
 * @param yield The coroutine context
 * @return The balances of the amm asset pair
 */
std::pair<ripple::STAmount, ripple::STAmount>
getAmmPoolHolds(
    BackendInterface const& backend,
    std::uint32_t sequence,
    ripple::AccountID const& ammAccountID,
    ripple::Issue const& issue1,
    ripple::Issue const& issue2,
    bool freezeHandling,
    boost::asio::yield_context yield
);

/**
 * @brief getAmmLpHolds returns the liquidity provider token balance
 *
 * @param backend The backend to use
 * @param sequence The sequence number to use
 * @param cur1 The first currency
 * @param cur2 The second currency
 * @param ammAccount The amm account
 * @param lpAccount The lp account
 * @param yield The coroutine context
 * @return The lp token balance
 */
ripple::STAmount
getAmmLpHolds(
    BackendInterface const& backend,
    std::uint32_t sequence,
    ripple::Currency const& cur1,
    ripple::Currency const& cur2,
    ripple::AccountID const& ammAccount,
    ripple::AccountID const& lpAccount,
    boost::asio::yield_context yield
);

/**
 * @brief getAmmLpHolds returns the liquidity provider token balance
 *
 * @param backend The backend to use
 * @param sequence The sequence number to use
 * @param ammSle The amm ledger entry
 * @param lpAccount The lp account
 * @param yield The coroutine context
 * @return The lp token balance
 */
ripple::STAmount
getAmmLpHolds(
    BackendInterface const& backend,
    std::uint32_t sequence,
    ripple::SLE const& ammSle,
    ripple::AccountID const& lpAccount,
    boost::asio::yield_context yield
);

}  // namespace rpc
