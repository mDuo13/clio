#include <reporting/P2pProxy.h>
#include <reporting/server/session.h>

void
fail(boost::beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

std::pair<boost::json::object, uint32_t>
buildResponse(
    boost::json::object const& request,
    std::shared_ptr<BackendInterface> backend,
    std::shared_ptr<SubscriptionManager> manager,
    std::shared_ptr<ETLLoadBalancer> balancer,
    std::shared_ptr<session> session)
{
    std::string command = request.at("command").as_string().c_str();
    BOOST_LOG_TRIVIAL(info) << "Received rpc command : " << request;
    boost::json::object response;

    if (shouldForwardToP2p(request))
        return {balancer->forwardToP2p(request), 10};

    switch (commandMap[command])
    {
        case tx:
            return {doTx(request, *backend), 1};
        case account_tx: {
            auto res = doAccountTx(request, *backend);
            if (res.contains("transactions"))
                return {res, res["transactions"].as_array().size()};
            return {res, 1};
        }
        case ledger: {
            auto res = doLedger(request, *backend);
            if (res.contains("transactions"))
                return {res, res["transactions"].as_array().size()};
            return {res, 1};
        }
        case ledger_entry:
            return {doLedgerEntry(request, *backend), 1};
        case ledger_range:
            return {doLedgerRange(request, *backend), 1};
        case ledger_data: {
            auto res = doLedgerData(request, *backend);
            if (res.contains("objects"))
                return {res, res["objects"].as_array().size() * 4};
            return {res, 1};
        }
        case account_info:
            return {doAccountInfo(request, *backend), 1};
        case book_offers: {
            auto res = doBookOffers(request, *backend);
            if (res.contains("offers"))
                return {res, res["offers"].as_array().size() * 4};
            return {res, 1};
        }
        case account_channels: {
            auto res = doAccountChannels(request, *backend);
            if (res.contains("channels"))
                return {res, res["channels"].as_array().size()};
            return {res, 1};
        }
        case account_lines: {
            auto res = doAccountLines(request, *backend);
            if (res.contains("lines"))
                return {res, res["lines"].as_array().size()};
            return {res, 1};
        }
        case account_currencies: {
            auto res = doAccountCurrencies(request, *backend);
            size_t count = 1;
            if (res.contains("send_currencies"))
                count = res["send_currencies"].as_array().size();
            if (res.contains("receive_currencies"))
                count += res["receive_currencies"].as_array().size();
            return {res, count};
        }

        case account_offers: {
            auto res = doAccountOffers(request, *backend);
            if (res.contains("offers"))
                return {res, res["offers"].as_array().size()};
            return {res, 1};
        }
        case account_objects: {
            auto res = doAccountObjects(request, *backend);
            if (res.contains("objects"))
                return {res, res["objects"].as_array().size()};
            return {res, 1};
        }
        case channel_authorize: {
            return {doChannelAuthorize(request), 1};
        };
        case channel_verify:
            return {doChannelVerify(request), 1};
        case subscribe:
            return {doSubscribe(request, session, *manager), 1};
        case unsubscribe:
            return {doUnsubscribe(request, session, *manager), 1};
        case server_info: {
            return {doServerInfo(request, *backend), 1};
            break;
        }
        default:
            response["error"] = "Unknown command: " + command;
            return {response, 1};
    }
}
