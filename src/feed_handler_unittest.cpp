#include <functional>
#include <utility>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "feed_handler.h"


test_ns::symbol_t test_symbol_1 = "S1";
test_ns::symbol_t test_symbol_2 = "S2";

/*
 *
 */
struct test_callback_t {
    std::vector<std::string> output;
    std::vector<std::pair<std::string, std::string>> errors;
    void ok_func(const std::string& s) {
        output.push_back(s);
    }
    void err_func(const std::string& line, const std::string& err) {
        errors.push_back(std::make_pair(line, err));
    }
};

struct get_price_levels_t {
    using price_levels_t = std::vector<test_ns::price_level_t>;
    price_levels_t bids;
    price_levels_t sales;
    void func(const test_ns::price_level_t& bid,
            const test_ns::price_level_t& sale) {
        bids.push_back(bid);
        sales.push_back(sale);
    }
};

struct get_full_orders_t {
    using full_orders_line_t = std::vector<test_ns::full_orders_t>;
    full_orders_line_t bids;
    full_orders_line_t asks;
    void operator()(const test_ns::full_orders_t& bid,
            const test_ns::full_orders_t& ask) {
        bids.push_back(bid);
        asks.push_back(ask);
    }
};


/*
 *
 */
#define CREATE_DEFAULT_TEST_HANDLER \
        test_callback_t a_test_object; \
        test_ns::callback_t \
            a_callback = std::bind(&test_callback_t::ok_func, \
                    &a_test_object, std::placeholders::_1); \
        test_ns::err_callback_t an_err_callback = std::bind( \
                    &test_callback_t::err_func, &a_test_object, \
                    std::placeholders::_1, std::placeholders::_2); \
        test_ns::feed_handler a_handler("", \
                std::move(a_callback), std::move(an_err_callback));

#define CREATE_SYMBOL_TEST_HANDLER(symbol) \
        test_callback_t a_test_object; \
        test_ns::callback_t \
            a_callback = std::bind(&test_callback_t::ok_func, \
                    &a_test_object, std::placeholders::_1); \
        test_ns::err_callback_t an_err_callback = std::bind( \
                    &test_callback_t::err_func, &a_test_object, \
                    std::placeholders::_1, std::placeholders::_2); \
        test_ns::feed_handler a_handler(symbol, \
                std::move(a_callback), std::move(an_err_callback));


#define PRINT_CALLBACK_ERRORS \
        for (auto & err : a_test_object.errors) { \
            std::cout << "LINE: " << err.first << \
                "; ERR: " << err.second << std::endl; \
        }

#define CHECK_INVALID_NUMBER_OF_PARAMS \
        ASSERT_TRUE(a_test_object.output.empty()); \
        ASSERT_FALSE(a_test_object.errors.empty()); \
        ASSERT_EQ(a_test_object.errors.size(), 1); \
        ASSERT_TRUE(a_test_object.errors[0].second. \
                find("invalid number of parameters") != \
                a_test_object.errors[0].second.npos);

/*
 *
 */
TEST(FeedHandler, CreateWithEmptyCallback) {
    try {
        test_ns::callback_t a_callback;
        test_ns::err_callback_t an_err_callback;
        test_ns::feed_handler a_handler("",
                std::move(a_callback), std::move(an_err_callback));
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, CreateWithDefCallback) {
    try {
        test_ns::callback_t
            a_callback{test_ns::print_to_stdout};
        test_ns::err_callback_t
            an_err_callback{test_ns::print_to_stderr};
        test_ns::feed_handler a_handler("",
                std::move(a_callback), std::move(an_err_callback));
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, CreateWithTestCallback) {
    try {
        test_callback_t a_test_object;
        test_ns::callback_t
            a_callback = std::bind(&test_callback_t::ok_func,
                    &a_test_object, std::placeholders::_1);
        test_ns::err_callback_t an_err_callback = std::bind(
                    &test_callback_t::err_func, &a_test_object,
                    std::placeholders::_1, std::placeholders::_2);
        test_ns::feed_handler a_handler("",
                std::move(a_callback), std::move(an_err_callback));
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, CreateWithTestCallback2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        ASSERT_FALSE(a_handler.is_there_selected_symbol());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, CreateWithTestCallbackWithSymbol) {
    try {
        CREATE_SYMBOL_TEST_HANDLER("S1");
        ASSERT_TRUE(a_handler.is_there_selected_symbol());
        ASSERT_STREQ(a_handler.get_selected_symbol().c_str(), "S1");
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, EmptyCmd) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_EQ(a_test_object.errors.size(), 1);
        ASSERT_STREQ(a_test_object.errors[0].first.c_str(),"");
        ASSERT_STREQ(a_test_object.errors[0].second.c_str(),
                "incorrect command");
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, IncorrectOrder) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        const char incorrect_line[] = "INCORRECT ORDER,1,2,3";
        a_handler.process_command(incorrect_line);
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_EQ(a_test_object.errors.size(), 1);
        ASSERT_STREQ(a_test_object.errors[0].first.c_str(),
                incorrect_line);
        ASSERT_STREQ(a_test_object.errors[0].second.c_str(),
                "incorrect command");
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderAddFewParams) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_FALSE(a_test_object.errors.empty());
        ASSERT_EQ(a_test_object.errors.size(), 1);
        ASSERT_TRUE(a_test_object.errors[0].second.
                find("invalid number of parameters") !=
                a_test_object.errors[0].second.npos);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderAddFewParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_FALSE(a_test_object.errors.empty());
        ASSERT_EQ(a_test_object.errors.size(), 1);
        ASSERT_TRUE(a_test_object.errors[0].second.
                find("invalid number of parameters") !=
                a_test_object.errors[0].second.npos);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderAddMoreParams) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33,1");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_FALSE(a_test_object.errors.empty());
        ASSERT_EQ(a_test_object.errors.size(), 1);
        ASSERT_TRUE(a_test_object.errors[0].second.
                find("invalid number of parameters") !=
                a_test_object.errors[0].second.npos);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderAddMoreParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33,1,2");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_FALSE(a_test_object.errors.empty());
        ASSERT_EQ(a_test_object.errors.size(), 1);
        ASSERT_TRUE(a_test_object.errors[0].second.
                find("invalid number of parameters") !=
                a_test_object.errors[0].second.npos);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderAddBuy) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33");
        ASSERT_TRUE(a_test_object.output.empty());
        auto order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 3.33);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderAddSell) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Sell,30,4.33");
        ASSERT_TRUE(a_test_object.output.empty());
        auto order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::sell);
        ASSERT_EQ(order_1.second.quantity, 30);
        ASSERT_EQ(order_1.second.price, 4.33);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, SelectedSymbolOrderAddBuy) {
    try {
        CREATE_SYMBOL_TEST_HANDLER(test_symbol_1);
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33");
        ASSERT_TRUE(a_test_object.output.empty());
        auto order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 3.33);
    } catch (std::exception& e) {
        FAIL() << "Exception caught: " << e.what();
    }
}

TEST(FeedHandler, SelectedSymbolPrint) {
    try {
        CREATE_SYMBOL_TEST_HANDLER(test_symbol_1);
        a_handler.process_command("PRINT,S2");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
        a_handler.process_command("ORDER ADD,1,S2,Buy,20,3.33");
        a_handler.process_command("PRINT,S2");
        PRINT_CALLBACK_ERRORS;
        ASSERT_TRUE(a_test_object.errors.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
    } catch (std::exception& e) {
        FAIL() << "Exception caught: " << e.what();
    }
}

TEST(FeedHandler, OrderAddBuyDuplicate) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33");
        ASSERT_TRUE(a_test_object.output.empty());
        auto order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 3.33);

        ASSERT_TRUE(a_test_object.errors.empty());
        a_handler.process_command("ORDER ADD,1,S1,Sell,30,4.33");
        ASSERT_EQ(a_test_object.errors.size(), 1);
        auto order_2 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_TRUE(order_2.first);
        ASSERT_EQ(order_2.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_2.second.quantity, 20);
        ASSERT_EQ(order_2.second.price, 3.33);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderAddBuyTwoSymbols) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33");
        a_handler.process_command("ORDER ADD,1,S2,Sell,30,4.33");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_EQ(a_test_object.errors.size(), 0);

        auto order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 3.33);

        auto order_2 = a_handler.get_order(test_symbol_2, 1);
        ASSERT_TRUE(order_2.first);
        ASSERT_EQ(order_2.second.side, test_ns::side_t::sell);
        ASSERT_EQ(order_2.second.quantity, 30);
        ASSERT_EQ(order_2.second.price, 4.33);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderModifyInvalidNumParams1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER MODIFY,1,30");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderModifyInvalidNumParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER MODIFY,S1,1,30,4.01");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderModifyInvalidID) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER MODIFY,1,30,4.01");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_FALSE(a_test_object.errors.empty());
        auto order = a_handler.get_order(test_symbol_1, 1);
        ASSERT_FALSE(order.first);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderModifyBuy) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33");
        ASSERT_TRUE(a_test_object.output.empty());
        auto order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_STREQ(a_handler.get_symbol_for_order(1).c_str(), "S1");
        ASSERT_TRUE(order_1.first);
        ASSERT_TRUE(a_test_object.errors.empty());
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 3.33);

        a_handler.process_command("ORDER MODIFY,1,30,4.01");
        ASSERT_STREQ(a_handler.get_symbol_for_order(1).c_str(), "S1");
        order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 30);
        ASSERT_EQ(order_1.second.price, 4.01);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderCancelInvalidNumParams1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER CANCEL");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderCancelInvalidNumParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER CANCEL,S1,1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderCancelInvalidID) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER CANCEL,1");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_FALSE(a_test_object.errors.empty());
        auto order = a_handler.get_order(test_symbol_1, 1);
        ASSERT_FALSE(order.first);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, OrderCancelBuy) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33");
        ASSERT_TRUE(a_test_object.output.empty());
        auto order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_STREQ(a_handler.get_symbol_for_order(1).c_str(), "S1");
        ASSERT_TRUE(order_1.first);
        ASSERT_TRUE(a_test_object.errors.empty());
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 3.33);

        a_handler.process_command("ORDER MODIFY,1,30,4.01");
        ASSERT_STREQ(a_handler.get_symbol_for_order(1).c_str(), "S1");
        order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 30);
        ASSERT_EQ(order_1.second.price, 4.01);

        a_handler.process_command("ORDER CANCEL,1");
        ASSERT_FALSE(a_handler.is_there_symbol_for_order(1));
        ASSERT_TRUE(a_test_object.errors.empty());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintInvalidNumParams1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("PRINT,S1,1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintInvalidNumParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("PRINT");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintOneOrder) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT,S1");
        ASSERT_EQ(a_test_object.output.size(), 1);
        char expected_ouput[200];
        sprintf(expected_ouput, "%-20s | %-20s", "20@3.33", "");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintTwoBuysPrintResults) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,10.0");
        a_handler.process_command("ORDER ADD,2,S1,Buy,30,10.0");
        a_handler.process_command("ORDER ADD,3,S1,Buy,2,12.0");
        a_handler.process_command("ORDER ADD,4,S1,Buy,3,12.0");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT,S1");
        ASSERT_EQ(a_test_object.output.size(), 2);
        char expected_ouput[200];
        sprintf(expected_ouput, "%-20s | %-20s", "5@12", "");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);
        sprintf(expected_ouput, "%-20s | %-20s", "50@10", "");
        ASSERT_STREQ(a_test_object.output[1].c_str(), expected_ouput);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintTwoSalesPrintResults) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Sell,20,10.1");
        a_handler.process_command("ORDER ADD,2,S1,Sell,30,10.1");
        a_handler.process_command("ORDER ADD,3,S1,Sell,2,12.0");
        a_handler.process_command("ORDER ADD,4,S1,Sell,3,12.0");

        a_handler.process_command("ORDER ADD,5,S1,Buy,20,10.2");
        a_handler.process_command("ORDER ADD,6,S1,Buy,30,10.2");
        a_handler.process_command("ORDER ADD,7,S1,Buy,2,12.33");
        a_handler.process_command("ORDER ADD,8,S1,Buy,3,12.33");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT,S1");
        ASSERT_EQ(a_test_object.output.size(), 2);
        char expected_ouput[200];
        sprintf(expected_ouput, "%-20s | %-20s", "5@12.33", "50@10.1");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);
        sprintf(expected_ouput, "%-20s | %-20s", "50@10.2", "5@12");
        ASSERT_STREQ(a_test_object.output[1].c_str(), expected_ouput);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintTwoBothPrintResults) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Sell,20,10.1");
        a_handler.process_command("ORDER ADD,2,S1,Sell,30,10.1");
        a_handler.process_command("ORDER ADD,3,S1,Sell,2,12.0");
        a_handler.process_command("ORDER ADD,4,S1,Sell,3,12.0");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT,S1");
        ASSERT_EQ(a_test_object.output.size(), 2);
        char expected_ouput[200];
        sprintf(expected_ouput, "%-20s | %-20s", "", "50@10.1");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);
        sprintf(expected_ouput, "%-20s | %-20s", "", "5@12");
        ASSERT_STREQ(a_test_object.output[1].c_str(), expected_ouput);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintFullInvalidNumParams1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("PRINT_FULL,S1,1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintFullInvalidNumParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("PRINT_FULL");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintFullEmpty) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT_FULL,S1");
        ASSERT_EQ(a_test_object.output.size(), 0);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

static void format_column(std::ostringstream &ss) {
    ss << std::left << std::setw(10);
};

static void format_number(std::ostringstream &ss, unsigned value) {
    format_column(ss);
    ss << value;
};

static void format_quantity(std::ostringstream &ss,
                               test_ns::quantity_t value) {
    format_column(ss);
    ss << value;
};

static void format_double(std::ostringstream &ss, double value) {
    format_column(ss);
    ss << value;
};

static void format_spaces(std::ostringstream &ss, unsigned times) {
    for (auto i = 0U; i < times; ++i) {
        format_column(ss);
        ss << ' ';
    }
};

TEST(FeedHandler, PrintFullBuy1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,100,10.");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT_FULL,S1");
        ASSERT_EQ(a_test_object.output.size(), 5);
        std::ostringstream ss;
        format_number(ss, 1);
        format_quantity(ss, 100);
        format_double(ss, 10.);
        format_spaces(ss, 3);
        ASSERT_STREQ(a_test_object.output[3].c_str(), ss.str().c_str());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintFullBuy2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,100,10.");
        a_handler.process_command("ORDER ADD,2,S1,Buy,200,9.");
        a_handler.process_command("ORDER ADD,3,S1,Buy,300,10.");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT_FULL,S1");
        ASSERT_EQ(a_test_object.output.size(), 6);
        std::ostringstream ss;
        format_number(ss, 2);
        format_quantity(ss, 400);
        format_double(ss, 10.);
        format_spaces(ss, 3);
        ASSERT_STREQ(a_test_object.output[3].c_str(), ss.str().c_str());
        ss.str("");
        format_number(ss, 1);
        format_quantity(ss, 200);
        format_double(ss, 9.);
        format_spaces(ss, 3);
        ASSERT_STREQ(a_test_object.output[4].c_str(), ss.str().c_str());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintFullSell1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Sell,100,10.");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT_FULL,S1");
        ASSERT_EQ(a_test_object.output.size(), 5);
        std::ostringstream ss;
        format_spaces(ss, 3);
        format_number(ss, 1);
        format_quantity(ss, 100);
        format_double(ss, 10.);
        ASSERT_STREQ(a_test_object.output[3].c_str(), ss.str().c_str());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintFullSell2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Sell,100,10.");
        a_handler.process_command("ORDER ADD,2,S1,Sell,200,11.");
        a_handler.process_command("ORDER ADD,3,S1,Sell,300,10.");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT_FULL,S1");
        ASSERT_EQ(a_test_object.output.size(), 6);
        std::ostringstream ss;
        format_spaces(ss, 3);
        format_number(ss, 2);
        format_quantity(ss, 400);
        format_double(ss, 10.);
        ASSERT_STREQ(a_test_object.output[3].c_str(), ss.str().c_str());
        ss.str("");
        format_spaces(ss, 3);
        format_number(ss, 1);
        format_quantity(ss, 200);
        format_double(ss, 11.);
        ASSERT_STREQ(a_test_object.output[4].c_str(), ss.str().c_str());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, PrintFullBuySell) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Sell,100,10.");
        a_handler.process_command("ORDER ADD,2,S1,Sell,200,11.");
        a_handler.process_command("ORDER ADD,3,S1,Sell,300,10.");
        a_handler.process_command("ORDER ADD,4,S1,Buy,100,10.");
        a_handler.process_command("ORDER ADD,5,S1,Buy,200,9.");
        a_handler.process_command("ORDER ADD,6,S1,Buy,300,10.");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("PRINT_FULL,S1");
        ASSERT_EQ(a_test_object.output.size(), 6);
        std::ostringstream ss;
        format_number(ss, 2);
        format_quantity(ss, 400);
        format_double(ss, 10.);
        format_number(ss, 2);
        format_quantity(ss, 400);
        format_double(ss, 10.);
        ASSERT_STREQ(a_test_object.output[3].c_str(), ss.str().c_str());
        ss.str("");
        format_number(ss, 1);
        format_quantity(ss, 200);
        format_double(ss, 9.);
        format_number(ss, 1);
        format_quantity(ss, 200);
        format_double(ss, 11.);
        ASSERT_STREQ(a_test_object.output[4].c_str(), ss.str().c_str());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, SubsBBOInvalidNumParams1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("SUBSCRIBE BBO");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, SubsBBOInvalidNumParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("SUBSCRIBE BBO,S1,1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, BBOBuyOnly) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,10.1");
        a_handler.process_command("ORDER ADD,2,S1,Buy,30,10.1");
        a_handler.process_command("ORDER ADD,3,S1,Buy,2,12.0");
        a_handler.process_command("ORDER ADD,4,S1,Buy,3,12.0");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);

        a_handler.process_command("SUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 1);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 1);
        ASSERT_EQ(a_test_object.output.size(), 1);
        char expected_ouput[200];
        sprintf(expected_ouput, "BBO: %-10s%-20s | %-20s", "S1", "5@12", "");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);

        a_test_object.output.clear();
        a_handler.process_command("ORDER ADD,5,S1,Buy,4,14.1");
        sprintf(expected_ouput, "BBO: %-10s%-20s | %-20s", "S1", "4@14.1", "");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);

        a_test_object.output.clear();
        a_handler.process_command("ORDER ADD,6,S1,Buy,5,14.1");
        sprintf(expected_ouput, "BBO: %-10s%-20s | %-20s", "S1", "9@14.1", "");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, BBOSellOnly) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Sell,20,10.1");
        a_handler.process_command("ORDER ADD,2,S1,Sell,30,10.1");
        a_handler.process_command("ORDER ADD,3,S1,Sell,2,12.0");
        a_handler.process_command("ORDER ADD,4,S1,Sell,3,12.0");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);

        a_handler.process_command("SUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 1);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 1);
        ASSERT_EQ(a_test_object.output.size(), 1);
        char expected_ouput[200];
        sprintf(expected_ouput, "BBO: %-10s%-20s | %-20s", "S1", "", "50@10.1");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);

        a_test_object.output.clear();
        a_handler.process_command("ORDER ADD,5,S1,Sell,4,9.1");
        sprintf(expected_ouput, "BBO: %-10s%-20s | %-20s", "S1", "", "4@9.1");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);

        a_test_object.output.clear();
        a_handler.process_command("ORDER ADD,6,S1,Sell,5,9.1");
        sprintf(expected_ouput, "BBO: %-10s%-20s | %-20s", "S1", "", "9@9.1");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, BBOBuyAndSell) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,10.1");
        a_handler.process_command("ORDER ADD,2,S1,Buy,30,10.1");
        a_handler.process_command("ORDER ADD,3,S1,Buy,2,12.0");
        a_handler.process_command("ORDER ADD,4,S1,Buy,3,12.0");
        a_handler.process_command("ORDER ADD,5,S1,Sell,20,10.1");
        a_handler.process_command("ORDER ADD,6,S1,Sell,30,10.1");
        a_handler.process_command("ORDER ADD,7,S1,Sell,2,12.0");
        a_handler.process_command("ORDER ADD,8,S1,Sell,3,12.0");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);

        a_handler.process_command("SUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 1);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 1);
        ASSERT_EQ(a_test_object.output.size(), 1);
        char expected_ouput[200];
        sprintf(expected_ouput, "BBO: %-10s%-20s | %-20s",
                "S1", "5@12", "50@10.1");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, UnsubsBBOInvalidNumParams1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("UNSUBSCRIBE BBO");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, UnsubsBBOInvalidNumParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("UNSUBSCRIBE BBO,S1,1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, UnsubsBBO) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);
        a_handler.process_command("UNSUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, SubsUnsubsBBO) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);

        a_handler.process_command("SUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 1);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 1);
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("UNSUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 0);
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, SubsAddOrderUnsubsBBO) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("SUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 1);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 1);
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("SUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 1);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 2);
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("ORDER ADD,1,S1,Buy,20,10.1");
        ASSERT_EQ(a_test_object.output.size(), 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        a_test_object.output.clear();

        a_handler.process_command("ORDER ADD,2,S1,Sell,20,10.1");
        ASSERT_EQ(a_test_object.output.size(), 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        char expected_ouput[200];
        sprintf(expected_ouput, "BBO: %-10s%-20s | %-20s",
                "S1", "20@10.1", "20@10.1");
        ASSERT_STREQ(a_test_object.output[0].c_str(), expected_ouput);
        a_test_object.output.clear();

        a_handler.process_command("UNSUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 1);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 1);
        ASSERT_EQ(a_test_object.output.size(), 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        a_test_object.output.clear();

        a_handler.process_command("UNSUBSCRIBE BBO,S1");
        ASSERT_EQ(a_handler.get_total_number_bbo_subs(), 0);
        ASSERT_EQ(a_handler.get_bbo_subs_number("S1"), 0);
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("ORDER ADD,3,S1,Buy,20,10.1");
        a_handler.process_command("ORDER ADD,4,S1,Sell,20,10.1");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}
TEST(FeedHandler, SubsVWAPInvalidNumParams1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("SUBSCRIBE VWAP,S1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, SubsVWAPInvalidNumParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("SUBSCRIBE VWAP,S1,5,1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, VWAPBuyOnly) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
        a_handler.process_command("SUBSCRIBE VWAP,S1,5");
        std::ostringstream expected_ouput;
        expected_ouput << "VWAP: " << std::left << std::setw(10) << "S1"
                << " <NIL,NIL>";
        ASSERT_STREQ(a_test_object.output[0].c_str(),
                expected_ouput.str().c_str());
        a_test_object.output.clear();

        a_handler.process_command("ORDER ADD,1,S1,Buy,10,72.82");
        a_test_object.output.clear();

        a_handler.process_command("ORDER ADD,2,S1,Buy,100,72.81");
        expected_ouput.str("");
        expected_ouput << "VWAP: " << std::left << std::setw(10) << "S1"
                << " <" << 72.82 <<  ",NIL>";
        ASSERT_STREQ(a_test_object.output[0].c_str(),
                expected_ouput.str().c_str());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}
TEST(FeedHandler, UnsubsVWAPInvalidNumParams1) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("UNSUBSCRIBE VWAP,S1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, UnsubsVWAPInvalidNumParams2) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;
        a_handler.process_command("UNSUBSCRIBE VWAP,S1,5,1");
        CHECK_INVALID_NUMBER_OF_PARAMS;
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, SubsAddOrderUnsubsVWAP) {
    try {
        CREATE_DEFAULT_TEST_HANDLER;

        ASSERT_EQ(a_handler.get_total_number_vwap_subs(), 0);
        ASSERT_EQ(a_handler.get_vwap_subs_number("S1", 5), 0);
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("SUBSCRIBE VWAP,S1,5");
        ASSERT_EQ(a_handler.get_total_number_vwap_subs(), 1);
        ASSERT_EQ(a_handler.get_vwap_subs_number("S1", 5), 1);
        ASSERT_EQ(a_test_object.output.size(), 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        a_test_object.output.clear();

        a_handler.process_command("SUBSCRIBE VWAP,S1,5");
        ASSERT_EQ(a_handler.get_total_number_vwap_subs(), 1);
        ASSERT_EQ(a_handler.get_vwap_subs_number("S1", 5), 2);
        ASSERT_EQ(a_test_object.output.size(), 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        a_test_object.output.clear();

        a_handler.process_command("ORDER ADD,1,S1,Buy,10,72.82");
        ASSERT_EQ(a_test_object.output.size(), 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        a_test_object.output.clear();

        a_handler.process_command("ORDER ADD,2,S1,Buy,100,72.81");
        ASSERT_EQ(a_test_object.output.size(), 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        a_test_object.output.clear();

        a_handler.process_command("UNSUBSCRIBE VWAP,S1,5");
        ASSERT_EQ(a_handler.get_total_number_vwap_subs(), 1);
        ASSERT_EQ(a_handler.get_vwap_subs_number("S1", 5), 1);
        ASSERT_EQ(a_test_object.output.size(), 1);
        ASSERT_TRUE(a_test_object.errors.empty());
        a_test_object.output.clear();

        a_handler.process_command("UNSUBSCRIBE VWAP,S1,5");
        ASSERT_EQ(a_handler.get_total_number_vwap_subs(), 0);
        ASSERT_EQ(a_handler.get_vwap_subs_number("S1", 5), 0);
        ASSERT_EQ(a_test_object.output.size(), 0);
        ASSERT_TRUE(a_test_object.errors.empty());

        a_handler.process_command("ORDER ADD,30,S1,Buy,20,10.1");
        a_handler.process_command("ORDER ADD,40,S1,Sell,20,10.1");
        ASSERT_TRUE(a_test_object.output.empty());
        ASSERT_TRUE(a_test_object.errors.empty());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(FeedHandler, SelectedSymbolOrderAddBuy2) {
    try {
        CREATE_SYMBOL_TEST_HANDLER(test_symbol_2);
        a_handler.process_command("ORDER ADD,1,S1,Buy,20,3.33");
        ASSERT_TRUE(a_test_object.output.empty());
        auto order_1 = a_handler.get_order(test_symbol_1, 1);
        ASSERT_FALSE(order_1.first);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

/*
 *
 */
TEST(OrderBook, Create) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        ASSERT_STREQ(an_order_book.get_symbol().c_str(), test_symbol_1.c_str());
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, AddOrder1) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        {
            auto order_1 = an_order_book.get_order(1);
            ASSERT_FALSE(order_1.first);
            an_order_book.add_order(1, test_ns::side_t::buy, 20, 3.33);
            order_1 = an_order_book.get_order(1);
            ASSERT_TRUE(order_1.first);
            ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
            ASSERT_EQ(order_1.second.quantity, 20);
            ASSERT_EQ(order_1.second.price, 3.33);
        }

        {
            auto order_2 = an_order_book.get_order(2);
            ASSERT_FALSE(order_2.first);
            an_order_book.add_order(2, test_ns::side_t::sell, 30, 4.33);
            order_2 = an_order_book.get_order(2);
            ASSERT_TRUE(order_2.first);
            ASSERT_EQ(order_2.second.side, test_ns::side_t::sell);
            ASSERT_EQ(order_2.second.quantity, 30);
            ASSERT_EQ(order_2.second.price, 4.33);

            auto order_1 = an_order_book.get_order(1);
            ASSERT_TRUE(order_1.first);
            ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
            ASSERT_EQ(order_1.second.quantity, 20);
            ASSERT_EQ(order_1.second.price, 3.33);
        }
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, AddDuplicatiteOrder) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        auto order_1 = an_order_book.get_order(1);
        ASSERT_FALSE(order_1.first);
        an_order_book.add_order(1, test_ns::side_t::buy, 20, 3.33);
        order_1 = an_order_book.get_order(1);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 3.33);

        an_order_book.add_order(1, test_ns::side_t::sell, 30, 4.33);
        FAIL() << "duplicate orders must result in exception";
    } catch (std::exception& e) {
        SUCCEED();
    }
}

TEST(OrderBook, GetPriceLevel) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        get_price_levels_t get_price_levels;
        test_ns::get_price_levels_callback_t callback1 =
                std::bind(&get_price_levels_t::func, &get_price_levels,
                        std::placeholders::_1, std::placeholders::_2);
        an_order_book.get_price_levels(std::move(callback1));

        ASSERT_EQ(get_price_levels.bids.size(), 0);
        ASSERT_EQ(get_price_levels.sales.size(), 0);

        an_order_book.add_order(1, test_ns::side_t::buy, 20, 3.33);
        test_ns::get_price_levels_callback_t callback2 =
                std::bind(&get_price_levels_t::func, &get_price_levels,
                        std::placeholders::_1, std::placeholders::_2);
        an_order_book.get_price_levels(std::move(callback2));

        ASSERT_EQ(get_price_levels.bids.size(), 1);
        ASSERT_EQ(get_price_levels.sales.size(), 1);
        ASSERT_TRUE(get_price_levels.bids[0].first);
        ASSERT_EQ(get_price_levels.bids[0].second.price, 3.33);
        ASSERT_EQ(get_price_levels.bids[0].second.volume, 20);
        ASSERT_FALSE(get_price_levels.sales[0].first);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, GetPriceLevelSale) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        an_order_book.add_order(1, test_ns::side_t::sell, 20, 10.);
        an_order_book.add_order(2, test_ns::side_t::sell, 40, 12.);
        an_order_book.add_order(3, test_ns::side_t::sell, 5, 10.);
        an_order_book.add_order(4, test_ns::side_t::sell, 10, 12.);

        get_price_levels_t get_price_levels;
        test_ns::get_price_levels_callback_t callback2 =
                std::bind(&get_price_levels_t::func, &get_price_levels,
                        std::placeholders::_1, std::placeholders::_2);
        an_order_book.get_price_levels(std::move(callback2));

        ASSERT_EQ(get_price_levels.bids.size(), 2);
        ASSERT_FALSE(get_price_levels.bids[0].first);
        ASSERT_FALSE(get_price_levels.bids[1].first);

        ASSERT_EQ(get_price_levels.sales.size(), 2);
        ASSERT_TRUE(get_price_levels.sales[0].first);
        ASSERT_EQ(get_price_levels.sales[0].second.price, 10.);
        ASSERT_EQ(get_price_levels.sales[0].second.volume, 25);
        ASSERT_TRUE(get_price_levels.sales[1].first);
        ASSERT_EQ(get_price_levels.sales[1].second.price, 12.);
        ASSERT_EQ(get_price_levels.sales[1].second.volume, 50);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, GetPriceLevelBye) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        an_order_book.add_order(1, test_ns::side_t::sell, 20, 10.);
        an_order_book.add_order(2, test_ns::side_t::sell, 40, 12.);
        an_order_book.add_order(3, test_ns::side_t::sell, 5, 10.);
        an_order_book.add_order(4, test_ns::side_t::sell, 10, 12.);
        an_order_book.add_order(5, test_ns::side_t::buy, 20, 10.);
        an_order_book.add_order(6, test_ns::side_t::buy, 40, 12.);
        an_order_book.add_order(7, test_ns::side_t::buy, 5, 10.);
        an_order_book.add_order(8, test_ns::side_t::buy, 10, 12.);

        get_price_levels_t get_price_levels;
        test_ns::get_price_levels_callback_t callback2 =
                std::bind(&get_price_levels_t::func, &get_price_levels,
                        std::placeholders::_1, std::placeholders::_2);
        an_order_book.get_price_levels(std::move(callback2));

        ASSERT_EQ(get_price_levels.sales.size(), 2);
        ASSERT_TRUE(get_price_levels.sales[0].first);
        ASSERT_EQ(get_price_levels.sales[0].second.price, 10.);
        ASSERT_EQ(get_price_levels.sales[0].second.volume, 25);
        ASSERT_TRUE(get_price_levels.sales[1].first);
        ASSERT_EQ(get_price_levels.sales[1].second.price, 12.);
        ASSERT_EQ(get_price_levels.sales[1].second.volume, 50);

        ASSERT_EQ(get_price_levels.bids.size(), 2);
        ASSERT_TRUE(get_price_levels.bids[0].first);
        ASSERT_EQ(get_price_levels.bids[0].second.price, 12.);
        ASSERT_EQ(get_price_levels.bids[0].second.volume, 50);
        ASSERT_TRUE(get_price_levels.bids[1].first);
        ASSERT_EQ(get_price_levels.bids[1].second.price, 10.);
        ASSERT_EQ(get_price_levels.bids[1].second.volume, 25);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, GetPriceLevelAll) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        an_order_book.add_order(1, test_ns::side_t::buy, 20, 10.);
        an_order_book.add_order(2, test_ns::side_t::buy, 40, 12.);
        an_order_book.add_order(3, test_ns::side_t::buy, 5, 10.);
        an_order_book.add_order(4, test_ns::side_t::buy, 10, 12.);

        get_price_levels_t get_price_levels;
        test_ns::get_price_levels_callback_t callback2 =
                std::bind(&get_price_levels_t::func, &get_price_levels,
                        std::placeholders::_1, std::placeholders::_2);
        an_order_book.get_price_levels(std::move(callback2));

        ASSERT_EQ(get_price_levels.sales.size(), 2);
        ASSERT_FALSE(get_price_levels.sales[0].first);
        ASSERT_FALSE(get_price_levels.sales[1].first);

        ASSERT_EQ(get_price_levels.bids.size(), 2);
        ASSERT_TRUE(get_price_levels.bids[0].first);
        ASSERT_EQ(get_price_levels.bids[0].second.price, 12.);
        ASSERT_EQ(get_price_levels.bids[0].second.volume, 50);
        ASSERT_TRUE(get_price_levels.bids[1].first);
        ASSERT_EQ(get_price_levels.bids[1].second.price, 10.);
        ASSERT_EQ(get_price_levels.bids[1].second.volume, 25);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, BBO) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        an_order_book.add_order(1, test_ns::side_t::sell, 20, 10.);
        an_order_book.add_order(2, test_ns::side_t::sell, 40, 12.);
        an_order_book.add_order(3, test_ns::side_t::sell, 5, 10.);
        an_order_book.add_order(4, test_ns::side_t::sell, 10, 12.);
        an_order_book.add_order(5, test_ns::side_t::buy, 20, 10.);
        an_order_book.add_order(6, test_ns::side_t::buy, 40, 12.);
        an_order_book.add_order(7, test_ns::side_t::buy, 5, 10.);
        an_order_book.add_order(8, test_ns::side_t::buy, 10, 12.);

        test_ns::bbo_t bbo;
        an_order_book.get_bbo(&bbo);

        ASSERT_EQ(bbo.buy.first, true);
        ASSERT_EQ(bbo.buy.second.volume, 50);
        ASSERT_EQ(bbo.buy.second.price, 12);

        ASSERT_EQ(bbo.sell.first, true);
        ASSERT_EQ(bbo.sell.second.volume, 25);
        ASSERT_EQ(bbo.sell.second.price, 10);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, BBOBuyOnly) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        an_order_book.add_order(5, test_ns::side_t::buy, 20, 10.);
        an_order_book.add_order(6, test_ns::side_t::buy, 40, 12.);
        an_order_book.add_order(7, test_ns::side_t::buy, 5, 10.);
        an_order_book.add_order(8, test_ns::side_t::buy, 10, 12.);

        test_ns::bbo_t bbo;
        an_order_book.get_bbo(&bbo);

        ASSERT_EQ(bbo.buy.first, true);
        ASSERT_EQ(bbo.buy.second.volume, 50);
        ASSERT_EQ(bbo.buy.second.price, 12);

        ASSERT_EQ(bbo.sell.first, false);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, BBOSellOnly) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        an_order_book.add_order(1, test_ns::side_t::sell, 20, 10.);
        an_order_book.add_order(2, test_ns::side_t::sell, 40, 12.);
        an_order_book.add_order(3, test_ns::side_t::sell, 5, 10.);
        an_order_book.add_order(4, test_ns::side_t::sell, 10, 12.);

        test_ns::bbo_t bbo;
        an_order_book.get_bbo(&bbo);

        ASSERT_EQ(bbo.buy.first, false);

        ASSERT_EQ(bbo.sell.first, true);
        ASSERT_EQ(bbo.sell.second.volume, 25);
        ASSERT_EQ(bbo.sell.second.price, 10);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, ModifyOrder1) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        auto order_1 = an_order_book.get_order(1);
        ASSERT_FALSE(order_1.first);
        an_order_book.add_order(111, test_ns::side_t::buy, 20, 10.0);
        order_1 = an_order_book.get_order(111);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 10.);

        an_order_book.modify_order(111, 30, 40.0);
        order_1 = an_order_book.get_order(111);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 30);
        ASSERT_EQ(order_1.second.price, 40.);

        an_order_book.modify_order(111, 50, 70.0);
        order_1 = an_order_book.get_order(111);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 50);
        ASSERT_EQ(order_1.second.price, 70.);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, ModifyOrder2) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        auto order_1 = an_order_book.get_order(1);
        ASSERT_FALSE(order_1.first);
        an_order_book.add_order(111, test_ns::side_t::sell, 20, 10.0);
        order_1 = an_order_book.get_order(111);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::sell);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 10.);

        an_order_book.modify_order(111, 30, 40.0);
        order_1 = an_order_book.get_order(111);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::sell);
        ASSERT_EQ(order_1.second.quantity, 30);
        ASSERT_EQ(order_1.second.price, 40.);

        an_order_book.modify_order(111, 50, 70.0);
        order_1 = an_order_book.get_order(111);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::sell);
        ASSERT_EQ(order_1.second.quantity, 50);
        ASSERT_EQ(order_1.second.price, 70.);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, CancelOrderBuy) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        auto order_1 = an_order_book.get_order(1);
        ASSERT_FALSE(order_1.first);
        an_order_book.add_order(111, test_ns::side_t::buy, 20, 10.0);
        order_1 = an_order_book.get_order(111);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 10.);

        an_order_book.cancel_order(111);
        order_1 = an_order_book.get_order(111);
        ASSERT_FALSE(order_1.first);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, CancelOrderSell) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        auto order_1 = an_order_book.get_order(1);
        ASSERT_FALSE(order_1.first);
        an_order_book.add_order(111, test_ns::side_t::buy, 20, 10.0);
        order_1 = an_order_book.get_order(111);
        ASSERT_TRUE(order_1.first);
        ASSERT_EQ(order_1.second.side, test_ns::side_t::buy);
        ASSERT_EQ(order_1.second.quantity, 20);
        ASSERT_EQ(order_1.second.price, 10.);

        an_order_book.cancel_order(111);
        order_1 = an_order_book.get_order(111);
        ASSERT_FALSE(order_1.first);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, VWAPBuyOnly) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        test_ns::vwap_t vwap;
        an_order_book.get_vwap(5, &vwap);
        ASSERT_EQ(vwap.buy.valid, false);
        ASSERT_EQ(vwap.sell.valid, false);

        an_order_book.add_order(5, test_ns::side_t::buy, 10, 72.82);
        an_order_book.add_order(6, test_ns::side_t::buy, 100, 72.81);

        an_order_book.get_vwap(5, &vwap);
        ASSERT_EQ(vwap.buy.valid, true);
        ASSERT_EQ(vwap.buy.price, 72.82);
        ASSERT_EQ(vwap.sell.valid, false);

        an_order_book.get_vwap(20, &vwap);
        ASSERT_EQ(vwap.buy.valid, true);
        ASSERT_EQ(vwap.buy.price, 72.815);
        ASSERT_EQ(vwap.sell.valid, false);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, VWAPSellOnly) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        test_ns::vwap_t vwap;
        an_order_book.get_vwap(5, &vwap);
        ASSERT_EQ(vwap.buy.valid, false);
        ASSERT_EQ(vwap.sell.valid, false);

        an_order_book.add_order(6, test_ns::side_t::sell, 10, 100.);
        an_order_book.add_order(5, test_ns::side_t::sell, 20, 200.);

        an_order_book.get_vwap(5, &vwap);
        ASSERT_EQ(vwap.buy.valid, false);
        ASSERT_EQ(vwap.sell.valid, true);
        ASSERT_EQ(vwap.sell.price, 100.);

        an_order_book.get_vwap(20, &vwap);
        ASSERT_EQ(vwap.buy.valid, false);
        ASSERT_EQ(vwap.sell.valid, true);
        ASSERT_EQ(vwap.sell.price, 150.);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, VWAPBuyAndSell) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};

        test_ns::vwap_t vwap;
        an_order_book.get_vwap(5, &vwap);
        ASSERT_EQ(vwap.buy.valid, false);
        ASSERT_EQ(vwap.sell.valid, false);

        an_order_book.add_order(1, test_ns::side_t::buy, 10, 72.82);
        an_order_book.add_order(2, test_ns::side_t::buy, 100, 72.81);
        an_order_book.add_order(3, test_ns::side_t::sell, 10, 100.);
        an_order_book.add_order(4, test_ns::side_t::sell, 20, 200.);

        an_order_book.get_vwap(5, &vwap);
        ASSERT_EQ(vwap.buy.valid, true);
        ASSERT_EQ(vwap.buy.price, 72.82);
        ASSERT_EQ(vwap.sell.valid, true);
        ASSERT_EQ(vwap.sell.price, 100.);

        an_order_book.get_vwap(20, &vwap);
        ASSERT_EQ(vwap.buy.valid, true);
        ASSERT_EQ(vwap.buy.price, 72.815);
        ASSERT_EQ(vwap.sell.valid, true);
        ASSERT_EQ(vwap.sell.price, 150.);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, PrintFullEmpty) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        get_full_orders_t get_full_orders_callback;
        an_order_book.get_full_orders(std::move(get_full_orders_callback));
        ASSERT_EQ(get_full_orders_callback.bids.size(), 0);
        ASSERT_EQ(get_full_orders_callback.asks.size(), 0);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, PrintFullBuy1) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        get_full_orders_t get_full_orders;
        test_ns::get_full_orders_callback_t callback =
                std::bind(&get_full_orders_t::operator(),
                        &get_full_orders,
                        std::placeholders::_1, std::placeholders::_2);

        an_order_book.add_order(1, test_ns::side_t::buy, 100, 10.);

        an_order_book.get_full_orders(std::move(callback));
        ASSERT_EQ(get_full_orders.bids.size(), 1);
        ASSERT_EQ(get_full_orders.asks.size(), 1);

        ASSERT_EQ(get_full_orders.bids[0].valid, true);
        ASSERT_EQ(get_full_orders.bids[0].orders, 1);
        ASSERT_EQ(get_full_orders.bids[0].price, 10.);
        ASSERT_EQ(get_full_orders.bids[0].volume, 100);
        ASSERT_EQ(get_full_orders.asks[0].valid, false);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, PrintFullBuy2) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        get_full_orders_t get_full_orders;
        test_ns::get_full_orders_callback_t callback =
                std::bind(&get_full_orders_t::operator(),
                        &get_full_orders,
                        std::placeholders::_1, std::placeholders::_2);

        an_order_book.add_order(1, test_ns::side_t::buy, 100, 10.);
        an_order_book.add_order(2, test_ns::side_t::buy, 200, 9.);
        an_order_book.add_order(3, test_ns::side_t::buy, 300, 10.);

        an_order_book.get_full_orders(std::move(callback));
        ASSERT_EQ(get_full_orders.bids.size(), 2);
        ASSERT_EQ(get_full_orders.asks.size(), 2);

        ASSERT_EQ(get_full_orders.bids[0].valid, true);
        ASSERT_EQ(get_full_orders.bids[0].orders, 2);
        ASSERT_EQ(get_full_orders.bids[0].price, 10.);
        ASSERT_EQ(get_full_orders.bids[0].volume, 400);
        ASSERT_EQ(get_full_orders.bids[1].valid, true);
        ASSERT_EQ(get_full_orders.bids[1].orders, 1);
        ASSERT_EQ(get_full_orders.bids[1].price, 9.);
        ASSERT_EQ(get_full_orders.bids[1].volume, 200);
        ASSERT_EQ(get_full_orders.asks[0].valid, false);
        ASSERT_EQ(get_full_orders.asks[1].valid, false);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, PrintFullSell1) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        get_full_orders_t get_full_orders;
        test_ns::get_full_orders_callback_t callback =
                std::bind(&get_full_orders_t::operator(),
                        &get_full_orders,
                        std::placeholders::_1, std::placeholders::_2);

        an_order_book.add_order(1, test_ns::side_t::sell, 100, 10.);

        an_order_book.get_full_orders(std::move(callback));
        ASSERT_EQ(get_full_orders.bids.size(), 1);
        ASSERT_EQ(get_full_orders.asks.size(), 1);

        ASSERT_EQ(get_full_orders.asks[0].valid, true);
        ASSERT_EQ(get_full_orders.asks[0].orders, 1);
        ASSERT_EQ(get_full_orders.asks[0].price, 10.);
        ASSERT_EQ(get_full_orders.asks[0].volume, 100);
        ASSERT_EQ(get_full_orders.bids[0].valid, false);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, PrintFullSell2) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        get_full_orders_t get_full_orders;
        test_ns::get_full_orders_callback_t callback =
                std::bind(&get_full_orders_t::operator(),
                        &get_full_orders,
                        std::placeholders::_1, std::placeholders::_2);

        an_order_book.add_order(1, test_ns::side_t::sell, 100, 10.);
        an_order_book.add_order(2, test_ns::side_t::sell, 200, 11.);
        an_order_book.add_order(3, test_ns::side_t::sell, 300, 10.);
        an_order_book.add_order(4, test_ns::side_t::buy, 100, 10.);
        an_order_book.add_order(5, test_ns::side_t::buy, 200, 9.);
        an_order_book.add_order(6, test_ns::side_t::buy, 300, 10.);

        an_order_book.get_full_orders(std::move(callback));
        ASSERT_EQ(get_full_orders.bids.size(), 2);
        ASSERT_EQ(get_full_orders.asks.size(), 2);

        ASSERT_EQ(get_full_orders.asks[0].valid, true);
        ASSERT_EQ(get_full_orders.asks[0].orders, 2);
        ASSERT_EQ(get_full_orders.asks[0].price, 10.);
        ASSERT_EQ(get_full_orders.asks[0].volume, 400);
        ASSERT_EQ(get_full_orders.asks[1].valid, true);
        ASSERT_EQ(get_full_orders.asks[1].orders, 1);
        ASSERT_EQ(get_full_orders.asks[1].price, 11.);
        ASSERT_EQ(get_full_orders.asks[1].volume, 200);
        ASSERT_EQ(get_full_orders.bids[0].valid, true);
        ASSERT_EQ(get_full_orders.bids[0].orders, 2);
        ASSERT_EQ(get_full_orders.bids[0].price, 10.);
        ASSERT_EQ(get_full_orders.bids[0].volume, 400);
        ASSERT_EQ(get_full_orders.bids[1].valid, true);
        ASSERT_EQ(get_full_orders.bids[1].orders, 1);
        ASSERT_EQ(get_full_orders.bids[1].price, 9.);
        ASSERT_EQ(get_full_orders.bids[1].volume, 200);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

TEST(OrderBook, PrintFullBidsSells) {
    try {
        test_ns::order_book an_order_book{test_symbol_1};
        get_full_orders_t get_full_orders;
        test_ns::get_full_orders_callback_t callback =
                std::bind(&get_full_orders_t::operator(),
                        &get_full_orders,
                        std::placeholders::_1, std::placeholders::_2);

        an_order_book.add_order(1, test_ns::side_t::sell, 100, 10.);
        an_order_book.add_order(2, test_ns::side_t::sell, 200, 11.);
        an_order_book.add_order(3, test_ns::side_t::sell, 300, 10.);

        an_order_book.get_full_orders(std::move(callback));
        ASSERT_EQ(get_full_orders.bids.size(), 2);
        ASSERT_EQ(get_full_orders.asks.size(), 2);

        ASSERT_EQ(get_full_orders.asks[0].valid, true);
        ASSERT_EQ(get_full_orders.asks[0].orders, 2);
        ASSERT_EQ(get_full_orders.asks[0].price, 10.);
        ASSERT_EQ(get_full_orders.asks[0].volume, 400);
        ASSERT_EQ(get_full_orders.asks[1].valid, true);
        ASSERT_EQ(get_full_orders.asks[1].orders, 1);
        ASSERT_EQ(get_full_orders.asks[1].price, 11.);
        ASSERT_EQ(get_full_orders.asks[1].volume, 200);
        ASSERT_EQ(get_full_orders.bids[0].valid, false);
        ASSERT_EQ(get_full_orders.bids[1].valid, false);
    } catch (std::exception& e) {
        FAIL() << e.what();
    }
}

/*
 *
 */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

