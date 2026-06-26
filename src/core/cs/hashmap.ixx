module;
#include <cstdlib>
#include <xxhash.h>
#include <emmintrin.h>
#include <mmintrin.h>
#include <string.h>
export module nekomata2:core.cs.hashmap;
import :core.platform.int_def;
import :core.platform.assert;
import :core.cs.iterators;
import :core.cs.mem;

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

export template <typename K, typename V, typename H> class HashMap;
export template <typename K, typename V, typename H> class HashMapKeysIter;
export template <typename K, typename V, typename H> class HashMapValuesIter;
export template <typename K, typename V, typename H> class HashMapIter;

template <typename T> constexpr auto avalancheHash(T v) -> u64 {
    u64 x = v;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9_u64;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb_u64;
    return x ^ (x >> 31);
}

template <typename K> struct Hash;

template<> struct Hash<u8>          { static u64 hash(u8 v) { return avalancheHash(v); } };
template<> struct Hash<i8>          { static u64 hash(i8 v) { return avalancheHash(v); } };
template<> struct Hash<u16>         { static u64 hash(u16 v) { return avalancheHash(v); } };
template<> struct Hash<i16>         { static u64 hash(i16 v) { return avalancheHash(v); } };
template<> struct Hash<u32>         { static u64 hash(u32 v) { return avalancheHash(v); } };
template<> struct Hash<i32>         { static u64 hash(i32 v) { return avalancheHash(v); } };
template<> struct Hash<u64>         { static u64 hash(u64 v) { return avalancheHash(v); } };
template<> struct Hash<i64>         { static u64 hash(i64 v) { return avalancheHash(v); } };
template<> struct Hash<f32>         { static u64 hash(f32 v) { return avalancheHash(v); } };
template<> struct Hash<f64>         { static u64 hash(f64 v) { return avalancheHash(v); } };
template<> struct Hash<bool>        { static u64 hash(bool v) { return avalancheHash(v); } };
template<> struct Hash<std::string> { static u64 hash(std::string_view v) { return XXH3_64bits(v.data(), v.size()); } };
template<> struct Hash<std::type_index> { static u64 hash(std::type_index v) { return avalancheHash(v.hash_code()); } };

export template <typename K, typename V, typename H = Hash<K>> class HashMap {
public:
    ~HashMap() {
        freeAll();
    }

    static auto create() -> HashMap {
        auto hashmap = HashMap();
        hashmap.alloc(kInitialCapacity);
        return hashmap;
    }

    static auto withCapacity(usize cap) -> HashMap {
        auto hashmap = HashMap();
        hashmap.alloc(cap);
        return hashmap;
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto insert(const K& key, const V& value) -> V& {
        return insertInner(K(key), V(value));
    }

    constexpr auto insert(const K& key, V&& value) -> V& {
        return insertInner(K(key), static_cast<V&&>(value));
    }

    constexpr auto insert(K&& key, V&& value) -> V& {
        return insertInner(static_cast<K&&>(key), static_cast<V&&>(value));
    }


    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto get(const K& key) -> std::optional<std::reference_wrapper<V>> {
        u64 hash = H::hash(key);
        u8 h2 = computeH2(hash);
        usize index = hashHomeIndex(hash);

        __m128i needle = _mm_set1_epi8(h2);
        __m128i empty = _mm_set1_epi8(kCtrlSentinelEmpty);

        for (usize step = 0; step < m_capacity; step += 16) {
            usize base = (step + index) & (m_capacity - 1);

            __m128i group = _mm_loadu_si128(reinterpret_cast<const __m128i*>(m_ctrls + base));
            u32 matches = _mm_movemask_epi8(_mm_cmpeq_epi8(group, needle));

            while (matches) {
                u32 bit = __builtin_ctz(matches);
                usize slot = (base + bit) & (m_capacity - 1);
                if (m_entries[slot].key == key) return m_entries[slot].value;
                matches &= (matches - 1);
            }
            u32 emptyCount = _mm_movemask_epi8(_mm_cmpeq_epi8(group, empty));
            if (emptyCount) return std::nullopt;
        }
        return std::nullopt;
    }

    constexpr auto operator[](const K& key) -> V& {
        auto ref = get(key);
        debug_assert(ref.has_value(), "HashMap: key not found");
        return ref.value();
    }

    constexpr auto contains(const K& key) -> bool {
        return get(key).has_value();
    }


    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto remove(const K& key) -> std::optional<V> {
        u64 hash = H::hash(key);
        u8 h2 = computeH2(hash);
        usize index = hashHomeIndex(hash);

        __m128i needle = _mm_set1_epi8(h2);
        __m128i empty = _mm_set1_epi8(kCtrlSentinelEmpty);

        for (usize step = 0; step < m_capacity; step += 16) {
            usize base = (step + index) & (m_capacity - 1);

            __m128i group = _mm_loadu_si128(reinterpret_cast<const __m128i*>(m_ctrls + base));

            u32 matches = _mm_movemask_epi8(_mm_cmpeq_epi8(group, needle));

            while (matches) {
                u32 bit = __builtin_ctz(matches);
                usize slot = (base + static_cast<usize>(bit)) & (m_capacity - 1);
                if (m_entries[slot].key == key) return removeAt(slot);

                matches &= (matches - 1);
            }

            u32 emptyCount = _mm_movemask_epi8(_mm_cmpeq_epi8(group, empty));
            if (emptyCount) return std::nullopt;
        }
        return std::nullopt;
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto keys() -> HashMapKeysIter<K, V, H> {
        return HashMapKeysIter<K, V, H>(this);
    }

    constexpr auto values() -> HashMapValuesIter<K, V, H> {
        return HashMapValuesIter<K, V, H>(this);
    }

    constexpr auto iter() -> HashMapIter<K, V, H> {
        return HashMapIter<K, V, H>(this);
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto len() const -> usize { return m_len; }
    constexpr auto capacity() const -> usize { return m_capacity; }
    constexpr auto isEmpty() const -> bool { return m_len == 0; }

    constexpr auto clear() -> void {
        destroyEntries();
        memset(m_ctrls, kCtrlSentinelEmpty, m_capacity + 16);
        memset(m_dibs, 0, m_capacity * sizeof(u32));
        m_len = 0;
    }

private:
    HashMap() = default;
    friend class HashMapKeysIter<K, V, H>;
    friend class HashMapValuesIter<K, V, H>;
    friend class HashMapIter<K, V, H>;

    static constexpr auto kKeyNeedsFinalizer = !std::is_trivially_destructible_v<K>;
    static constexpr auto kValNeedsFinalizer = !std::is_trivially_destructible_v<V>;
    static constexpr auto kMustFinalize = kKeyNeedsFinalizer || kValNeedsFinalizer;

    static constexpr auto kInitialCapacity = 32_usize;
    static constexpr auto kLoadFactor      = 0.75_f32;

    struct Entry {
        K key;
        V value;
    };

    u8*    m_ctrls    = nullptr;
    Entry* m_entries  = nullptr;
    u32*   m_dibs     = nullptr;
    u32    m_len      = 0;
    u32    m_capacity = 0;

    static constexpr auto kCtrlSentinelEmpty   = 0x80_u8;

    static constexpr auto computeH2(u64 hash) -> u8 { return static_cast<u8>(hash & 0x7f); }

    constexpr auto hashHomeIndex(u64 hash) -> usize { return static_cast<usize>(hash >> 7) & (m_capacity - 1); }
    constexpr auto computeNextIndex(usize index) -> usize { return (index + 1) & (m_capacity - 1); }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto alloc(usize cap) -> void {
        m_capacity = cap;
        m_ctrls = Mem::allocAlignedChecked<u8>(cap + 16, 16);
        m_entries = Mem::allocChecked<Entry>(cap);
        m_dibs = Mem::allocChecked<u32>(cap);

        memset(m_ctrls, kCtrlSentinelEmpty, cap + 16);
        memset(m_dibs, 0, cap * sizeof(u32));
    }

    constexpr auto freeAll() -> void {
        if (!m_ctrls) return;
        destroyEntries();
        Mem::freeAligned(m_ctrls);
        Mem::free(m_entries);
        Mem::free(m_dibs);
        m_ctrls = nullptr;
        m_entries = nullptr;
        m_dibs = nullptr;
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto removeAt(usize index) -> std::optional<V> {
        auto result = std::make_optional(std::move(m_entries[index].value));

        if constexpr (kKeyNeedsFinalizer) m_entries[index].key.~K();
        if constexpr (kValNeedsFinalizer) m_entries[index].value.~V();
        m_ctrls[index] = kCtrlSentinelEmpty;
        mirrorPaddingEntries(index);
        m_dibs[index] = 0;

        m_len--;

        while (true) {
            auto nextIndex = computeNextIndex(index);
            if (m_ctrls[nextIndex] == kCtrlSentinelEmpty || m_dibs[nextIndex] == 0) break;

            new (&m_entries[index].key) K(std::move(m_entries[nextIndex].key));
            new (&m_entries[index].value) V(std::move(m_entries[nextIndex].value));
            m_ctrls[index] = m_ctrls[nextIndex];
            mirrorPaddingEntries(index);
            m_dibs[index] = m_dibs[nextIndex] - 1;

            if constexpr (kKeyNeedsFinalizer) m_entries[nextIndex].key.~K();
            if constexpr (kValNeedsFinalizer) m_entries[nextIndex].value.~V();
            m_ctrls[nextIndex] = kCtrlSentinelEmpty;
            mirrorPaddingEntries(nextIndex);
            m_dibs[nextIndex] = 0;
            index = nextIndex;
        }

        return result;
    }


    constexpr auto insertInner(K&& key, V&& value) -> V& {
        maybeRehash();

        u64 hash = H::hash(key);
        u8 h2 = computeH2(hash);
        usize index = hashHomeIndex(hash);

        auto wkey = std::forward<K>(key);
        auto wvalue = std::forward<V>(value);
        auto wh2 = h2;
        auto wdib = 0_u32;

        while (true) {
            u8 c = m_ctrls[index];

            // Path 1 - Find an empty slot
            if (c == kCtrlSentinelEmpty) {
                new (&m_entries[index].key) K(std::move(wkey));
                new (&m_entries[index].value) V(std::move(wvalue));
                m_ctrls[index] = wh2;
                mirrorPaddingEntries(index);
                m_dibs[index] = wdib;
                m_len++;
                return m_entries[index].value;
            }

            // Path 2 - Same h2 and a possible key match
            if (c == wh2 && m_entries[index].key == wkey) {
                if constexpr (kValNeedsFinalizer) m_entries[index].value.~V();
                new (&m_entries[index].value) V(std::move(wvalue));
                return m_entries[index].value;
            }

            // Path 3 - Robin Hood steal
            if (wdib > m_dibs[index]) {
                auto tkey = std::move(m_entries[index].key);
                auto tvalue = std::move(m_entries[index].value);
                auto th2 = m_ctrls[index];
                auto tdib = m_dibs[index];

                if constexpr (kKeyNeedsFinalizer) m_entries[index].key.~K();
                if constexpr (kValNeedsFinalizer) m_entries[index].value.~V();

                new (&m_entries[index].key) K(std::move(wkey));
                new (&m_entries[index].value) V(std::move(wvalue));
                m_ctrls[index] = wh2;
                mirrorPaddingEntries(index);
                m_dibs[index] = wdib;

                wkey = std::move(tkey);
                wvalue = std::move(tvalue);
                wh2 = th2;
                wdib = tdib;
            }

            wdib++;
            index = computeNextIndex(index);
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto maybeRehash() -> void {
        if (m_len + 1 > static_cast<usize>(m_capacity * kLoadFactor)) rehash(m_capacity * 2);
    }

    constexpr auto rehash(usize newCap) -> void {
        auto oldCtrls = m_ctrls;
        auto oldEntries = m_entries;
        auto oldDibs = m_dibs;
        auto oldCap = m_capacity;

        alloc(newCap);
        m_len = 0;

        for (usize i = 0; i < oldCap; i++) {
            if (oldCtrls[i] != kCtrlSentinelEmpty) {
                insert(std::move(oldEntries[i].key), std::move(oldEntries[i].value));
            }
        }

        Mem::freeAligned(oldCtrls);
        Mem::free(oldEntries);
        Mem::free(oldDibs);
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------

    constexpr auto mirrorPaddingEntries(usize index) -> void {
        if (index < 16)
            m_ctrls[m_capacity + index] = m_ctrls[index];
    }

    constexpr auto destroyEntries() -> void {
        if constexpr (kMustFinalize) {
            for (usize i = 0; i < m_capacity; i++) {
                if (m_ctrls[i] != kCtrlSentinelEmpty) {
                    if constexpr (kKeyNeedsFinalizer) m_entries[i].key.~K();
                    if constexpr (kValNeedsFinalizer) m_entries[i].value.~V();
                }
            }
        }
    }
};


export template <typename K, typename V, typename H> class HashMapKeysIter : public IteratorBase<HashMapKeysIter<K, V, H>> {
public:
    using Item = NonZeroPtr<const K>;
    constexpr HashMapKeysIter(HashMap<K, V, H>* hashmap) : m_hashmap(hashmap) { skipEmpty(); }

    constexpr auto next() -> Option<Item> {
        if (m_index >= m_hashmap->m_capacity) return Option<Item>::none();
        auto i = m_index;
        m_index++;
        skipEmpty();
        return Option<Item>::some(NonZeroPtr<const K>(&m_hashmap->m_entries[i].key));
    }

private:
    HashMap<K, V, H>* m_hashmap = nullptr;
    usize m_index = 0;

    constexpr auto skipEmpty() -> void {
        while (m_index < m_hashmap->m_capacity && m_hashmap->m_ctrls[m_index] == HashMap<K, V, H>::kCtrlSentinelEmpty) m_index++;
    }
};

export template <typename K, typename V, typename H> class HashMapValuesIter : public IteratorBase<HashMapValuesIter<K, V, H>> {
public:
    using Item = NonZeroPtr<V>;
    constexpr HashMapValuesIter(HashMap<K, V, H>* hashmap) : m_hashmap(hashmap) { skipEmpty(); }

    constexpr auto next() -> Option<Item> {
        if (m_index >= m_hashmap->m_capacity) return Option<Item>::none();
        auto i = m_index;
        m_index++;
        skipEmpty();
        return Option<Item>::some(NonZeroPtr<V>(&m_hashmap->m_entries[i].value));
    }

private:
    HashMap<K, V, H>* m_hashmap = nullptr;
    usize m_index = 0;

    constexpr auto skipEmpty() -> void {
        while (m_index < m_hashmap->m_capacity && m_hashmap->m_ctrls[m_index] == HashMap<K, V, H>::kCtrlSentinelEmpty) m_index++;
    }
};

export template <typename K, typename V, typename H> class HashMapIter : public IteratorBase<HashMapIter<K, V, H>> {
public:
    using Item = KeyValue<NonZeroPtr<const K>, NonZeroPtr<V>>;
    constexpr HashMapIter(HashMap<K, V, H>* hashmap) : m_hashmap(hashmap) { skipEmpty(); }

    constexpr auto next() -> Option<Item> {
        if (m_index >= m_hashmap->m_capacity) return Option<Item>::none();
        auto i = m_index;
        m_index++;
        skipEmpty();
        return Option<Item>::some(Item(NonZeroPtr<const K>(&m_hashmap->m_entries[i].key), NonZeroPtr<V>(&m_hashmap->m_entries[i].value)));
    }

private:
    HashMap<K, V>* m_hashmap = nullptr;
    usize m_index = 0;

    constexpr auto skipEmpty() -> void {
        while (m_index < m_hashmap->m_capacity && m_hashmap->m_ctrls[m_index] == HashMap<K, V, H>::kCtrlSentinelEmpty) m_index++;
    }
};