//------------------------------------------------------------------------------
/*
    This file is part of clio: https://github.com/XRPLF/clio
    Copyright (c) 2022, the clio developers.

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
#include "etl/ETLState.hpp"
#include "etl/NetworkValidatedLedgersInterface.hpp"
#include "etl/Source.hpp"
#include "etl/impl/ForwardingCache.hpp"
#include "feed/SubscriptionManagerInterface.hpp"
#include "rpc/Errors.hpp"
#include "util/config/Config.hpp"
#include "util/log/Logger.hpp"

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <grpcpp/grpcpp.h>
#include <org/xrpl/rpc/v1/get_ledger.pb.h>
#include <org/xrpl/rpc/v1/ledger.pb.h>
#include <xrpl/proto/org/xrpl/rpc/v1/xrp_ledger.grpc.pb.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace etl {

/**
 * @brief This class is used to manage connections to transaction processing processes.
 *
 * This class spawns a listener for each etl source, which listens to messages on the ledgers stream (to keep track of
 * which ledgers have been validated by the network, and the range of ledgers each etl source has). This class also
 * allows requests for ledger data to be load balanced across all possible ETL sources.
 */
class LoadBalancer {
public:
    using RawLedgerObjectType = org::xrpl::rpc::v1::RawLedgerObject;
    using GetLedgerResponseType = org::xrpl::rpc::v1::GetLedgerResponse;
    using OptionalGetLedgerResponseType = std::optional<GetLedgerResponseType>;

private:
    static constexpr std::uint32_t DEFAULT_DOWNLOAD_RANGES = 16;

    util::Logger log_{"ETL"};
    // Forwarding cache must be destroyed after sources because sources have a callback to invalidate cache
    std::optional<impl::ForwardingCache> forwardingCache_;
    std::optional<std::string> forwardingXUserValue_;

    std::vector<SourcePtr> sources_;
    std::optional<ETLState> etlState_;
    std::uint32_t downloadRanges_ =
        DEFAULT_DOWNLOAD_RANGES; /*< The number of markers to use when downloading initial ledger */
    std::atomic_bool hasForwardingSource_{false};

public:
    /**
     * @brief Value for the X-User header when forwarding admin requests
     */
    static constexpr std::string_view ADMIN_FORWARDING_X_USER_VALUE = "clio_admin";

    /**
     * @brief Value for the X-User header when forwarding user requests
     */
    static constexpr std::string_view USER_FORWARDING_X_USER_VALUE = "clio_user";

    /**
     * @brief Create an instance of the load balancer.
     *
     * @param config The configuration to use
     * @param ioc The io_context to run on
     * @param backend BackendInterface implementation
     * @param subscriptions Subscription manager
     * @param validatedLedgers The network validated ledgers datastructure
     * @param sourceFactory A factory function to create a source
     */
    LoadBalancer(
        util::Config const& config,
        boost::asio::io_context& ioc,
        std::shared_ptr<BackendInterface> backend,
        std::shared_ptr<feed::SubscriptionManagerInterface> subscriptions,
        std::shared_ptr<NetworkValidatedLedgersInterface> validatedLedgers,
        SourceFactory sourceFactory = make_Source
    );

    /**
     * @brief A factory function for the load balancer.
     *
     * @param config The configuration to use
     * @param ioc The io_context to run on
     * @param backend BackendInterface implementation
     * @param subscriptions Subscription manager
     * @param validatedLedgers The network validated ledgers datastructure
     * @param sourceFactory A factory function to create a source
     * @return A shared pointer to a new instance of LoadBalancer
     */
    static std::shared_ptr<LoadBalancer>
    make_LoadBalancer(
        util::Config const& config,
        boost::asio::io_context& ioc,
        std::shared_ptr<BackendInterface> backend,
        std::shared_ptr<feed::SubscriptionManagerInterface> subscriptions,
        std::shared_ptr<NetworkValidatedLedgersInterface> validatedLedgers,
        SourceFactory sourceFactory = make_Source
    );

    ~LoadBalancer();

    /**
     * @brief Load the initial ledger, writing data to the queue.
     * @note This function will retry indefinitely until the ledger is downloaded.
     *
     * @param sequence Sequence of ledger to download
     * @param cacheOnly Whether to only write to cache and not to the DB; defaults to false
     * @param retryAfter Time to wait between retries (2 seconds by default)
     * @return A std::vector<std::string> The ledger data
     */
    std::vector<std::string>
    loadInitialLedger(
        uint32_t sequence,
        bool cacheOnly = false,
        std::chrono::steady_clock::duration retryAfter = std::chrono::seconds{2}
    );

    /**
     * @brief Fetch data for a specific ledger.
     *
     * This function will continuously try to fetch data for the specified ledger until the fetch succeeds, the ledger
     * is found in the database, or the server is shutting down.
     *
     * @param ledgerSequence Sequence of the ledger to fetch
     * @param getObjects Whether to get the account state diff between this ledger and the prior one
     * @param getObjectNeighbors Whether to request object neighbors
     * @param retryAfter Time to wait between retries (2 seconds by default)
     * @return The extracted data, if extraction was successful. If the ledger was found
     * in the database or the server is shutting down, the optional will be empty
     */
    OptionalGetLedgerResponseType
    fetchLedger(
        uint32_t ledgerSequence,
        bool getObjects,
        bool getObjectNeighbors,
        std::chrono::steady_clock::duration retryAfter = std::chrono::seconds{2}
    );

    /**
     * @brief Represent the state of this load balancer as a JSON object
     *
     * @return JSON representation of the state of this load balancer.
     */
    boost::json::value
    toJson() const;

    /**
     * @brief Forward a JSON RPC request to a randomly selected rippled node.
     *
     * @param request JSON-RPC request to forward
     * @param clientIp The IP address of the peer, if known
     * @param isAdmin Whether the request is from an admin
     * @param yield The coroutine context
     * @return Response received from rippled node as JSON object on success or error on failure
     */
    std::expected<boost::json::object, rpc::ClioError>
    forwardToRippled(
        boost::json::object const& request,
        std::optional<std::string> const& clientIp,
        bool isAdmin,
        boost::asio::yield_context yield
    );

    /**
     * @brief Return state of ETL nodes.
     * @return ETL state, nullopt if etl nodes not available
     */
    std::optional<ETLState>
    getETLState() noexcept;

private:
    /**
     * @brief Execute a function on a randomly selected source.
     *
     * @note f is a function that takes an Source as an argument and returns a bool.
     * Attempt to execute f for one randomly chosen Source that has the specified ledger. If f returns false, another
     * randomly chosen Source is used. The process repeats until f returns true.
     *
     * @param f Function to execute. This function takes the ETL source as an argument, and returns a bool
     * @param ledgerSequence f is executed for each Source that has this ledger
     * @param retryAfter Time to wait between retries (2 seconds by default)
     * server is shutting down
     */
    template <typename Func>
    void
    execute(Func f, uint32_t ledgerSequence, std::chrono::steady_clock::duration retryAfter = std::chrono::seconds{2});

    /**
     * @brief Choose a new source to forward requests
     */
    void
    chooseForwardingSource();
};

}  // namespace etl
