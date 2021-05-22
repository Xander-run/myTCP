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

    // todo: 超出部分做截断而不是直接返回
    // todo: eof情况下就算truncated也要更新maxIndex
    string truncatedData = data.substr(0, length - max(exceedCapacity(index, length), exceedEOF(index, length)));
    length = truncatedData.length();

    // todo: amxUnsaved和eof之后的上限
    if (_maxUnsavedIndex < int(index + length - 1)) {
        _maxUnsavedIndex = index + length - 1;
    }

    if (eof && _canMaxEndIndexChange) {
        _maxEndIndex = index + data.length() - 1;
        _canMaxEndIndexChange = false;
//        _output.end_input();
    }
    // todo: 只有capacity内的才能被推入
    for (size_t i = index; i < index + length; i++) {
        _saved[i % _capacity] = true;
        _chars[i % _capacity] = truncatedData[i - index];
    }

    int newMinNeededIndex = int(_minNeededIndex);
    string toWrite = "";

   for (; newMinNeededIndex <= _maxUnsavedIndex; newMinNeededIndex++) {
        if (toWrite.length() >= _output.remaining_capacity()) {
            break;
        }
        if (_saved[newMinNeededIndex % _capacity]) {
            _saved[newMinNeededIndex % _capacity] = false;
            toWrite += _chars[newMinNeededIndex % _capacity];
        } else {
            break;
        }
    }

    if (newMinNeededIndex > _maxUnsavedIndex) {
        if (!_canMaxEndIndexChange && newMinNeededIndex > _maxEndIndex) {  // todo
            if (!_saved[newMinNeededIndex % _capacity]) {
                _output.end_input();
            }
        } else {
            _maxUnsavedIndex = -1;
        }
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

int StreamReassembler::exceedCapacity(const size_t index, const size_t length) {
    int neededCapacity;
    size_t remainingCapacity = _output.remaining_capacity();  // todo
    if (_maxUnsavedIndex > 0) {
        // assembler中占用了一部分内存
        remainingCapacity -= _maxUnsavedIndex - _minNeededIndex + 1;
        neededCapacity = int(index + length - _maxUnsavedIndex - 1);
    } else {
        neededCapacity = int(index + length - _minNeededIndex);
    }

    return neededCapacity - int(remainingCapacity);
}

int StreamReassembler::exceedEOF(const size_t index, const size_t length) {
    return (int(index + length - 1) - _maxEndIndex) * (!_canMaxEndIndexChange);
}
