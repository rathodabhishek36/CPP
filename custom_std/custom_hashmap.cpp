#include <iostream>
#include <vector>
#include <unordered_map>
#include <iterator>
#include <concepts>

namespace ajr {

template<typename T>
concept HasIsTransparent = requires { typename T::is_transparent; };

template<typename Key, typename Value>
class HashmapNode {
    Key first;
    Value second;
    HashmapNode* next_;
    HashmapNode* prev_;
    unsigned int bucket_;

    void assign(const Key& key, const Value& value) {
        first = key;
        second = value;
    }
};

template<typename Key, typename Value
        , typename Hash = std::hash<Key>
        , typename KeyEqual = std::equal_to<Key>
        , typename Allocator = std::allocator<HashmapNode<Key, Value>>>
class Hashmap {
    using value_type = std::pair<const Key, Value>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    struct Bucket {
        HashmapNode<Key, Value>* start_ = nullptr;
        HashmapNode<Key, Value>* end_ = nullptr;

        void create(HashmapNode<Key, Value>* node) {
            start_ = node;
            end_ = node;
        }

        bool operator()() const {
            return start_ != nullptr;
        }
    };
    unsigned long bucket_count_;
    std::vector<Bucket> buckets_;
    size_type size_;
    
    float max_load_factor_;
    Hash hashFunction_;
    KeyEqual keyEquality_;
    Allocator allocator_;

    template<typename KeyInput>
    HashmapNode<KeyInput, Value>* findNode(const KeyInput& key, unsigned int& bucket) const {
        bucket = hashFunction_(key) % bucket_count_;
        if (!buckets_[bucket]) {
            return nullptr;
        }
        auto start = buckets_[bucket].start_;
        while (start != buckets_[bucket].end_) {
            if (keyEquality_(start->first, key)) {
                return start;
            }
            start = start->next_;
        }
        return keyEquality_(buckets_[bucket].end_->first, key)
                ? buckets_[bucket].end_
                : nullptr;
    }

    inline void connectToOtherBuckets(unsigned int bucket) {
        auto prevBucket = bucket;
        while (--prevBucket >= 0) {
            if (buckets_[prevBucket]) {
                auto nextStart = buckets_[prevBucket].end_->next_;
                buckets_[prevBucket].end_->next_ = buckets_[bucket].start_;
                buckets_[bucket].start_->prev_ = buckets_[prevBucket].end_;
                buckets_[bucket].end_->next_ = nextStart;
                if (nextStart) { nextStart->prev_ = buckets_[bucket].end_; }
                return;
            }
        }
        auto nextBucket = bucket;
        while (++nextBucket < bucket_count_) {
            if (buckets_[nextBucket]) {
                auto prevEnd = buckets_[nextBucket].start_->prev_;
                buckets_[nextBucket].start_->prev_ = buckets_[bucket].end_;
                buckets_[bucket].end_->next_ = buckets_[nextBucket].start_;
                buckets_[bucket].start_->prev_ = prevEnd;
                if (prevEnd) { prevEnd->next_ = buckets_[bucket].start_; }
                return;
            }
        }
    }

public:
    class ForwardIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = HashmapNode<Key, Value>;
        using pointer = value_type*;
        using reference = value_type&;
        using difference_type = std::ptrdiff_t;
        
    private:
        pointer mPtr_;

    public:
        ForwardIterator() : mPtr_(nullptr) {}

        ForwardIterator(const pointer ptr) : mPtr_(ptr) {}

        ForwardIterator(const ForwardIterator& other) : mPtr_(other.mPtr_) {}

        ForwardIterator& operator=(const ForwardIterator& other) {
            mPtr_ = other.mPtr_;
            return *this;
        }
        
        ~ForwardIterator() = default;

        ForwardIterator operator++(int) {
            ForwardIterator tmp(*this);
            mPtr_ = mPtr_->next_;
            return tmp;
        }

        ForwardIterator& operator++() {
            mPtr_ = mPtr_->next_;
            return *this;
        }

        bool operator==(const ForwardIterator& other) const {
            return mPtr_ == other.mPtr_;
        }

        bool operator!=(const ForwardIterator& other) const {
            return mPtr_ != other.mPtr_;
        }

        ForwardIterator operator+(difference_type n) const {
            ForwardIterator tmp(*this);
            for (int i=0; i<n; ++i) {
                tmp.mPtr_ = tmp.mPtr_->next_;
            }
            return tmp;
        }

        ForwardIterator& operator+=(difference_type n) {
            for (int i=0; i<n; ++i) {
                mPtr_ = mPtr_->next_;
            }
            return *this;
        }

        reference operator*() const {
            return *mPtr_;
        }

        pointer operator->() const {
            return mPtr_;
        }
    };

    Hashmap() = default;

    Hashmap(size_type bucket_count = 16
            , const Hash& hash = Hash()
            , const KeyEqual& keyEquality = KeyEqual()
            , const Allocator& allocator = Allocator())
            : bucket_count_(bucket_count), buckets_(bucket_count, nullptr), hashFunction_(hash), keyEquality_(keyEquality), allocator_(allocator) 
    {    
    }

    Hashmap(size_type bucket_count, const Allocator& allocator) 
        : Hashmap(bucket_count, Hash(), KeyEqual(), allocator) 
    {
    }

    Hashmap(size_type bucket_count, const Hash& hash, const Allocator& allocator) 
        : Hashmap(bucket_count, hash, KeyEqual(), allocator) 
    {
    }

    template<std::input_iterator InputIt>
    requires (std::same_as<typename InputIt::value_type, value_type>)
    Hashmap(InputIt first, InputIt last
            , size_type bucket_count /* no need to specify */
            , const Hash& hash = Hash()
            , const KeyEqual& keyEquality = KeyEqual()
            , const Allocator& allocator = Allocator())
            : hashFunction_(hash), keyEquality_(keyEquality), allocator_(allocator) 
    {
        for (auto it=first; it!=last; ++it) {
            insert(*it);
        }
    }

    Hashmap(const Hashmap& other) {
        // TODO
    }

    Hashmap(Hashmap&& other) {
        // TODO
    }

    Hashmap& operator=(const Hashmap& other) {
        // TODO
    }

    Hashmap& operator=(Hashmap&& other) {
        // TODO
    }

    ~Hashmap() {
        // TODO
    }

    Hashmap(std::initializer_list<value_type> init, size_type bucket_count, const Hash& hash, const Allocator& allocator) :
        Hashmap(init.begin(), init.end(), bucket_count, hash, allocator)
    {
    }

    // Capacity functions
    bool empty() const noexcept {
        return size_ == 0;
    }

    std::size_t size() const noexcept {
        return size_;
    }

    // Modifier functions
    void clear() noexcept {
        for (const auto bucket : buckets_) {
            if (bucket) {
                auto node = bucket.start_;
                while (node) {
                    node->first.~Key();
                    node->second.~Value();
                    auto nextNode = node->next_;
                    allocator_.deallocate(node, 1);
                    node = nextNode;
                }
            }
        }
        std::fill(buckets_.begin(), buckets_.end(), nullptr);
        size_ = 0;
    }

    std::pair<ForwardIterator, bool> insert(const value_type& value) {
        unsigned int bucket;
        const auto& key = value.first;
        const auto& val = value.second;
        auto node = findNode(key, bucket);
        if (!node) {
            node = allocator_.allocate(1);
            ++size_;
            node->assign(key, val);
            if (!buckets_[bucket]) {
                buckets_[bucket].create(node);
                connectToOtherBuckets(bucket);
            } else {
                auto nextStart = buckets_[bucket].end_->next_;
                node->next_ = nextStart; 
                buckets_[bucket].end_->next_ = node;
                if (nextStart) { nextStart->prev_ = node; }
                node->prev_ = buckets_[bucket].end_;
                buckets_[bucket].end_ = node;
            }
            node->bucket_ = bucket;
            // TODO - If load_factor increases than a threshold, rehash
            return std::make_pair(ForwardIterator(node), true);
        }
        node->second = val;
        return std::make_pair(ForwardIterator(node), false);
    }

    std::pair<ForwardIterator, bool> insert(value_type&& value) {
        return insert(value);
    }

    template<typename P>
    std::pair<ForwardIterator, bool> insert(P&& value) {
        return insert(value_type(std::forward<P>(value)));
    }

    template<std::input_iterator InputIt>
    requires (std::is_same_v<InputIt::value_type, value_type>)
    void insert(InputIt first, InputIt last) {
        for (auto it=first; it!=last; ++it) {
            insert(*it);
        }
    }

    void insert(std::initializer_list<value_type> list) {
        insert(list.begin(), list.end());
    }

    template<typename... Args>
    std::pair<ForwardIterator, bool> emplace(Args&&... args) {
        return insert(value_type(std::forward<Args>(args)...));
    }

    ForwardIterator erase(const ForwardIterator pos) {
        auto node = pos.mPtr_;
        auto prevNode = node->prev_;
        auto nextNode = node->next_;
        if (prevNode) { prevNode->next_ = nextNode; }
        if (nextNode) { nextNode->prev_ = prevNode; }
        
        auto bucket = node->bucket_;
        if (buckets_[bucket].start_ == node) {
            buckets_[bucket].start_ = (nextNode && nextNode->bucket_==bucket) ? nextNode : nullptr;
        }
        if (buckets_[bucket].end_ == node) {
            buckets_[bucket].end_ = (prevNode && prevNode->bucket_==bucket) ? prevNode : nullptr;
        }
        node->first.~Key();
        node->second.~Value();
        allocator_.deallocate(node, 1);
        --size_;
        return ForwardIterator(nextNode);
    }

    ForwardIterator erase(const ForwardIterator first, const ForwardIterator last) {
        for (auto it=first; it!=last; ++it) {
            erase(it);
        }
        return last;
    }

    size_type erase(const Key& key) {
        unsigned int bucket;
        auto node = findNode(key, bucket);
        if (node) {
            erase(ForwardIterator(node));
            return 1;
        }
        return 0;
    }


    void swap(Hashmap& other) noexcept {
        // TODO
    }

    // Lookup functions
    Value& at(const Key& key) {
        unsigned int bucket;
        auto node = findNode(key, bucket);
        if (!node) {
            throw std::out_of_range("Key not found");
        }
        return node->second;
    }

    const Value& at(const Key& key) const {
        return at(key);
    }

    Value& operator[](const Key& key) {
        unsigned int bucket;
        auto node = findNode(key, bucket);
        if (!node) {
            return insert(value_type(key, Value())).first->second;
        }
        return node->second;
    }

    Value& operator[](Key&& key) {
        unsigned int bucket;
        auto node = findNode(key, bucket);
        if (!node) {
            return insert(value_type(std::move(key), Value())).first->second;
        }
        return node->second;
    }

    size_type count(const Key& key) const {
        unsigned int bucket;
        return findNode(key, bucket) ? 1 : 0;
    }

    template<typename KeyType>
    requires (HasIsTransparent<Hash> && HasIsTransparent<KeyEqual>)
    size_type count(const KeyType& key) const {
        unsigned int bucket;
        return findNode(key, bucket) ? 1 : 0;
    }

    template<typename KeyType>
    const ForwardIterator find(const KeyType& x) const {
        unsigned int bucket;
        return ForwardIterator(findNode(x, bucket));
    }

    template<typename KeyType>
    requires (HasIsTransparent<Hash> && HasIsTransparent<KeyEqual>)
    ForwardIterator find(const KeyType& x) {
        unsigned int bucket;
        return ForwardIterator(findNode(x, bucket));
    }

    bool contains(const Key& x) const {
        return find(x) != end();
    }

    template<typename KeyType>
    requires (HasIsTransparent<Hash> && HasIsTransparent<KeyEqual>)
    bool contains(const KeyType& x) const {
        return find(x) != end();
    }

    // Bucket interface functions
    ForwardIterator begin() noexcept {
        for (int bucket = 0; bucket < bucket_count_; ++bucket) {
            if (buckets_[bucket]) {
                return ForwardIterator(buckets_[bucket].start_);
            }
        }
        return nullptr;
    }
    
    const ForwardIterator cbegin() const noexcept {
        return begin();
    }
    
    ForwardIterator end() noexcept {
        for (int bucket = bucket_count_-1; bucket >= 0; --bucket) {
            if (buckets_[bucket]) {
                return ForwardIterator(buckets_[bucket].end_);
            }
        }
        return nullptr;
    }
    
    const ForwardIterator cend() const noexcept {
        return end();
    }
    
    size_type bucket_count() const {
        return bucket_count_;
    }

    size_type bucket_size(size_type bucket) const {
        auto start = buckets_[bucket].start_;
        size_type count = (start) ? 1 : 0;
        while (start != buckets_[bucket].end_) {
            ++count;
            start = start->next_;
        }
        return count;
    }

    size_type bucket(const Key& key) const {
        unsigned int bucket;
        findNode(key, bucket);
        return bucket;
    }

    template<typename KeyType>
    requires (HasIsTransparent<Hash> && HasIsTransparent<KeyEqual>)
    size_type bucket(const KeyType& k) const {
        unsigned int bucket;
        findNode(k, bucket);
        return bucket;
    }

    // Hash Policy Functions
    float load_factor() const {
        return size() / bucket_count();
    }

    float max_load_factor() const {
        return max_load_factor_;
    }

    void max_load_factor(float ml) const {
        max_load_factor_ = ml;
        // TODO ?? - need to rehash if load factor exceeds max_load_factor
    }

    void rehash(size_type count) {
        // TODO
    }

    void reserve(size_type count) {
        // TODO
    }

};

}

int main() {

    // ajr::Hashmap hashmap;
    std::unordered_map<int, int>* um = new std::unordered_map<int ,int>();
    std::cout << std::distance(um->begin(), um->end());

    return 0;
}