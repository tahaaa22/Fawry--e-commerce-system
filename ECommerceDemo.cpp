#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <ctime>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <set>

// Abstract base for shippable products
class ShippableProduct {
public:
    virtual std::string getName() const = 0;
    virtual double getWeight() const = 0;
    virtual ~ShippableProduct() = default;
};

// Abstract Product class
class Product {
protected:
    std::string name;
    double price;
    int quantity;
public:
    Product(const std::string& n, double p, int q) : name(n), price(p), quantity(q) {}
    virtual ~Product() = default;
    virtual bool isExpired() const { return false; }
    virtual bool isShippable() const { return dynamic_cast<const ShippableProduct*>(this) != nullptr; }
    std::string getName() const { return name; }
    double getPrice() const { return price; }
    int getQuantity() const { return quantity; }
    void reduceQuantity(int amount) { quantity -= amount; }
};

// Expirable product
class ExpirableProduct : public Product {
    std::tm expiryDate;
public:
    ExpirableProduct(const std::string& n, double p, int q, const std::tm& exp)
        : Product(n, p, q), expiryDate(exp) {}
    bool isExpired() const override {
        std::time_t now = std::time(nullptr);
        std::tm today = *std::localtime(&now);
        return std::mktime(const_cast<std::tm*>(&expiryDate)) < std::mktime(&today);
    }
};

// Shippable product
class ShippableItem : public Product, public ShippableProduct {
    double weight;
public:
    ShippableItem(const std::string& n, double p, int q, double w)
        : Product(n, p, q), weight(w) {}
    double getWeight() const override { return weight; }
    std::string getName() const override { return name; }
};

// Expirable and shippable product
class ExpirableShippableItem : public ExpirableProduct, public ShippableProduct {
    double weight;
public:
    ExpirableShippableItem(const std::string& n, double p, int q, const std::tm& exp, double w)
        : ExpirableProduct(n, p, q, exp), weight(w) {}
    double getWeight() const override { return weight; }
    std::string getName() const override { return name; }
};

// Non-shippable, non-expirable product
class SimpleProduct : public Product {
public:
    SimpleProduct(const std::string& n, double p, int q) : Product(n, p, q) {}
};

// Customer class
class Customer {
    std::string name;
    double balance;
public:
    Customer(const std::string& n, double b) : name(n), balance(b) {}
    std::string getName() const { return name; }
    double getBalance() const { return balance; }
    void deductBalance(double amount) { balance -= amount; }
};

// Cart class
class Cart {
    std::map<Product*, int> items;
public:
    void add(Product* product, int quantity) {
        if (quantity <= 0) throw std::runtime_error("Quantity must be positive");
        if (product->isExpired()) throw std::runtime_error(product->getName() + " is expired");
        if (quantity > product->getQuantity()) throw std::runtime_error("Not enough stock for " + product->getName());
        items[product] += quantity;
    }
    const std::map<Product*, int>& getItems() const { return items; }
    bool isEmpty() const { return items.empty(); }
    void clear() { items.clear(); }
};

// Shipping Service
class ShippingService {
public:
    static void ship(const std::vector<ShippableProduct*>& items, const std::map<std::string, int>& nameToCount) {
        if (items.empty()) return;
        std::cout << "** Shipment notice **\n";
        double totalWeight = 0.0;
        std::set<std::string> printed;
        for (const auto& item : items) {
            if (printed.find(item->getName()) == printed.end()) {
                int count = nameToCount.at(item->getName());
                std::cout << count << "x " << item->getName() << "\n";
                printed.insert(item->getName());
            }
        }
        for (const auto& item : items) {
            std::cout << std::fixed << std::setprecision(0) << item->getWeight() * 1000 << "g\n";
            totalWeight += item->getWeight();
        }
        std::cout << "Total package weight " << std::fixed << std::setprecision(1) << totalWeight << "kg\n";
    }
};

// Checkout logic
void checkout(Customer& customer, Cart& cart) {
    if (cart.isEmpty()) {
        std::cout << "Error: Cart is empty\n";
        return;
    }
    double subtotal = 0.0;
    double shipping = 0.0;
    std::vector<ShippableProduct*> shippables;
    std::map<std::string, int> nameToCount;
    for (const auto& entry : cart.getItems()) {
        Product* product = entry.first;
        int qty = entry.second;
        if (product->isExpired()) {
            std::cout << "Error: " << product->getName() << " is expired\n";
            return;
        }
        if (qty > product->getQuantity()) {
            std::cout << "Error: Not enough stock for " << product->getName() << "\n";
            return;
        }
        subtotal += product->getPrice() * qty;
        if (product->isShippable()) {
            for (int i = 0; i < qty; ++i) {
                shippables.push_back(dynamic_cast<ShippableProduct*>(product));
            }
            nameToCount[product->getName()] = qty;
        }
    }
    double totalWeight = 0.0;
    for (const auto& item : shippables) totalWeight += item->getWeight();
    shipping = std::ceil(totalWeight * 10) * 3; // 30 per kg, 3 per 100g
    double total = subtotal + shipping;
    if (customer.getBalance() < total) {
        std::cout << "Error: Insufficient balance\n";
        return;
    }
    if (!shippables.empty()) {
        ShippingService::ship(shippables, nameToCount);
    }
    std::cout << "** Checkout receipt **\n";
    for (const auto& entry : cart.getItems()) {
        std::cout << entry.second << "x " << std::left << std::setw(12) << entry.first->getName()
                  << static_cast<int>(entry.first->getPrice() * entry.second) << "\n";
    }
    std::cout << "----------------------\n";
    std::cout << "Subtotal         " << static_cast<int>(subtotal) << "\n";
    std::cout << "Shipping         " << static_cast<int>(shipping) << "\n";
    std::cout << "Amount           " << static_cast<int>(total) << "\n";
    customer.deductBalance(total);
    std::cout << "Balance          " << static_cast<int>(customer.getBalance()) << "\n";
    std::cout << "END.\n\n";
    for (const auto& entry : cart.getItems()) {
        entry.first->reduceQuantity(entry.second);
    }
    cart.clear();
}

// Helper to create a date
std::tm makeDate(int year, int month, int day) {
    std::tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
    t.tm_isdst = -1;
    return t;
}

int main() {
    // Products
    auto cheese = std::make_unique<ExpirableShippableItem>("Cheese", 100, 5, makeDate(2025, 12, 31), 0.4);
    auto biscuits = std::make_unique<ExpirableShippableItem>("Biscuits", 150, 2, makeDate(2025, 12, 31), 0.7);
    auto tv = std::make_unique<ShippableItem>("TV", 150, 3, 7.0);
    auto mobile = std::make_unique<SimpleProduct>("Mobile", 200, 10);
    auto scratchCard = std::make_unique<SimpleProduct>("ScratchCard", 50, 20);

    // Customer
    Customer customer("Ali", 1000);

    // Cart
    Cart cart;
    try {
        cart.add(cheese.get(), 2);
        cart.add(biscuits.get(), 1);
        cart.add(tv.get(), 0); // Try with 0 TV to match sample output
        cart.add(scratchCard.get(), 1);
    } catch (const std::exception& e) {
        std::cout << e.what() << "\n";
    }
    checkout(customer, cart);

    // Try error cases
    Cart cart2;
    try {
        cart2.add(cheese.get(), 10); // More than available
    } catch (const std::exception& e) {
        std::cout << e.what() << "\n";
    }
    try {
        cart2.add(cheese.get(), 1);
        cheese->reduceQuantity(5); // Make cheese out of stock
        checkout(customer, cart2);
    } catch (const std::exception& e) {
        std::cout << e.what() << "\n";
    }
    Cart cart3;
    checkout(customer, cart3); // Empty cart

    return 0;
}