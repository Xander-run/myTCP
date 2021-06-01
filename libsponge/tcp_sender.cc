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

uint64_t TCPSender::bytes_in_flight() const {
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
                TCPSegment finSegment
                segments_out().push();
                _tcpSenderState = FIN_SENT;
                return;
            } else if (_currentWindowSize <= 0) {
                return;
            }
        }
            break;
        case FIN_SENT:
            break;
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
    // 1. 更新_currentWindowSize, 并删除_outstandingSegments
    // 2. 如果有_currentWindowSize ！= 0， fillwindow
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
        // 积累的超时时间超过当前RTO，重置积累超时时间,将累计重传次数加一，并进行重传
        _accumulated_timeout = 0;
        _consecutiveRetryNum++;
        consecutive_retransmissions();
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
