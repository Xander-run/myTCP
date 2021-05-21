#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _saved(new bool[capacity]), _chars(new char[capacity]) {
//    this->_saved = new bool[capacity]();
//    this->_chars = new char[capacity];
}

StreamReassembler::StreamReassembler(const StreamReassembler& other) : _output(other._capacity), _capacity(other._capacity), _saved(new bool[other._capacity]), _chars(new char[other._capacity]) {

}

StreamReassembler& StreamReassembler::operator=(const StreamReassembler& other) {
    this->_capacity = other._capacity;
    this->_maxUnsavedIndex = other._maxUnsavedIndex;
    this->_chars = other._chars; // FIXME
    this->_saved = other._saved; // FIXME
    this->_minNeededIndex = other._minNeededIndex;
    this->_output = other._output;
    this->_canMaxEndIndexChange = other._canMaxEndIndexChange;
    this->_maxEndIndex = other._maxEndIndex;
    return *this;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t length = data.length();
    if (exceedCapacity(index, length) || exceedEOF(index, length)) {
        return;
    }

    if (_maxUnsavedIndex < int(index + length - 1)) {
        _maxUnsavedIndex = index + length - 1;
    }

    if (eof && _canMaxEndIndexChange) {
        _maxEndIndex = index + length - 1;
        _canMaxEndIndexChange = false;
//        _output.end_input();
    }

    for (size_t i = index; i < index + length; i++) {
        _saved[i % _capacity] = true;
        _chars[i % _capacity] = data[i - index];
    }

    int newMinNeededIndex = int(_minNeededIndex);
    string toWrite = "";

    for (; newMinNeededIndex <= _maxUnsavedIndex; newMinNeededIndex++) {
        if (_saved[newMinNeededIndex % _capacity]) {
            _saved[newMinNeededIndex % _capacity] = false;
            toWrite += _chars[newMinNeededIndex % _capacity];
        } else {
            break;
        }
    }

    if (newMinNeededIndex > _maxUnsavedIndex) {
        _maxUnsavedIndex = -1;
    }

    _minNeededIndex = newMinNeededIndex;
    _output.write(toWrite);
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t total = 0;
    for (int i = int(_minNeededIndex); i <= _maxUnsavedIndex; i++) {
        total += _saved[i % _capacity];
    }
    return total;
}

bool StreamReassembler::empty() const {
    return int(_minNeededIndex) > _maxEndIndex;
}

bool StreamReassembler::exceedCapacity(const size_t index, const size_t length) {
    size_t neededCapacity;
    size_t remainingCapacity = _capacity - (_output.buffer_size() - _output.remaining_capacity());
    if (_maxUnsavedIndex > 0) {
        remainingCapacity -= _maxUnsavedIndex - _minNeededIndex + 1;
        neededCapacity = index + length - _maxUnsavedIndex - 1;
    } else {
        neededCapacity = index + length - _minNeededIndex - 1;
    }

    return neededCapacity > remainingCapacity;
}

bool StreamReassembler::exceedEOF(const size_t index, const size_t length) {
    return int(index + length - 1) > _maxEndIndex && !_canMaxEndIndexChange;
}
