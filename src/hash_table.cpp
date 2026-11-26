#include "project/hash_table.h"

#include <stdexcept>

// Constructor
HashTable::HashTable(std::size_t capacity)
    : table_(capacity), size_(0) {

    if (capacity == 0) {
        throw std::invalid_argument(
            "Hash table capacity must be greater than zero");
    }

    for (std::size_t i = 0; i < table_.size(); ++i) {
        table_[i] = HashEntry();
    }
}

// Destructor
HashTable::~HashTable() = default;

// Generate a hash index from a string key
std::size_t HashTable::hashFunction(
    const std::string& key) const {

    std::size_t hash = 2166136261u;

    for (unsigned char ch : key) {
        hash ^= ch;
        hash *= 16777619u;
    }

    return hash % table_.size();
}

// Quadratic probing with tombstone handling
std::size_t HashTable::probe(
    const std::string& key) const {

    std::size_t original = hashFunction(key);
    std::size_t firstDeleted = table_.size();

    for (std::size_t step = 0;
         step < table_.size(); ++step) {

        std::size_t index =
            (original + step * step) % table_.size();

        const auto& entry = table_[index];

        // Return the index if the active key is found
        if (entry.occupied && !entry.deleted &&
            entry.key == key) {

            return index;
        }

        // Remember the first deleted position
        if (entry.deleted) {
            if (firstDeleted == table_.size()) {
                firstDeleted = index;
            }
        }

        // Stop at a position that has never been used
        else if (!entry.occupied) {
            if (firstDeleted != table_.size()) {
                return firstDeleted;
            }

            return index;
        }
    }

    // Return a reusable deleted position if available
    return firstDeleted;
}

// Insert a new key-value pair
bool HashTable::insert(
    const std::string& key, int value) {

    // Reject duplicate keys
    if (contains(key)) {
        return false;
    }

    // Resize before exceeding a load factor of 0.5
    if (static_cast<double>(size_ + 1) /
        static_cast<double>(table_.size()) > 0.5) {

        rehash();
    }

    std::size_t index = probe(key);

    // Resize if quadratic probing cannot find a slot
    while (index == table_.size()) {
        rehash();
        index = probe(key);
    }

    table_[index].key = key;
    table_[index].value = value;
    table_[index].occupied = true;
    table_[index].deleted = false;

    ++size_;

    return true;
}

// Remove a key using a tombstone
bool HashTable::remove(const std::string& key) {
    std::size_t index = probe(key);

    if (index == table_.size() ||
        !table_[index].occupied ||
        table_[index].deleted ||
        table_[index].key != key) {

        return false;
    }

    // Preserve the probing sequence
    table_[index].deleted = true;

    --size_;

    return true;
}

// Check whether a key exists
bool HashTable::contains(
    const std::string& key) const {

    std::size_t index = probe(key);

    return index < table_.size() &&
           table_[index].occupied &&
           !table_[index].deleted &&
           table_[index].key == key;
}

// Retrieve the value associated with a key
int HashTable::get(const std::string& key) const {
    std::size_t index = probe(key);

    if (index == table_.size() ||
        !table_[index].occupied ||
        table_[index].deleted ||
        table_[index].key != key) {

        throw std::out_of_range("Key not found");
    }

    return table_[index].value;
}

// Update an existing key's value
bool HashTable::update(
    const std::string& key, int newValue) {

    std::size_t index = probe(key);

    if (index == table_.size() ||
        !table_[index].occupied ||
        table_[index].deleted ||
        table_[index].key != key) {

        return false;
    }

    table_[index].value = newValue;

    return true;
}

// Return the number of active entries
std::size_t HashTable::size() const noexcept {
    return size_;
}

// Return the total table capacity
std::size_t HashTable::capacity() const noexcept {
    return table_.size();
}

// Check whether the hash table is empty
bool HashTable::empty() const noexcept {
    return size_ == 0;
}

// Calculate the load factor
double HashTable::loadFactor() const noexcept {
    return static_cast<double>(size_) /
           static_cast<double>(table_.size());
}

// Return all active keys
std::vector<std::string> HashTable::keys() const {
    std::vector<std::string> result;

    for (const auto& entry : table_) {
        if (entry.occupied && !entry.deleted) {
            result.push_back(entry.key);
        }
    }

    return result;
}

// Return all active values
std::vector<int> HashTable::values() const {
    std::vector<int> result;

    for (const auto& entry : table_) {
        if (entry.occupied && !entry.deleted) {
            result.push_back(entry.value);
        }
    }

    return result;
}

// Increase capacity and reinsert active entries
void HashTable::rehash() {
    std::size_t newCapacity =
        nextPrime(table_.size() * 2);

    // Preserve the original entries
    std::vector<HashEntry> oldTable = table_;

    // Create a larger empty table
    table_.assign(newCapacity, HashEntry());

    size_ = 0;

    // Reinsert active entries using the new capacity
    for (const auto& entry : oldTable) {
        if (entry.occupied && !entry.deleted) {
            insert(entry.key, entry.value);
        }
    }
}

// Determine whether a number is prime
bool HashTable::isPrime(std::size_t value) const {
    if (value < 2) {
        return false;
    }

    if (value == 2) {
        return true;
    }

    if (value % 2 == 0) {
        return false;
    }

    for (std::size_t i = 3;
         i <= value / i; i += 2) {

        if (value % i == 0) {
            return false;
        }
    }

    return true;
}

// Find the next prime number
std::size_t HashTable::nextPrime(
    std::size_t value) const {

    if (value <= 2) {
        return 2;
    }

    std::size_t candidate =
        value % 2 == 0 ? value + 1 : value;

    while (!isPrime(candidate)) {
        candidate += 2;
    }

    return candidate;
}
