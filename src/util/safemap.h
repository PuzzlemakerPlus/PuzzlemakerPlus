#include <unordered_map>
#pragma once

// A wrapper around std::unordered_map that disables operator[] to prevent accidental insertion of default-constructed values.
// used for m_nodeMap and m_surfaceMap
template <class K, class V, class Hash = std::hash<K>, class Eq = std::equal_to<K>>
class SafeUnorderedMap {
public:
    using Map = std::unordered_map<K, V, Hash, Eq>;
    using iterator = typename Map::iterator;
    using const_iterator = typename Map::const_iterator;

    SafeUnorderedMap() = default;

    V& at(const K& k) { return map_.at(k); }
    const V& at(const K& k) const { return map_.at(k); }

    iterator find(const K& k) { return map_.find(k); }
    const_iterator find(const K& k) const { return map_.find(k); }

    template <class... Args>
    auto emplace(Args&&... args) {
        return map_.emplace(std::forward<Args>(args)...);
    }

    size_t erase(const K& k) { return map_.erase(k); }
    size_t size() const { return map_.size(); }
    bool empty() const { return map_.empty(); }
    void clear() { map_.clear(); }

    iterator begin() { return map_.begin(); }
    iterator end() { return map_.end(); }
    const_iterator begin() const { return map_.begin(); }
    const_iterator end() const { return map_.end(); }
    const_iterator cbegin() const { return map_.cbegin(); }
    const_iterator cend() const { return map_.cend(); }

    V& defaultConstruct(const K& k) {
		return map_[k];
	}
    
private:
    Map map_;
    V& operator[](const K&) = delete;
    const V& operator[](const K&) const = delete;
};