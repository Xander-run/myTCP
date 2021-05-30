#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    switch (_tcpState) {
        case LISTEN:
            if (seg.header().syn) { // TODO: handle illegal header situation, eg: syn&fin are both set
                _tcpState = SYN_RECV;
                _checkpoint = 0;
                _isn = seg.header().seqno;
            }
            break;
        case SYN_RECV: {
            bool eof = seg.header().fin;
            _reassembler.push_substring(seg.payload().copy(), unwrap(seg.header().seqno, _isn, _checkpoint) - 1, eof);
            _checkpoint = _reassembler.stream_out().bytes_written();
            if (_reassembler.stream_out().input_ended()) {
                _tcpState = FIN_RECV;
            }
            break;
        }
        case FIN_RECV: {
            break;
        }
        case ERROR:
            break;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    switch (_tcpState) {
        case LISTEN :
            return {};
        case SYN_RECV:
            return wrap(_reassembler.stream_out().bytes_written() + 1, _isn);
        case FIN_RECV:
            return wrap(_reassembler.stream_out().bytes_written() + 2, _isn);
        default:
            return {};
    }
}

size_t TCPReceiver::window_size() const {
    return _capacity + _reassembler.stream_out().bytes_read() - _reassembler.stream_out().bytes_written();
}
