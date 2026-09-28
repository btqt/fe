#ifndef RDG_DIAG_QUEUE
#define RDG_DIAG_QUEUE

#include <iostream>
#include <queue>
#include <utils/Mutex.h>
#include <utils/Log.h>

namespace rdgapp {

template <class T, class Container = std::deque<T>,
          class Compare = std::less<typename Container::value_type>>
class DiagQueue {
public:
    using value_type = typename Container::value_type;
    // using iterator = typename Container::iterator;
    // using reverse_iterator = typename Container::reverse_iterator;
    // using const_iterator = typename Container::const_iterator;
private:
    Container c;
    Compare comp;

public:
    DiagQueue(const Container pC, const Compare pComp) noexcept : c(pC), comp(pComp) {}
    DiagQueue() noexcept : DiagQueue({}, {}) {}
    virtual ~DiagQueue() = default;
    DiagQueue(const DiagQueue&) = default;
    DiagQueue(DiagQueue&&) = default;
    DiagQueue& operator=(const DiagQueue&) = default;
    DiagQueue& operator=(DiagQueue&&) = default;

    bool empty() const noexcept {
        const android::Mutex::Autolock _l{mLock};
        (void)_l;
        return c.empty();
    }

    auto size() const noexcept -> decltype(c.size()) {
        const android::Mutex::Autolock _l{mLock};
        (void)_l;
        return c.size();
    }

    // auto front() const noexcept -> decltype(c.front()) {
    //     const android::Mutex::Autolock _l{mLock};
    //     return c.front();
    // }

    auto back() noexcept -> decltype(c.back()) {
        const android::Mutex::Autolock _l{mLock};
        (void)_l;
        return c.back();
    }

    // auto begin() noexcept -> decltype(c.begin()){
    //     const android::Mutex::Autolock _l{mLock};
    //     return c.begin();
    // }

    // auto end() noexcept -> decltype(c.end()) {
    //     const android::Mutex::Autolock _l{mLock};
    //     return c.end();
    // }

    // auto cbegin() noexcept -> decltype(c.cbegin()) {
    //     const android::Mutex::Autolock _l{mLock};
    //     return c.cbegin();
    // }

    // auto cend() noexcept -> decltype(c.cend()) {
    //     const android::Mutex::Autolock _l{mLock};
    //     return c.cend();
    // }


    // auto rbegin() noexcept -> decltype(c.rbegin()) {
    //     const android::Mutex::Autolock _l{mLock};
    //     return c.rbegin();
    // }

    // auto rend() noexcept -> decltype(c.rend()) {
    //     const android::Mutex::Autolock _l{mLock};
    //     return c.rend();
    // }

    void push(const value_type& v) noexcept {
        // const android::Mutex::Autolock _l{mLock};
        // c.push(v);
        // =======================UPDATE================================
        const android::Mutex::Autolock _l{mLock};

        // If queue is empty, insert at first
        if (c.empty()) {
            c.push_front(v);
        } else {
            typename Container::const_iterator it{};
            bool isInserted{false};
            for (it = c.cbegin(); it != c.cend(); ++it) {
                // Because queue is already sorted, so we insert at highest priority position possible
                if (*it < v) {
                    (void)c.insert(it, v);
                    isInserted = true;
                    break;
                }
            }

            // If inserted item has lowest priority, inserted at last
            if (isInserted == false) {
                c.push_back(v);
            }
        }
        (void)_l;
    }

    /* Pop out the lowest priority item */
    void pop_back() noexcept {
        const android::Mutex::Autolock _l{mLock};
        (void)_l;
        c.pop_back();
    }

    /* Pop out the highest priority item */
    // void pop_front() noexcept {
    //     const android::Mutex::Autolock _l{mLock};
    //     c.pop_front();
    // }
    
    auto top() noexcept -> decltype(c.front()) {
        const android::Mutex::Autolock _l{mLock};
        (void)_l;
        return c.front();
    }
    
    void pop() noexcept {
        const android::Mutex::Autolock _l{mLock};
        (void)_l;
        c.pop_front();
    }

    // void clear() noexcept {
    //     c.clear();
    // }

    // auto erase (const_iterator position) noexcept -> decltype(c.erase(position)) {
    //     const android::Mutex::Autolock _l{mLock};
    //     return c.erase(position);
    // }

private:
    mutable android::Mutex mLock;
};
}
#endif //RDG_DIAG_QUEUE
