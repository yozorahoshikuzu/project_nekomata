export module nekomata2:core.containers.mpsc_queue;
import std;

export template <typename T> class AtomicMpscQueue {
public:
    AtomicMpscQueue() {
        Node* dummy = new Node();
        m_head.store(dummy);
        m_tail = dummy;
    }


    ~AtomicMpscQueue() {
        while (pop().has_value())
            ;
        delete m_tail;
    }

    auto push(T value) -> void {
        Node* node = new Node(std::move(value));
        Node* prev = m_head.exchange(node, std::memory_order_acq_rel);
        prev->m_next.store(node, std::memory_order_release);
    }

    auto pop() -> std::optional<T> {
        Node* next = m_tail->m_next.load(std::memory_order_acquire);
        if (!next)
            return std::nullopt;

        auto result = std::move(next->m_value);

        Node* oldTail = m_tail;
        m_tail = next;
        delete oldTail;
        return result;
    }

    auto peek() -> std::optional<std::reference_wrapper<T>> {
        Node* next = m_tail->m_next.load(std::memory_order_acquire);
        if (!next)
            return std::nullopt;

        if (!next->m_value.has_value())
            return std::nullopt;

        return std::ref(next->m_value.value());
    }

    auto hasElement() -> bool {
        Node* next = m_tail->m_next.load(std::memory_order_acquire);
        return next != nullptr;
    }

private:
    struct Node {
        std::optional<T> m_value;
        std::atomic<Node*> m_next{nullptr};

        Node() = default;
        Node(T v) : m_value(std::move(v)) {}
    };

    std::atomic<Node*> m_head;
    Node* m_tail;
};