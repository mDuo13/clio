//------------------------------------------------------------------------------
/*
    This file is part of clio: https://github.com/XRPLF/clio
    Copyright (c) 2023, the clio developers.

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

#include "rpc/Counters.hpp"
#include "rpc/JS.hpp"
#include "rpc/WorkQueue.hpp"
#include "util/LoggerFixtures.hpp"
#include "util/MockPrometheus.hpp"
#include "util/prometheus/Counter.hpp"

#include <boost/json/value_to.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <xrpl/protocol/jss.h>

#include <chrono>
#include <string>

using namespace rpc;

using util::prometheus::CounterInt;
using util::prometheus::WithMockPrometheus;
using util::prometheus::WithPrometheus;

struct RPCCountersTest : WithPrometheus, NoLoggerFixture {
    WorkQueue queue{4u, 1024u};  // todo: mock instead
    Counters counters{queue};
};

TEST_F(RPCCountersTest, CheckThatCountersAddUp)
{
    for (auto i = 0u; i < 512u; ++i) {
        counters.rpcErrored("error");
        counters.rpcComplete("complete", std::chrono::milliseconds{1u});
        counters.rpcForwarded("forward");
        counters.rpcFailedToForward("failedToForward");
        counters.rpcFailed("failed");
        counters.onTooBusy();
        counters.onNotReady();
        counters.onBadSyntax();
        counters.onUnknownCommand();
        counters.onInternalError();
    }

    auto const report = counters.report();
    auto const& rpc = report.at(JS(rpc)).as_object();

    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("error").as_object().at(JS(started))), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("error").as_object().at(JS(finished))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("error").as_object().at(JS(errored))), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("error").as_object().at("forwarded")), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("error").as_object().at("failed_forward")), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("error").as_object().at(JS(failed))), "0");

    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("complete").as_object().at(JS(started))), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("complete").as_object().at(JS(finished))), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("complete").as_object().at(JS(errored))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("complete").as_object().at("forwarded")), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("complete").as_object().at("failed_forward")), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("complete").as_object().at(JS(failed))), "0");
    EXPECT_EQ(
        boost::json::value_to<std::string>(rpc.at("complete").as_object().at(JS(duration_us))), "512000"
    );  // 1000 per call

    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("forward").as_object().at(JS(started))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("forward").as_object().at(JS(finished))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("forward").as_object().at(JS(errored))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("forward").as_object().at("forwarded")), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("forward").as_object().at("failed_forward")), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("forward").as_object().at(JS(failed))), "0");

    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failed").as_object().at(JS(started))), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failed").as_object().at(JS(finished))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failed").as_object().at(JS(errored))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failed").as_object().at("forwarded")), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failed").as_object().at("failed_forward")), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failed").as_object().at(JS(failed))), "512");

    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failedToForward").as_object().at(JS(started))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failedToForward").as_object().at(JS(finished))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failedToForward").as_object().at(JS(errored))), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failedToForward").as_object().at("forwarded")), "0");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failedToForward").as_object().at("failed_forward")), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(rpc.at("failedToForward").as_object().at(JS(failed))), "0");

    EXPECT_EQ(boost::json::value_to<std::string>(report.at("too_busy_errors")), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(report.at("not_ready_errors")), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(report.at("bad_syntax_errors")), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(report.at("unknown_command_errors")), "512");
    EXPECT_EQ(boost::json::value_to<std::string>(report.at("internal_errors")), "512");

    EXPECT_EQ(report.at("work_queue"), queue.report());  // Counters report includes queue report
}

struct RPCCountersMockPrometheusTests : WithMockPrometheus {
    WorkQueue queue{4u, 1024u};  // todo: mock instead
    Counters counters{queue};
};

TEST_F(RPCCountersMockPrometheusTests, rpcFailed)
{
    auto& startedMock = makeMock<CounterInt>("rpc_method_total_number", "{method=\"test\",status=\"started\"}");
    auto& failedMock = makeMock<CounterInt>("rpc_method_total_number", "{method=\"test\",status=\"failed\"}");
    EXPECT_CALL(startedMock, add(1));
    EXPECT_CALL(failedMock, add(1));
    counters.rpcFailed("test");
}

TEST_F(RPCCountersMockPrometheusTests, rpcErrored)
{
    auto& startedMock = makeMock<CounterInt>("rpc_method_total_number", "{method=\"test\",status=\"started\"}");
    auto& erroredMock = makeMock<CounterInt>("rpc_method_total_number", "{method=\"test\",status=\"errored\"}");
    EXPECT_CALL(startedMock, add(1));
    EXPECT_CALL(erroredMock, add(1));
    counters.rpcErrored("test");
}

TEST_F(RPCCountersMockPrometheusTests, rpcComplete)
{
    auto& startedMock = makeMock<CounterInt>("rpc_method_total_number", "{method=\"test\",status=\"started\"}");
    auto& finishedMock = makeMock<CounterInt>("rpc_method_total_number", "{method=\"test\",status=\"finished\"}");
    auto& durationMock = makeMock<CounterInt>("rpc_method_duration_us", "{method=\"test\"}");
    EXPECT_CALL(startedMock, add(1));
    EXPECT_CALL(finishedMock, add(1));
    EXPECT_CALL(durationMock, add(123));
    counters.rpcComplete("test", std::chrono::microseconds(123));
}

TEST_F(RPCCountersMockPrometheusTests, rpcForwarded)
{
    auto& forwardedMock = makeMock<CounterInt>("rpc_method_total_number", "{method=\"test\",status=\"forwarded\"}");
    EXPECT_CALL(forwardedMock, add(1));
    counters.rpcForwarded("test");
}

TEST_F(RPCCountersMockPrometheusTests, rpcFailedToForwarded)
{
    auto& failedForwadMock =
        makeMock<CounterInt>("rpc_method_total_number", "{method=\"test\",status=\"failed_forward\"}");
    EXPECT_CALL(failedForwadMock, add(1));
    counters.rpcFailedToForward("test");
}

TEST_F(RPCCountersMockPrometheusTests, onTooBusy)
{
    auto& tooBusyMock = makeMock<CounterInt>("rpc_error_total_number", "{error_type=\"too_busy\"}");
    EXPECT_CALL(tooBusyMock, add(1));
    counters.onTooBusy();
}

TEST_F(RPCCountersMockPrometheusTests, onNotReady)
{
    auto& notReadyMock = makeMock<CounterInt>("rpc_error_total_number", "{error_type=\"not_ready\"}");
    EXPECT_CALL(notReadyMock, add(1));
    counters.onNotReady();
}

TEST_F(RPCCountersMockPrometheusTests, onBadSyntax)
{
    auto& badSyntaxMock = makeMock<CounterInt>("rpc_error_total_number", "{error_type=\"bad_syntax\"}");
    EXPECT_CALL(badSyntaxMock, add(1));
    counters.onBadSyntax();
}

TEST_F(RPCCountersMockPrometheusTests, onUnknownCommand)
{
    auto& unknownCommandMock = makeMock<CounterInt>("rpc_error_total_number", "{error_type=\"unknown_command\"}");
    EXPECT_CALL(unknownCommandMock, add(1));
    counters.onUnknownCommand();
}

TEST_F(RPCCountersMockPrometheusTests, onInternalError)
{
    auto& internalErrorMock = makeMock<CounterInt>("rpc_error_total_number", "{error_type=\"internal_error\"}");
    EXPECT_CALL(internalErrorMock, add(1));
    counters.onInternalError();
}
