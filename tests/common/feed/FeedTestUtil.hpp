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

#include "util/AsioContextTestFixture.hpp"
#include "util/MockBackendTestFixture.hpp"
#include "util/MockPrometheus.hpp"
#include "util/MockWsBase.hpp"
#include "web/interface/ConnectionBase.hpp"

#include <boost/json/parse.hpp>
#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <string>
#include <utility>

// Base class for feed tests, providing easy way to access the received feed
template <typename TestedFeed>
struct FeedBaseTest : util::prometheus::WithPrometheus, SyncAsioContextTest, MockBackendTest {
protected:
    std::shared_ptr<web::ConnectionBase> sessionPtr;
    std::shared_ptr<TestedFeed> testFeedPtr;
    MockSession* mockSessionPtr = nullptr;

    void
    SetUp() override
    {
        SyncAsioContextTest::SetUp();
        testFeedPtr = std::make_shared<TestedFeed>(ctx);
        sessionPtr = std::make_shared<MockSession>();
        sessionPtr->apiSubVersion = 1;
        mockSessionPtr = dynamic_cast<MockSession*>(sessionPtr.get());
    }

    void
    TearDown() override
    {
        sessionPtr.reset();
        testFeedPtr.reset();
        SyncAsioContextTest::TearDown();
    }
};

namespace impl {
class SharedStringJsonEqMatcher {
    std::string expected_;

public:
    using is_gtest_matcher = void;

    explicit SharedStringJsonEqMatcher(std::string expected) : expected_(std::move(expected))
    {
    }

    bool
    MatchAndExplain(std::shared_ptr<std::string> const& arg, std::ostream* /* listener */) const
    {
        return boost::json::parse(*arg) == boost::json::parse(expected_);
    }

    void
    DescribeTo(std::ostream* os) const
    {
        *os << "Contains json " << expected_;
    }

    void
    DescribeNegationTo(std::ostream* os) const
    {
        *os << "Expecting json " << expected_;
    }
};
}  // namespace impl

inline ::testing::Matcher<std::shared_ptr<std::string>>
SharedStringJsonEq(std::string const& expected)
{
    return impl::SharedStringJsonEqMatcher(expected);
}
