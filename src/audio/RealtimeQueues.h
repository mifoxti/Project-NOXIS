#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace noxis {
// Producers are bounded to 16 attempts. The audio consumer never waits.
template<typename T,std::size_t Capacity> class EventQueue {
    struct Slot {std::atomic<std::size_t> sequence{0};T item{};};
    std::array<Slot,Capacity> slots{};
    std::atomic<std::size_t> writePosition{0};std::size_t readPosition=0;
public:
    EventQueue(){for(std::size_t i=0;i<Capacity;++i)slots[i].sequence.store(i);}
    bool push(const T& item) noexcept {
        auto position=writePosition.load(std::memory_order_relaxed);
        for(int attempt=0;attempt<16;++attempt) {
            auto& slot=slots[position%Capacity];const auto seq=slot.sequence.load(std::memory_order_acquire);
            const auto difference=static_cast<std::intptr_t>(seq)-static_cast<std::intptr_t>(position);
            if(difference==0) {
                if(writePosition.compare_exchange_weak(position,position+1,std::memory_order_relaxed)) {slot.item=item;slot.sequence.store(position+1,std::memory_order_release);return true;}
            } else if(difference<0) return false; else position=writePosition.load(std::memory_order_relaxed);
        } return false;
    }
    bool pop(T& item) noexcept {
        auto& slot=slots[readPosition%Capacity];if(slot.sequence.load(std::memory_order_acquire)!=readPosition+1)return false;
        item=slot.item;slot.sequence.store(readPosition+Capacity,std::memory_order_release);++readPosition;return true;
    }
};
class ScopeBuffer {
public:
    void push(float value) noexcept {
        const auto w=write.load(std::memory_order_relaxed);const auto next=(w+1)%data.size();
        if(next==read.load(std::memory_order_acquire))return;
        data[w]=value;write.store(next,std::memory_order_release);
    }
    bool pop(float& value) noexcept {
        const auto r=read.load(std::memory_order_relaxed);if(r==write.load(std::memory_order_acquire))return false;
        value=data[r];read.store((r+1)%data.size(),std::memory_order_release);return true;
    }
private:
    std::array<float,8192> data{};std::atomic<std::size_t> write{0},read{0};
};
static_assert(std::atomic<std::size_t>::is_always_lock_free);
}
