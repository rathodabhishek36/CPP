#include <iostream>
#include <map>
#include <unordered_map>
#include <list>
#include <memory>
#include <chrono>
#include <algorithm>
#include <random>

struct Order;

using Volume = uint32_t;
using Price = double;
using OrderList = std::list<std::unique_ptr<Order>>;
using OrderListIterator = typename OrderList::iterator;

enum Side : bool {
    Buy = true,
    Sell = false,
};

struct Order {
    int id_;
    Side side_;
    Volume shares_;
    Price price_;

    Order (int id, Side side, Volume shares, Price price)
        : id_(id)
        , side_(side)
        , shares_(shares)
        , price_(price) 
    {
    }

};

struct PriceLevel {
    uint64_t noOfOrders_ = 0;
    Volume volume_ = 0;
    OrderList orderList_;

    PriceLevel(const Order& order) {
        upsert_order(order);
    }

    Order* upsert_order(const Order& order) {
        ++noOfOrders_;
        volume_ += order.shares_;
        orderList_.push_back(std::move(std::make_unique<Order>(order)));
        return orderList_.back().get();
    }

};

// Sequence container implementaion of OrderBook
// Worse theoritical time complexity than std::map
// But better practical time complexity due to cache friendliness
class OrderBookPerSymbolWithVector {
    std::vector<std::pair<Price, std::unique_ptr<PriceLevel>>> buyLevels_;
    std::vector<std::pair<Price, std::unique_ptr<PriceLevel>>> sellLevels_;
    std::unordered_map<int, Order*> orderLookup_; // cannot store iterators as vector iterators may be invalidated

    template<typename Comparator>
    inline void add_order(auto& level, const Order& order, Comparator comparator) {
        auto id = order.id_;
        auto price = order.price_;
        auto priceItr = std::lower_bound(std::begin(level), std::end(level), price, [comparator] (const auto& p, Price price) {
            return comparator(p.first, price);
        });
        Order* orderPtr = nullptr;
        if (priceItr != level.end() && priceItr->first == order.price_) {
            orderPtr = priceItr->second->upsert_order(order);
        } else {
            auto orderItr = level.emplace(priceItr, order.price_, std::make_unique<PriceLevel>(order));
            orderPtr = orderItr->second->orderList_.back().get();
        }
        orderLookup_[id] = orderPtr;
    }

    inline void cancel_order(auto& level, Order* orderPtr) {
        auto id = orderPtr->id_;
        auto priceItr = std::find_if(std::begin(level), std::end(level), [orderPtr] (const auto& p) {
            return p.first == orderPtr->price_;
        });
        if (priceItr != std::end(level)) {
            auto& priceLevel = priceItr->second;
            if (--priceLevel->noOfOrders_ == 0) {
                level.erase(priceItr);
            } else {
                priceLevel->volume_ -= orderPtr->shares_;
                priceLevel->orderList_.remove_if([id] (const auto& order) {
                    return order->id_ == id;
                });
            }
            orderLookup_.erase(id);
        }
    }

public:
    void add_order(const Order& order) {
        if (order.side_ == Side::Buy) {
            add_order(buyLevels_, order, std::greater<Price>());
        } else {
            add_order(sellLevels_, order, std::less<Price>());
        }
    }

    void cancel_order(const int orderId) {
        auto orderPtr = orderLookup_[orderId];
        if (orderPtr) {
            if (orderPtr->side_ == Side::Buy) {
                cancel_order(buyLevels_, orderPtr);
            } else {
                cancel_order(sellLevels_, orderPtr);
            }
        }
    }
};

// Node container implementaion of OrderBook
// Better theoritical time complexity than vector
// But worse practical time complexity due to cache misses
class OrderBookPerSymbolWithRBTree {
    std::map<Price, std::unique_ptr<PriceLevel>, std::greater<Price>> buyTree_;
    std::map<Price, std::unique_ptr<PriceLevel>, std::less<Price>> sellTree_;
    std::unordered_map<int, OrderListIterator> orderLookup_;

    // void match_orders() {
    //     while (!buyTree_.empty() && !sellTree_.empty()) {
    //         auto buy_it = buyTree_.begin();
    //         auto sell_it = sellTree_.begin();
    //         if (buy_it->first < sell_it->first) {
    //             break;
    //         }

    //         auto& buyOrders = buy_it->second->orderList_;
    //         auto& sellOrders = sell_it->second->orderList_;

    //         while (!buyOrders.empty() && !sellOrders.empty()) {
    //             auto& buy_order = buyOrders.front();
    //             auto& sell_order = sellOrders.front();
                
    //             int matched_quantity = std::min(buy_order->shares_, sell_order->shares_);
    //             buy_order->shares_ -= matched_quantity;
    //             sell_order->shares_ -= matched_quantity;
                
    //             std::cout << "Matched: " << matched_quantity << " @ " << sell_it->first << "\n";
                
    //             if (buy_order->shares_ == 0) { 
    //                 buyOrders.pop_front();
    //             }
    //             if (sell_order->shares_ == 0) {
    //                 sellOrders.pop_front();
    //             }
    //         }
            
    //         if (buyOrders.empty()){
    //             buyTree_.erase(buy_it);
    //         }
    //         if (sellOrders.empty()) {
    //             sellTree_.erase(sell_it);
    //         }
    //     }
    // }

    inline void add_order(auto& tree, const Order& order) {
        auto id = order.id_;
        auto price_itr = tree.find(order.price_);
        if (price_itr == tree.end()) {
            price_itr = tree.try_emplace(order.price_, std::make_unique<PriceLevel>(order)).first;
        } else {
            price_itr->second->upsert_order(order);
        }
        auto orderItr = std::prev(price_itr->second->orderList_.end());
        orderLookup_[id] = orderItr;
    }

    inline void cancel_order(auto& tree, OrderListIterator orderItr) {
        const auto& orderPtr = orderItr->get();
        auto orderId = orderPtr->id_;
        auto priceItr = tree.find(orderPtr->price_);
        if (priceItr != tree.end()) {
            auto& priceLevel = priceItr->second;
            if (--priceLevel->noOfOrders_ == 0) {
                tree.erase(priceItr);
            } else {
                priceLevel->volume_ -= orderPtr->shares_;
                priceLevel->orderList_.erase(orderItr);
            }
            orderLookup_.erase(orderId);
        }
    }

public:
    void add_order(const Order& order) {
        if (order.side_ == Side::Buy) {
            add_order(buyTree_, order);
        } else {
            add_order(sellTree_, order);
        }
        // match_orders(); // Match orders immediately after adding
    }

    void cancel_order(const int orderId) {
        auto orderItr = orderLookup_[orderId];
        if (orderItr->get()->side_ == Side::Buy) {
            cancel_order(buyTree_, orderItr);
        } else {
            cancel_order(sellTree_, orderItr);
        }
    }
};

template<typename OrderBook>
void run() {
    OrderBook orderBook;
    perf_test(orderBook);
}

void perf_test(auto& orderBook) {
    const int numOrders = 100000;
    std::vector<std::unique_ptr<int>> randomAllocs;

    // Seed for randomness
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<Price> priceDistribution(100.0, 110.0);
    std::uniform_int_distribution<Volume> volumeDistribution(1, 100);

    // Benchmark : Add large nuumber of orders
    for (int i=0; i<numOrders; ++i) {
        Side side = (i%2 == 0) ? Side::Buy : Side::Sell;
        Price price = priceDistribution(rng);
        Volume shares = volumeDistribution(rng);
        if (price < 105 && shares > 30) {
            randomAllocs.push_back(std::make_unique<int>(i));
        }
        orderBook.add_order(Order(i, side, shares, price));
    }

    // Benchmark : Cancel some orders
    for (int i=0; i<numOrders; i+=10) {
        orderBook.cancel_order(i);
    }

    std::cout << "Workload complete" << std::endl;
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: ./a.out <vector|rbtree>" << std::endl;
        return 1;
    }
    std::string arg = argv[1];
    if (arg == "vector") {
        run<OrderBookPerSymbolWithVector>();
    } else if (arg == "rbtree") {
        run<OrderBookPerSymbolWithRBTree>();
    } else {
        std::cerr << "Usage: ./a.out <vector|rbtree>" << std::endl;
        return 1;
    }

    return 0;
}