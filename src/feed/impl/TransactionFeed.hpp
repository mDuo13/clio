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
#include "data/Types.hpp"
#include "feed/Types.hpp"
#include "feed/impl/TrackableSignal.hpp"
#include "feed/impl/TrackableSignalMap.hpp"
#include "feed/impl/Util.hpp"
#include "util/log/Logger.hpp"
#include "util/prometheus/Gauge.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <fmt/core.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Book.h>
#include <xrpl/protocol/LedgerHeader.h>

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace feed::impl {

class TransactionFeed {
    // Hold two versions of transaction messages
    using AllVersionTransactionsType = std::array<std::shared_ptr<std::string>, 2>;

    struct TransactionSlot {
        std::reference_wrapper<TransactionFeed> feed;
        std::weak_ptr<Subscriber> connectionWeakPtr;

        TransactionSlot(TransactionFeed& feed, SubscriberSharedPtr const& connection)
            : feed(feed), connectionWeakPtr(connection)
        {
        }

        void
        operator()(AllVersionTransactionsType const& allVersionMsgs) const;
    };

    util::Logger logger_{"Subscriptions"};

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::reference_wrapper<util::prometheus::GaugeInt> subAllCount_;
    std::reference_wrapper<util::prometheus::GaugeInt> subAccountCount_;
    std::reference_wrapper<util::prometheus::GaugeInt> subBookCount_;

    TrackableSignalMap<ripple::AccountID, Subscriber, AllVersionTransactionsType const&> accountSignal_;
    TrackableSignalMap<ripple::Book, Subscriber, AllVersionTransactionsType const&> bookSignal_;
    TrackableSignal<Subscriber, AllVersionTransactionsType const&> signal_;

    // Signals for proposed tx subscribers
    TrackableSignalMap<ripple::AccountID, Subscriber, AllVersionTransactionsType const&> accountProposedSignal_;
    TrackableSignal<Subscriber, AllVersionTransactionsType const&> txProposedsignal_;

    std::unordered_set<SubscriberPtr>
        notified_;  // Used by slots to prevent double notifications if tx contains multiple subscribed accounts

public:
    /**
     * @brief Construct a new Transaction Feed object.
     * @param ioContext The actual publish will be called in the strand of this.
     */
    TransactionFeed(boost::asio::io_context& ioContext)
        : strand_(boost::asio::make_strand(ioContext))
        , subAllCount_(getSubscriptionsGaugeInt("tx"))
        , subAccountCount_(getSubscriptionsGaugeInt("account"))
        , subBookCount_(getSubscriptionsGaugeInt("book"))
    {
    }

    /**
     * @brief Subscribe to the transaction feed.
     * @param subscriber
     */
    void
    sub(SubscriberSharedPtr const& subscriber);

    /**
     * @brief Subscribe to the transaction feed, only receive the feed when particular account is affected.
     * @param subscriber
     * @param account The account to watch.
     */
    void
    sub(ripple::AccountID const& account, SubscriberSharedPtr const& subscriber);

    /**
     * @brief Subscribe to the transaction feed, only receive the feed when particular order book is affected.
     * @param subscriber
     * @param book The order book to watch.
     */
    void
    sub(ripple::Book const& book, SubscriberSharedPtr const& subscriber);

    /**
     * @brief Subscribe to the transaction feed for proposed transaction stream.
     * @param subscriber
     */
    void
    subProposed(SubscriberSharedPtr const& subscriber);

    /**
     * @brief Subscribe to the transaction feed for proposed account, only receive the feed when particular account is
     * affected.
     * @param subscriber
     * @param account The account to watch.
     */
    void
    subProposed(ripple::AccountID const& account, SubscriberSharedPtr const& subscriber);

    /**
     * @brief Unsubscribe to the transaction feed.
     * @param subscriber
     */
    void
    unsub(SubscriberSharedPtr const& subscriber);

    /**
     * @brief Unsubscribe to the transaction for particular account.
     * @param subscriber
     * @param account The account to unsubscribe.
     */
    void
    unsub(ripple::AccountID const& account, SubscriberSharedPtr const& subscriber);

    /**
     * @brief Unsubscribe to the transaction feed for proposed transaction stream.
     * @param subscriber
     */
    void
    unsubProposed(SubscriberSharedPtr const& subscriber);

    /**
     * @brief Unsubscribe to the transaction for particular proposed account.
     * @param subscriber
     * @param account The account to unsubscribe.
     */
    void
    unsubProposed(ripple::AccountID const& account, SubscriberSharedPtr const& subscriber);

    /**
     * @brief Unsubscribe to the transaction feed for particular order book.
     * @param subscriber
     * @param book The book to unsubscribe.
     */
    void
    unsub(ripple::Book const& book, SubscriberSharedPtr const& subscriber);

    /**
     * @brief Publishes the transaction feed.
     * @param txMeta The transaction and metadata.
     * @param lgrInfo The ledger header.
     * @param backend The backend.
     */
    void
    pub(data::TransactionAndMetadata const& txMeta,
        ripple::LedgerHeader const& lgrInfo,
        std::shared_ptr<data::BackendInterface const> const& backend);

    /**
     * @brief Get the number of subscribers of the transaction feed.
     */
    std::uint64_t
    transactionSubCount() const;

    /**
     * @brief Get the number of accounts subscribers.
     */
    std::uint64_t
    accountSubCount() const;

    /**
     * @brief Get the number of books subscribers.
     */
    std::uint64_t
    bookSubCount() const;

private:
    void
    unsubInternal(SubscriberPtr subscriber);

    void
    unsubInternal(ripple::AccountID const& account, SubscriberPtr subscriber);

    void
    unsubProposedInternal(SubscriberPtr subscriber);

    void
    unsubProposedInternal(ripple::AccountID const& account, SubscriberPtr subscriber);

    void
    unsubInternal(ripple::Book const& book, SubscriberPtr subscriber);
};
}  // namespace feed::impl
