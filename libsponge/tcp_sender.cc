#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <cassert>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _current_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

size_t TCPSender::bytes_in_flight() const {
    // 这边可以遍历一遍容器，但我觉得比较傻逼，不过我还不确定这个是不是对的，之后如果有问题我就无脑遍历了XD
    if (_outstandingSegments.empty()) {
        return 0;
    } else {
        TCPSegment beginSegment = _outstandingSegments.front();
        TCPSegment endSegment = _outstandingSegments.back();
        if (_outstandingSegments.size() == 1) {
            return beginSegment.header().fin + beginSegment.header().syn + beginSegment.payload().size();
        } else {
            return endSegment.header().seqno + endSegment.payload().size() - beginSegment.header().seqno + endSegment.header().fin;
        }
    }
}


// I do this recursively
void TCPSender::fill_window() {
    switch (_tcpSenderState) {
        case CLOSED: {
            TCPSegment synSegment;
            synSegment.header().seqno = next_seqno();
            synSegment.header().syn = true;
            _segments_out.push(synSegment);
            _outstandingSegments.push(synSegment);
            _next_seqno += synSegment.length_in_sequence_space();
            _currentWindowSize -= synSegment.length_in_sequence_space();
            assert(_currentWindowSize == 0);
            _tcpSenderState = SYN_SENT;
            return;
        }
        case SYN_SENT: {
            if (next_seqno_absolute() > 0 && next_seqno_absolute() == bytes_in_flight()) {
                return;
            } else {
                _tcpSenderState = SYN_ACKED;
                fill_window();
                return;
            }
        }
        case SYN_ACKED: {
            if (stream_in().eof()) {
                if (next_seqno_absolute() == stream_in().bytes_written() + 2) {
                    _tcpSenderState = FIN_SENT;
                } else {
                    return;
                }
            } else {
                if (_currentWindowSize <= 0) {
                    send_empty_segment();
                    return;
                } else {
                    // 存在空余的window，此时进行填入，留下一个payload的slot，为了留给eof
                    // todo: 如果有一个eof的标志位，这时候需要减少一个字节的数据吗？现在是没有考虑，直接将fin置0就算了
                    TCPSegment theSegment;
                    Buffer payload = Buffer(stream_in().read(min(TCPConfig::MAX_PAYLOAD_SIZE, _currentWindowSize)));
                    if (!payload.size()) return;
                    theSegment.payload() = payload;
                    theSegment.header().fin = stream_in().eof();
                    theSegment.header().seqno = next_seqno();
                    _segments_out.push(theSegment);
                    _outstandingSegments.push(theSegment);
                    _currentWindowSize -= theSegment.length_in_sequence_space();
                    // do this recursively, check if can push another segment
                    fill_window();
                }
            }
        }
            break;
        case FIN_SENT:
            if (!bytes_in_flight()) {
                _tcpSenderState = FIN_ACKED;
            }
            return;
        case FIN_ACKED:
            return;
        case ERROR:
            return; // TODO: deal with the ERROR
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 收到ack
    // 1. 更新_currentWindowSize(只有从_outstandingSegments中弹出过了之后才进行更新), 并删除_outstandingSegments
    // 2. 如果有_currentWindowSize ！= 0， fill window
    // todo: 但是有可能接收方动态调整了窗口大小啊？这种情况会丢弃，之后的ack就会展现出这个窗口的变化，再次设置即可
    _currentWindowSize = window_size;
    uint64_t popedSize = 0;

    // FIXME: if the ack is wrong(smaller) , the loop wont end here
    if (unwrap(ackno, _isn, _next_seqno) < next_seqno_absolute()) return;
    while (!_outstandingSegments.empty()) {
        TCPSegment frontSegment = _outstandingSegments.front();
        // FIXME: 如果说windowsize > 2^32次，这里可能会出bug的，应为checkpoint和_next_seqno并不一致
        if (unwrap(frontSegment.header().seqno, _isn, _next_seqno) + frontSegment.length_in_sequence_space() <= unwrap(ackno, _isn, _next_seqno)) {
            _outstandingSegments.pop();
            popedSize += frontSegment.length_in_sequence_space();
            // 接收到更大的TCP之后将retransmission_timeout回复为初始值
            _current_retransmission_timeout = _initial_retransmission_timeout;
        } else {
            // 如果不能pop最早的segment，这里就要中断循环，否则就会出现死循环的情况
            break;
        }
    }

    _currentWindowSize = min(_currentWindowSize + popedSize, uint64_t(window_size));

    if (_currentWindowSize > 0) {
        fill_window();
    }

    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_outstandingSegments.empty()) {
        // 不存在outstanding的segment，不需要计时/重传
        _accumulated_timeout = 0;
        return;
    }

    _accumulated_timeout += ms_since_last_tick;

    if (_accumulated_timeout >= _current_retransmission_timeout) {
        // 积累的超时时间超过当前RTO，重置积累超时时间,将累计重传次数加一，并进行重传, 并将当前的重传时长加倍
        _accumulated_timeout = 0;
        _consecutiveRetryNum++;
        _current_retransmission_timeout *= 2;
        _segments_out.push(_outstandingSegments.front());
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    assert(!_outstandingSegments.empty());
    return _consecutiveRetryNum;
}

void TCPSender::send_empty_segment() {
    TCPSegment emptySegment;
    emptySegment.header().seqno = next_seqno();
    _segments_out.push(emptySegment);
}
