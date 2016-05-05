#ifndef FEED_HANDLER_H
#define FEED_HANDLER_H

#include <string>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include <utility>

namespace tbricks_test {

/*
 *
 */
enum class side_t {
    buy,
    sell
};

/*
 *
 */
enum class command_t {
    none,
    order_add,
    order_modify,
    order_cancel,
    subs_bbo,
    unsubs_bbo,
    subs_vwap,
    unsubs_vwap,
    print,
    print_full
};

/*
 *
 */
using order_id_t = uint64_t;

/*
 *
 */
using quantity_t = uint64_t;

/*
 *
 */
using symbol_t = std::string;

/*
 *
 */
struct order_t {
    order_id_t id;
    quantity_t quantity;
    double price;
    side_t side;
};
using optional_order = std::pair<bool, order_t>;

/*
 *
 */
struct volume_price_t {
    quantity_t volume;
    double price;
};
using price_level_t = std::pair<bool, volume_price_t>;
using get_price_levels_callback_t =
        std::function<void(const price_level_t&, const price_level_t&)>;

using get_orders_callback_t =
        std::function<void()>;

struct bbo_t {
    price_level_t buy;
    price_level_t sell;
};

/*
 *
 */
class order_book {
 public:
    explicit order_book(const symbol_t& symbol);
    const symbol_t& get_symbol() const;
    void add_order(order_id_t id, side_t, quantity_t, double);
    optional_order get_order(order_id_t id) const;
    void modify_order(order_id_t id, quantity_t, double);
    void cancel_order(order_id_t id);
    void get_price_levels(get_price_levels_callback_t&&) const;
    void get_orders(get_orders_callback_t&&) const;
    void get_bbo(bbo_t*) const;
 private:
    using orders_t = std::unordered_map<order_id_t, order_t>;
    using order_ids_t = std::set<order_id_t>;
    using bids_t = std::map<double, order_ids_t, std::greater<double>>;
    using sales_t = std::map<double, order_ids_t>;
    symbol_t symbol;
    orders_t orders;
    bids_t bids;
    sales_t sales;
    void add_bid(double, order_id_t);
    void add_sale(double, order_id_t);
    void remove_bid(double, order_id_t);
    void remove_sale(double, order_id_t);
    volume_price_t get_volume_price(double price, const order_ids_t&) const;
};

using callback_t =
        std::function<void(const std::string&)>;
using err_callback_t =
        std::function<void(const std::string&, const std::string&)>;

/*
 *
 */
class feed_handler {
 public:
    feed_handler(const symbol_t& selected_symbol,
            callback_t&&, err_callback_t&&);
    void process_command(const std::string&);

    bool is_there_selected_symbol() const;
    symbol_t get_selected_symbol() const;
    unsigned number_order_books() const;
    optional_order get_order(const symbol_t& symbol, order_id_t id) const;
    bool is_there_symbol_for_order(order_id_t id) const;
    symbol_t get_symbol_for_order(order_id_t id) const;
    unsigned get_total_number_bbo_subs() const;
    unsigned get_bbo_subs_number(const symbol_t&) const;

 private:
    using order_books_t = std::unordered_map<symbol_t, order_book>;
    using order_id_symbols_t = std::unordered_map<order_id_t, symbol_t>;
    using bbo_subs_t = std::unordered_map<symbol_t, int>;
    using vwap_key_t = std::pair<symbol_t, quantity_t>;
    using args_t = std::vector<std::string>;
    callback_t callback;
    err_callback_t err_callback;
    symbol_t selected_symbol;
    order_books_t order_books;
    order_id_symbols_t order_id_symbols;
    bbo_subs_t bbo_subs;
    command_t parse_command(const std::string& line);
    bool parse_args(const std::string& line, unsigned number, args_t*) const;
    void process_order_add(const std::string& line);
    void process_order_modify(const std::string& line);
    void process_order_cancel(const std::string& line);
    void process_subs_bbo(const std::string& line);
    void process_unsubs_bbo(const std::string& line);
    void decrement_bbo(const symbol_t&);
    void print_bbo_subs() const;
    static bool str_to_order_id(const std::string&, order_id_t*);
    static bool str_to_symbol(const std::string&, symbol_t*);
    static bool str_to_side(const std::string&, side_t*);
    static bool str_to_quantity(const std::string&, quantity_t*);
    static bool str_to_price(const std::string&, double*);
    bool should_keep_symbol(const std::string&) const;
    order_book& get_order_book(const std::string&);
    bool is_there_order_book(const std::string&) const;
    const order_book& get_order_book_ref(const std::string&) const;
    order_book& get_order_book_ref(const std::string&);
    void print(const symbol_t& symbol) const;
    void print_full(const symbol_t& symbol) const;
    bool is_there_order_book(const order_id_t&) const;
    order_book& get_order_book_ref(const order_id_t&);
    const order_book& get_order_book_ref(const order_id_t&) const;
};

/*
 *
 */
void print_to_stdout(const std::string& s);
void print_to_stderr(const std::string& line, const std::string& err);


}  // namespace tbricks_test

#endif  // FEED_HANDLER_H
