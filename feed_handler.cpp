#include "feed_handler.h"

#include <iostream>
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cassert>
#include <iomanip>
/*
 *
 */
test_ns::
feed_handler::feed_handler(const std::string& selected_symbol,
        callback_t&& a_callback, err_callback_t&& an_err_callback) :
        callback(std::move(a_callback)),
        err_callback(std::move(an_err_callback)),
        selected_symbol(selected_symbol) {
}

/*
 *
 */

static
std::unordered_map<std::string, test_ns::command_t> string_to_cmd = {
        { "ORDER ADD", test_ns::command_t::order_add },
        { "ORDER MODIFY", test_ns::command_t::order_modify },
        { "ORDER CANCEL", test_ns::command_t::order_cancel},
        { "SUBSCRIBE BBO", test_ns::command_t::subs_bbo },
        { "UNSUBSCRIBE BBO", test_ns::command_t::unsubs_bbo },
        { "SUBSCRIBE VWAP", test_ns::command_t::subs_vwap },
        { "UNSUBSCRIBE VWAP", test_ns::command_t::unsubs_vwap },
        { "PRINT", test_ns::command_t::print },
        { "PRINT_FULL", test_ns::command_t::print_full }
};

/*
 *
 */
test_ns::command_t test_ns::
feed_handler::parse_command(const std::string& line) {
    if (line.empty())
        return command_t::none;
    std::string::size_type position = line.find(',');
    auto itr = string_to_cmd.find(line.substr(0, position));
    if (itr == string_to_cmd.end()) {
        return command_t::none;
    }
    return itr->second;
}

/*
 *
 */
void test_ns::
feed_handler::process_command(const std::string& line) {
    command_t command = parse_command(line);
    if (command == command_t::none) {
        err_callback(line, "incorrect command");
        return;
    }
    switch (command) {
    case command_t::order_add:
        process_order_add(line);
        break;
    case command_t::order_modify:
        process_order_modify(line);
        break;
    case command_t::order_cancel:
        process_order_cancel(line);
        break;
    case command_t::subs_bbo:
        process_subs_bbo(line);
        break;
    case command_t::unsubs_bbo:
        process_unsubs_bbo(line);
        break;
    case command_t::subs_vwap:
        process_subs_vwap(line);
        break;
    case command_t::unsubs_vwap:
        process_unsubs_vwap(line);
        break;
    case command_t::print:
        print(line);
        break;
    case command_t::print_full:
        print_full(line);
        break;
    default:
        err_callback(line, "not implemented");
        break;
    }
    if (!bbo_subs.empty()) {
        print_bbo_subs();
    }
    if (!vwap_subs.empty()) {
        print_vwap_subs();
    }
}

/*
 *
 */
void test_ns::
feed_handler::process_order_add(const std::string& line) {
    args_t args;
    if (!parse_args(line, 5, &args)) {
        err_callback(line, "invalid number of parameters");
        return;
    }
    order_id_t id;
    if (!str_to_order_id(args[0], &id)) {
        err_callback(line, "invalid order id");
        return;
    }

    symbol_t symbol;
    if (!str_to_symbol(args[1], &symbol)) {
        err_callback(line, "invalid symbol");
        return;
    }
    if (!should_handle_symbol(symbol)) {
        return;
    }

    side_t side;
    if (!str_to_side(args[2], &side)) {
        err_callback(line, "invalid side");
        return;
    }

    quantity_t quantity;
    if (!str_to_quantity(args[3], &quantity)) {
        err_callback(line, "invalid quantity");
        return;
    }

    double price;
    if (!str_to_price(args[4], &price)) {
        err_callback(line, "invalid price");
        return;
    }
    auto & an_order_book = get_order_book(symbol);
    try {
        an_order_book.add_order(id, side, quantity, price);
        order_id_symbols[id] = symbol;
    } catch (std::exception& e) {
        err_callback(line, std::string("failed to add: ") + e.what());
        return;
    }
}

/*
 *
 */
void test_ns::
feed_handler::process_order_modify(const std::string& line) {
    args_t args;
    if (!parse_args(line, 3, &args)) {
        err_callback(line, "invalid number of parameters");
        return;
    }
    order_id_t id;
    if (!str_to_order_id(args[0], &id)) {
        err_callback(line, "invalid order id");
        return;
    }

    quantity_t quantity;
    if (!str_to_quantity(args[1], &quantity)) {
        err_callback(line, "invalid quantity");
        return;
    }

    double price;
    if (!str_to_price(args[2], &price)) {
        err_callback(line, "invalid price");
        return;
    }
    if (!is_there_order_book(id)) {
        std::ostringstream ss;
        ss << "failed to modify order: " << id;
        err_callback(line, ss.str());
        return;
    }

    auto & an_order_book = get_order_book_ref(id);
    try {
        an_order_book.modify_order(id, quantity, price);
    } catch (std::exception& e) {
        err_callback(line, std::string("failed to modify: ") + e.what());
        return;
    }
}

/*
 *
 */
void test_ns::
feed_handler::process_order_cancel(const std::string& line) {
    args_t args;
    if (!parse_args(line, 1, &args)) {
        err_callback(line, "invalid number of parameters");
        return;
    }
    order_id_t id;
    if (!str_to_order_id(args[0], &id)) {
        err_callback(line, "invalid order id");
        return;
    }

    if (!is_there_order_book(id)) {
        std::ostringstream ss;
        ss << "failed to cancel order: " << id;
        err_callback(line, ss.str());
        return;
    }

    auto & an_order_book = get_order_book_ref(id);
    try {
        an_order_book.cancel_order(id);
        order_id_symbols.erase(id);
    } catch (std::exception& e) {
        err_callback(line, std::string("failed to modify: ") + e.what());
        return;
    }
}

/*
 *
 */
void test_ns::
feed_handler::process_subs_bbo(const std::string& line) {
    args_t args;
    auto res = parse_args(line, 1, &args);
    if (!res) {
        err_callback(line, "invalid number of parameters");
        return;
    }

    symbol_t symbol;
    if (!str_to_symbol(args[0], &symbol)) {
        err_callback(line, "invalid symbol");
        return;
    }

    if (!should_handle_symbol(symbol)) {
        return;
    }
    ++bbo_subs[symbol];
}

/*
 *
 */
void test_ns::
feed_handler::process_unsubs_bbo(const std::string& line) {
    args_t args;
    auto res = parse_args(line, 1, &args);
    if (!res) {
        err_callback(line, "invalid number of parameters");
        return;
    }

    symbol_t symbol;
    if (!str_to_symbol(args[0], &symbol)) {
        err_callback(line, "invalid symbol");
        return;
    }

    decrement_bbo(symbol);
}

/*
 *
 */
void test_ns::
feed_handler::process_subs_vwap(const std::string& line) {
    args_t args;
    auto res = parse_args(line, 2, &args);
    if (!res) {
        err_callback(line, "invalid number of parameters");
        return;
    }

    symbol_t symbol;
    if (!str_to_symbol(args[0], &symbol)) {
        err_callback(line, "invalid symbol");
        return;
    }

    quantity_t quantity;
    if (!str_to_quantity(args[1], &quantity)) {
        err_callback(line, "invalid quantity");
        return;
    }

    if (!should_handle_symbol(symbol)) {
        return;
    }

    ++vwap_subs[std::make_pair(symbol, quantity)];
}

/*
 *
 */
void test_ns::
feed_handler::process_unsubs_vwap(const std::string& line) {
    args_t args;
    auto res = parse_args(line, 2, &args);
    if (!res) {
        err_callback(line, "invalid number of parameters");
        return;
    }

    symbol_t symbol;
    if (!str_to_symbol(args[0], &symbol)) {
        err_callback(line, "invalid symbol");
        return;
    }

    quantity_t quantity;
    if (!str_to_quantity(args[1], &quantity)) {
        err_callback(line, "invalid quantity");
        return;
    }

    auto itr = vwap_subs.find(std::make_pair(symbol, quantity));
    if (itr != vwap_subs.end()) {
        if (itr->second <= 1) {
            vwap_subs.erase(itr);
        } else {
            --itr->second;
        }
    }
}

/*
 *
 */
unsigned test_ns::
feed_handler::get_total_number_bbo_subs() const {
    return bbo_subs.size();
}

/*
 *
 */
unsigned test_ns::
feed_handler::get_bbo_subs_number(const symbol_t& s) const {
    auto itr = bbo_subs.find(s);
    if (itr != bbo_subs.end()) {
        return itr->second;
    } else {
        return 0;
    }
}

/*
 *
 */
unsigned test_ns::
feed_handler::get_total_number_vwap_subs() const {
    return vwap_subs.size();
}

/*
 *
 */
unsigned test_ns::
feed_handler::get_vwap_subs_number(const symbol_t& s, quantity_t q) const {
    auto itr = vwap_subs.find(std::make_pair(s, q));
    if (itr != vwap_subs.end()) {
        return itr->second;
    } else {
        return 0;
    }
}

/*
 *
 */
void test_ns::
feed_handler::decrement_bbo(const symbol_t& s) {
    auto itr = bbo_subs.find(s);
    if (itr != bbo_subs.end()) {
        if (itr->second <= 1) {
            bbo_subs.erase(itr);
        } else {
            --itr->second;
        }
    }
}

/*
 *
 */
void test_ns::
feed_handler::print_bbo_subs() const {
    std::ostringstream ss, field1, field2;
    for (auto const & sym_and_ref : bbo_subs) {
        auto const & symbol = sym_and_ref.first;
        if (!is_there_order_book(symbol)) {
            continue;
        }
        auto const & order_book = get_order_book_ref(symbol);
        bbo_t bbo;
        order_book.get_bbo(&bbo);

        ss.str("");
        field1.str("");
        field2.str("");

        ss << "BBO: " << std::left << std::setw(10) << symbol;
        ss << std::left << std::setw(20);
        if (bbo.buy.first) {
            field1 << bbo.buy.second.volume << '@' << bbo.buy.second.price;
            ss << field1.str();
        } else {
            ss << ' ';
        }
        ss << " | ";
        ss << std::left << std::setw(20);
        if (bbo.sell.first) {
            field2 << bbo.sell.second.volume << '@' << bbo.sell.second.price;
            ss << field2.str();
        } else {
            ss << ' ';
        }
        callback(ss.str());
    }
}

/*
 *
 */
void test_ns::
feed_handler::print_vwap_subs() const {
    std::ostringstream ss;
    for (auto const & vwap_and_ref : vwap_subs) {
        auto const & symbol = vwap_and_ref.first.first;
        auto const & quantity = vwap_and_ref.first.second;
        ss.str("");

        if (!is_there_order_book(symbol)) {
            ss << "VWAP: " << std::left << std::setw(10) << symbol
               << " <NIL,NIL>";
            callback(ss.str());
            continue;
        }
        auto const & order_book = get_order_book_ref(symbol);
        vwap_t vwap;
        order_book.get_vwap(quantity, &vwap);

        ss << "VWAP: " << std::left << std::setw(10) << symbol << " <";
        if (vwap.buy.valid) {
            ss << vwap.buy.price;
        } else {
            ss << "NIL";
        }
        ss << ",";
        if (vwap.sell.valid) {
            ss << vwap.sell.price;
        } else {
            ss << "NIL";
        }
        ss << ">";
        callback(ss.str());
    }
}

/*
 *
 */
void test_ns::
feed_handler::print(const symbol_t& line) const {
    args_t args;
    if (!parse_args(line, 1, &args)) {
        err_callback(line, "invalid number of parameters");
        return;
    }
    symbol_t symbol;
    if (!str_to_symbol(args[0], &symbol)) {
        err_callback(line, "invalid symbol");
        return;
    }
    if (!should_handle_symbol(symbol)) {
        return;
    }
    if (!is_there_order_book(symbol)) {
        return;
    }
    auto const & order_book = get_order_book_ref(symbol);
    auto print_order_book_price_levels = [&]
        (const price_level_t& bid, const price_level_t& ask) {
        std::ostringstream ss, ss_field1, ss_field2;
        ss << std::left << std::setw(20);
        if (bid.first) {
            ss_field1 << bid.second.volume << '@' << bid.second.price;
            ss << ss_field1.str();
        } else {
            ss << ' ';
        }
        ss << " | ";
        ss << std::left << std::setw(20);
        if (ask.first) {
            ss_field2 << ask.second.volume << '@' << ask.second.price;
            ss << ss_field2.str();
        } else {
            ss << ' ';
        }
        callback(ss.str());
    };
    order_book.get_price_levels(std::move(print_order_book_price_levels));
}


/*
 *
 */
void test_ns::
feed_handler::print_full(const symbol_t& line) const {
    args_t args;
    if (!parse_args(line, 1, &args)) {
        err_callback(line, "invalid number of parameters");
        return;
    }
    symbol_t symbol;
    if (!str_to_symbol(args[0], &symbol)) {
        err_callback(line, "invalid symbol");
        return;
    }
    if (!should_handle_symbol(symbol)) {
        return;
    }
    if (!is_there_order_book(symbol)) {
        return;
    }
    auto const & order_book = get_order_book_ref(symbol);
    std::ostringstream ss;
    auto format_column = [&ss]() {
        ss << std::left << std::setw(10);
    };
    auto format_string = [&ss, &format_column] (const std::string& s) {
        format_column();
        ss << s;
    };

    auto format_number = [&ss, &format_column] (unsigned value) {
        format_column();
        ss << value;
    };

    auto format_quantity = [&ss, &format_column] (quantity_t value) {
        format_column();
        ss << std::left << std::setw(10) << value;
    };

    auto format_double = [&ss, &format_column] (double value) {
        format_column();
        ss << value;
    };

    auto format_spaces = [&ss, &format_column] (unsigned times) {
        for (auto i = 0U; i < times; ++i) {
            format_column();
            ss << ' ';
        }
    };

    auto separate_line = [&ss] () {
        ss << std::setfill('-') << std::setw(60) << '-';
        ss << std::setfill(' ');
    };

    auto print_string = [&ss, this] () {
        this->callback(ss.str());
        ss.str("");
    };

    separate_line();
    print_string();

    format_string("orders");
    format_string("volume");
    format_string("bid");
    format_string("ask");
    format_string("volume");
    format_string("orders");
    print_string();

    separate_line();
    print_string();

    auto print_orders = [&format_number, &format_quantity, &format_double,
                         &format_spaces, &print_string]
        (const full_orders_t& bid, const full_orders_t& ask) {
        if (bid.valid) {
            format_number(bid.orders);
            format_quantity(bid.volume);
            format_double(bid.price);
        } else {
            format_spaces(3);
        }
        if (ask.valid) {
            format_number(ask.orders);
            format_quantity(ask.volume);
            format_double(ask.price);
        } else {
            format_spaces(3);
        }
        print_string();
    };
    order_book.get_full_orders(std::move(print_orders));
    separate_line();
    print_string();
}

/*
 *
 */
bool test_ns::
feed_handler::parse_args(const std::string& line,
        unsigned number_args, args_t* args) const {
    args->clear();
    std::istringstream ss(line);
    std::string token;
    bool command_skipped = false;
    unsigned parsed_args = 0;
    while (std::getline(ss, token, ',') && parsed_args <= number_args) {
        if (!command_skipped) {
            command_skipped = true;
            continue;
        } else {
            ++parsed_args;
            args->push_back(token);
        }
    }
    if (!command_skipped) {
        return false;
    }
    if (ss.eof() && parsed_args == number_args) {
        return true;
    }
    return false;
}


/*
 *
 */
bool test_ns::
feed_handler::str_to_order_id(const std::string& token, order_id_t* id) {
    std::istringstream ss(token);
    ss >> *id;
    return static_cast<bool>(ss);
}

/*
 *
 */
bool test_ns::
feed_handler::str_to_symbol(const std::string& token, symbol_t* symbol) {
    if (token.empty())
        return false;
    *symbol = token;
    return true;
}

/*
 *
 */
bool test_ns::
feed_handler::str_to_side(const std::string& token, side_t* side) {
    if ( token == "Buy" ) {
        *side = side_t::buy;
        return true;
    }
    if ( token == "Sell" ) {
        *side = side_t::sell;
        return true;
    }
    return false;
}

/*
 *
 */
bool test_ns::
feed_handler::str_to_quantity(const std::string& token, quantity_t* quantity) {
    std::istringstream ss(token);
    ss >> *quantity;
    return static_cast<bool>(ss);
}

/*
 *
 */
bool test_ns::
feed_handler::str_to_price(const std::string& token, double* price) {
    std::istringstream ss(token);
    ss >> *price;
    return static_cast<bool>(ss);
}

/*
 *
 */
bool test_ns::
feed_handler::is_there_selected_symbol() const {
    return !selected_symbol.empty();
}

/*
 *
 */
bool test_ns::
feed_handler::should_handle_symbol(const std::string& symbol) const {
    if (selected_symbol.empty()) {
        return true;
    } else {
        return selected_symbol == symbol;
    }
}

/*
 *
 */
test_ns::order_book& test_ns::
feed_handler::get_order_book(const std::string& symbol) {
    auto itr = order_books.find(symbol);
    if (itr == order_books.end()) {
        auto insert_res = order_books.insert(std::make_pair(
                symbol, order_book(symbol)));
        return insert_res.first->second;

    } else {
        return itr->second;
    }
}

/*
 *
 */
bool test_ns::
feed_handler::is_there_order_book(const std::string& symbol) const {
    return order_books.find(symbol) != order_books.end();
}

/*
 *
 */
bool test_ns::
feed_handler::is_there_order_book(const order_id_t& id) const {
    auto itr = order_id_symbols.find(id);
    if (itr != order_id_symbols.end()) {
        return is_there_order_book(itr->second);
    } else {
        return false;
    }
}

/*
 *
 */
test_ns::order_book& test_ns::
feed_handler::get_order_book_ref(const order_id_t& id) {
    auto itr = order_id_symbols.find(id);
    if (itr != order_id_symbols.end()) {
        return get_order_book_ref(itr->second);
    } else {
        std::ostringstream ss;
        ss << "No order book for " << id;
        throw std::runtime_error(ss.str());
    }
}

/*
 *
 */
const test_ns::order_book& test_ns::
feed_handler::get_order_book_ref(const order_id_t& id) const {
    auto itr = order_id_symbols.find(id);
    if (itr != order_id_symbols.end()) {
        return get_order_book_ref(itr->second);
    } else {
        std::ostringstream ss;
        ss << "No order book for " << id;
        throw std::runtime_error(ss.str());
    }
}

/*
 *
 */
test_ns::order_book& test_ns::
feed_handler::get_order_book_ref(const std::string& symbol) {
    auto itr = order_books.find(symbol);
    if (itr == order_books.end()) {
        throw std::runtime_error("No order book for " + symbol);
    } else {
        return itr->second;
    }
}

/*
 *
 */
const test_ns::order_book& test_ns::
feed_handler::get_order_book_ref(const std::string& symbol) const {
    auto itr = order_books.find(symbol);
    if (itr == order_books.end()) {
        throw std::runtime_error("No order book for " + symbol);
    } else {
        return itr->second;
    }
}

/*
 *
 */
test_ns::symbol_t test_ns::
feed_handler::get_selected_symbol() const {
    return selected_symbol;
}

/*
 *
 */
test_ns::optional_order
test_ns::feed_handler::get_order(const symbol_t& symbol,
        order_id_t id) const {
    if (!should_handle_symbol(symbol)) {
        return test_ns::optional_order{false, order_t()};
    }
    if (!is_there_order_book(id)) {
        return test_ns::optional_order{false, order_t()};
    }

    auto & an_order_book = get_order_book_ref(symbol);
    return an_order_book.get_order(id);
}

/*
 *
 */
bool test_ns::
feed_handler::is_there_symbol_for_order(order_id_t id) const {
    return order_id_symbols.find(id) != order_id_symbols.end();
}

/*
 *
 */
test_ns::symbol_t test_ns::
feed_handler::get_symbol_for_order(order_id_t id) const {
    auto itr = order_id_symbols.find(id);
    if (itr != order_id_symbols.end()) {
        return itr->second;
    } else {
        return "";
    }
}


/*
 *
 */
void test_ns::print_to_stdout(const std::string& s) {
    std::cout << s << '\n';
}

/*
 *
 */
void test_ns::print_to_stderr(
        const std::string& line, const std::string& err) {
    std::cerr << "error: " << err << "line: " << line << '\n';
}

/*
 *
 */
test_ns::order_book::order_book(const symbol_t& symbol)
    : symbol(symbol) {
}

/*
 *
 */
const test_ns::symbol_t& test_ns::order_book::get_symbol() const {
    return symbol;
}

/*
 *
 */
void test_ns::order_book::add_order(order_id_t id, side_t side,
        quantity_t quantity, double price) {
    auto itr = orders.find(id);
    if (itr == orders.end()) {
        orders[id] = {id, quantity, price, side};
        if (side == side_t::buy) {
            add_bid(price, id);
        } else {
            add_sale(price, id);
        }
    } else {
        std::ostringstream s;
        s << "This order already exist: " << id;
        throw std::runtime_error(s.str());
    }
}

/*
 *
 */
void test_ns::order_book::modify_order(order_id_t id, quantity_t quantity,
        double price) {
    auto itr = orders.find(id);
    if (itr == orders.end()) {
        std::ostringstream s;
        s << "This order does not exist: " << id;
        throw std::runtime_error(s.str());
    } else {
        auto & an_order = itr->second;
        if (an_order.side == side_t::buy) {
            remove_bid(an_order.price, id);
        } else {
            remove_sale(an_order.price, id);
        }
        an_order.quantity = quantity;
        an_order.price = price;
        if (an_order.side == side_t::buy) {
            add_bid(price, id);
        } else {
            add_sale(price, id);
        }
    }
}

/*
 *
 */
void test_ns::order_book::cancel_order(order_id_t id) {
    auto itr = orders.find(id);
    if (itr == orders.end()) {
        std::ostringstream s;
        s << "This order does not exist: " << id;
        throw std::runtime_error(s.str());
    } else {
        auto & an_order = itr->second;
        if (an_order.side == side_t::buy) {
            remove_bid(an_order.price, id);
        } else {
            remove_sale(an_order.price, id);
        }
        orders.erase(itr);
    }
}

/*
 *
 */
void test_ns::order_book::add_bid(double price, order_id_t id) {
    auto itr = bids.find(price);
    if (itr == bids.end()) {
        auto res = bids.insert(std::make_pair(price, order_ids_t()));
        itr = res.first;
    }
    auto & order_ids = itr->second;
    order_ids.insert(id);
}

/*
 *
 */
void test_ns::order_book::remove_bid(double price, order_id_t id) {
    auto itr = bids.find(price);
    assert(itr != bids.end());
    if (itr != bids.end()) {
        auto & order_ids = itr->second;
        order_ids.erase(id);
        if (order_ids.empty()) {
            bids.erase(itr);
        }
    }
}

/*
 *
 */
void test_ns::order_book::add_sale(double price, order_id_t id) {
    auto itr = sales.find(price);
    if (itr == sales.end()) {
        auto res = sales.insert(std::make_pair(price, order_ids_t()));
        itr = res.first;
    }
    auto & order_ids = itr->second;
    order_ids.insert(id);
}

/*
 *
 */
void test_ns::order_book::remove_sale(double price, order_id_t id) {
    auto itr = sales.find(price);
    assert(itr != sales.end());
    if (itr != sales.end()) {
        auto & order_ids = itr->second;
        order_ids.erase(id);
        if (order_ids.empty()) {
            sales.erase(itr);
        }
    }
}

/*
 *
 */
test_ns::optional_order test_ns::order_book::get_order(
        order_id_t id) const {
    auto itr = orders.find(id);
    if (itr == orders.end()) {
        return std::make_pair(false, order_t());
    } else {
        return std::make_pair(true, itr->second);
    }
}

/*
 *
 */
void test_ns::order_book::get_price_levels(
        get_price_levels_callback_t&& callback) const {
    auto bids_itr = bids.begin();
    auto sales_itr = sales.begin();
    while (bids_itr != bids.end() && sales_itr != sales.end()) {
        volume_price_t bid_volume_price_level =
                get_volume_price(bids_itr->first, bids_itr->second);
        volume_price_t sale_volume_price_level =
                get_volume_price(sales_itr->first, sales_itr->second);
        callback(std::make_pair(true, bid_volume_price_level),
                std::make_pair(true, sale_volume_price_level));
        ++bids_itr;
        ++sales_itr;
    }

    while (bids_itr != bids.end()) {
        volume_price_t bid_volume_price_level =
                get_volume_price(bids_itr->first, bids_itr->second);
        volume_price_t sale_volume_price_level;
        callback(std::make_pair(true, bid_volume_price_level),
                std::make_pair(false, sale_volume_price_level));
        ++bids_itr;
    }

    while (sales_itr != sales.end()) {
        volume_price_t bid_volume_price_level;
        volume_price_t sale_volume_price_level =
                get_volume_price(sales_itr->first, sales_itr->second);
        callback(std::make_pair(false, bid_volume_price_level),
                std::make_pair(true, sale_volume_price_level));
        ++sales_itr;
    }
}

/*
 *
 */
test_ns::volume_price_t
test_ns::
order_book::get_volume_price(double price, const order_ids_t& order_ids) const {
    quantity_t total = 0;
    for (auto const & id : order_ids) {
        auto itr = orders.find(id);
        assert(itr != orders.end());
        if (itr != orders.end()) {
            total += itr->second.quantity;
        }
    }
    return test_ns::volume_price_t{total, price};
}

test_ns::full_orders_t
test_ns::
order_book::get_line_full_orders(double price,
        const order_ids_t& order_ids) const {
    quantity_t total = 0;
    for (auto const & id : order_ids) {
        auto itr = orders.find(id);
        assert(itr != orders.end());
        if (itr != orders.end()) {
            total += itr->second.quantity;
        }
    }
    return test_ns::full_orders_t{true, order_ids.size(), total, price};
}

/*
 *
 */
void test_ns::
order_book::get_full_orders(get_full_orders_callback_t&& callback) const {
    auto bids_itr = bids.begin();
    auto sales_itr = sales.begin();
    while (bids_itr != bids.end() && sales_itr != sales.end()) {
        full_orders_t bid = get_line_full_orders(bids_itr->first,
                bids_itr->second);
        full_orders_t ask = get_line_full_orders(sales_itr->first,
                sales_itr->second);
        callback(bid, ask);
        ++bids_itr;
        ++sales_itr;
    }
    while (bids_itr != bids.end()) {
        full_orders_t bid = get_line_full_orders(bids_itr->first,
                bids_itr->second);
        full_orders_t ask {false, 0, 0, 0};
        callback(bid, ask);
        ++bids_itr;
    }
    while (sales_itr != sales.end()) {
        full_orders_t bid = {false, 0, 0, 0};
        full_orders_t ask = get_line_full_orders(sales_itr->first,
                sales_itr->second);
        callback(bid, ask);
        ++sales_itr;
    }
}

/*
 *
 */
void test_ns::
order_book::get_bbo(bbo_t* bbo) const {
    if (bids.begin() == bids.end()) {
        bbo->buy = std::make_pair(false, volume_price_t());
    } else {
        auto bids_itr = bids.begin();
        bbo->buy = std::make_pair(true,
                get_volume_price(bids_itr->first, bids_itr->second));
    }

    if (sales.begin() == sales.end()) {
        bbo->sell = std::make_pair(false, volume_price_t());
    } else {
        auto sell_itr = sales.begin();
        bbo->sell = std::make_pair(true,
                get_volume_price(sell_itr->first, sell_itr->second));
    }
}

/*
 *
 */
void test_ns::
order_book::get_vwap(quantity_t quantity, vwap_t* vwap) const {
    vwap->quantity = quantity;
    if (bids.begin() == bids.end()) {
        vwap->buy = {false, 0.};
    } else {
        quantity_t found_quantity = 0;
        double found_cost  = 0;
        for (auto & a_group : bids) {
            double current_price = a_group.first;
            auto & order_ids = a_group.second;
            for (auto order_id : order_ids)  {
                auto itr = orders.find(order_id);
                assert(itr != orders.end());
                auto & order = itr->second;
                assert(current_price == order.price);
                assert(side_t::buy == order.side);
                found_quantity += order.quantity;
                if (found_quantity >= quantity) {
                    auto diff = found_quantity - quantity;
                    found_cost += (order.quantity-diff) * order.price;
                    break;
                } else {
                    found_cost += order.quantity * order.price;
                }
            }
            if (found_quantity >= quantity) {
                break;
            }
        }
        if (found_quantity >= quantity) {
            double a_price = found_cost / quantity;
            vwap->buy = {true, a_price};
        } else {
            vwap->buy = {false, 0.};
        }
    }
    if (sales.begin() == sales.end()) {
        vwap->sell = {false, 0.};
    } else {
        quantity_t found_quantity = 0;
        double found_cost  = 0;
        for (auto & a_group : sales) {
            double current_price = a_group.first;
            auto & order_ids = a_group.second;
            for (auto order_id : order_ids)  {
                auto itr = orders.find(order_id);
                assert(itr != orders.end());
                auto & order = itr->second;
                assert(side_t::sell == order.side);
                assert(current_price == order.price);
                found_quantity += order.quantity;
                if (found_quantity >= quantity) {
                    auto diff = found_quantity - quantity;
                    found_cost += (order.quantity-diff) * order.price;
                    break;
                } else {
                    found_cost += order.quantity * order.price;
                }
            }
            if (found_quantity >= quantity) {
                break;
            }
        }
        if (found_quantity >= quantity) {
            double a_price = found_cost / quantity;
            vwap->sell = {true, a_price};
        } else {
            vwap->buy = {false, 0. };
        }
    }
}

