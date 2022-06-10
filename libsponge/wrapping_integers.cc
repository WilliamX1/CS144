#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t raw_value = n & 0xFFFFFFFF; // n % (2 ^ 32)
    return isn + raw_value;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t val = static_cast<uint64_t>(n - isn) & 0xFFFFFFFF;

    // (2 ^ 32) * x + val ~ checkpoint
    // cout << "val: " << val << " checkpoint: " << checkpoint << " val >= checkpoint: " << (val >= checkpoint) << "\n";
    if (val >= checkpoint) return val;
    
    int32_t x = static_cast<int32_t>((checkpoint - val) >> 32);
    uint64_t ans = val, minus = val >= checkpoint ? val - checkpoint : checkpoint - val;
    for (int32_t i = x - 1; i <= x + 1; i++) {
        // check out every possible val
        uint64_t pos_ans = (1ul << 32) * static_cast<uint64_t>(i) + val;
        uint64_t pos_minus = pos_ans >= checkpoint ? pos_ans - checkpoint : checkpoint - pos_ans;
        if (pos_minus < minus) {
            minus = pos_minus;
            ans = pos_ans;
        };
        // cout << "i: " << i << " pos_ans: " << pos_ans << "\n";
    };
    
    return ans;
}
