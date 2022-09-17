#include <future>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <cassert>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>);

    struct Bucket{
        std::mutex m;
        std::map<Key, Value> submap;
    };

    struct Access {
        Access(std::mutex& m1, const Key& key, const size_t index, std::vector<Bucket>& buckets )
            : lg(m1), ref_to_value(buckets[index].submap[key]) {
        }

        Access operator+=(const Value& value) {
          ref_to_value += value;
          return *this;
        }

        std::lock_guard<std::mutex> lg;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : bucket_count_(bucket_count), buckets_(bucket_count_) {
    }

    Access operator[](const Key& key){
        size_t index = static_cast<size_t>(key) % bucket_count_;
        return Access(buckets_[index].m, key, index, buckets_);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> full_map;
        for (size_t i = 0; i < bucket_count_; ++i) {
          {
            std::lock_guard<std::mutex> lock_(buckets_[i].m);
            full_map.insert(buckets_[i].submap.begin(), buckets_[i].submap.end());
          }
        }
        return full_map;
    }

private:
    size_t bucket_count_;
    std::vector<Bucket> buckets_;
};
