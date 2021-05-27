#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
const static uint64_t TWO_POWERS32 = 4294967295 + 1;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    n = n + isn.raw_value();
    return WrappingInt32(static_cast<uint32_t>(n));
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
    // FIXME: this is a piece of shit
    // 将uint32_t补为64位
    uint64_t extended_n = n.raw_value();
    uint64_t extended_isn = isn.raw_value();
    uint64_t exceed_steps = (extended_n + TWO_POWERS32 - extended_isn) % TWO_POWERS32;
    uint64_t round_nums = checkpoint / TWO_POWERS32;
    uint64_t choice1, choice2, choice3, res;
    uint64_t diff1, diff2, diff3;

    // 计算三种可能的 absolute sequence number
    choice2 = round_nums * TWO_POWERS32 + exceed_steps;
    choice3 = (round_nums + 1) * TWO_POWERS32 + exceed_steps;
    if (round_nums != 0) {
        choice1 = (round_nums - 1) * TWO_POWERS32 + exceed_steps;
    } else {
        choice1 = choice2;
    }

    // 计算三个choice与checkpoint的距离
    if (choice1 < checkpoint) {
        diff1 = checkpoint - choice1;
    } else {
        diff1 = choice1 - checkpoint;
    }

    if (choice2 < checkpoint) {
        diff2 = checkpoint - choice2;
    } else {
        diff2 = choice2 - checkpoint;
    }

    diff3 = choice3 - checkpoint;

    // 根据三个距离选择结果
    if (diff1 < diff2) {
        if (diff1 < diff3) {
            res = choice1;
        } else {
            res = choice3;
        }
    } else {
        if (diff2 < diff3) {
            res = choice2;
        } else {
            res = choice3;
        }
    }

    return res;
}
