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
    this->_chars = other._chars; // FIXME
    this->_saved = other._saved; // FIXME
    this->_output = other._output;
    this->_canMaxEndIndexChange = other._canMaxEndIndexChange;
    return *this;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t length = data.length();

    // 根据eof和capacity将data进行截断
    string truncatedData = data.substr(0, length - max(0, max(exceedCapacity(index, length), exceedEOF(index, length))));
    length = truncatedData.length();

    // 设置eof部分，这里采取可以多次设置eof的方式
    if (eof) {
        _eofIndex = index + data.length() - 1;
    }

    // 更新标志左右区间的Index
    if (_beginIndex > index) {
        if (index < _maxStreamedIndex) {
            _beginIndex = _maxStreamedIndex;
        } else {
            _beginIndex = index;
        }
    }
    _endIndex = _endIndex > index + length ? _endIndex : index + length;

    // 将data的数据读入缓存数组，并更新数组状态, 为避免更新已经输出的Index位置的数据, 此处用_beginIndex作为开始值
    for (size_t i = _beginIndex; i < index + length; i++) {
        _saved[i % _capacity] = true;
        _chars[i % _capacity] = truncatedData[i - index];
    }

    // 获取能够写入_output的字符串
    string toWrite = "";
    for (size_t i = _maxStreamedIndex; i < _endIndex; i++) {
        if (_saved[i % _capacity]) {
            _saved[i % _capacity] = false;
            toWrite += _chars[i % _capacity];
        } else {
            break;
        }
    }

    // 更新_beginIndex
    while (_beginIndex < _endIndex) {
        if (!_saved[_beginIndex % _capacity]) {
            _beginIndex++;
        } else {
            break;
        }
    }

    // 将字符串写入，并更新_maxStreamed
    _output.write(toWrite);
    _maxStreamedIndex += toWrite.length();
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t total = 0;
    for (size_t i = _maxStreamedIndex; i <_endIndex; i++) {
        total += _saved[i %_capacity];
    }
    return total;
}

bool StreamReassembler::empty() const {
    // FIXME: not sure, should i consider eof?
    return _beginIndex == _endIndex;
}

// 返回超出容量的字符数量，没超出则返回 <= 0 的数
int StreamReassembler::exceedCapacity(const size_t index, const size_t length) {
    int neededCapacity = index + length - _endIndex;
    size_t remainingCapacity = _output.remaining_capacity() - (_endIndex - _maxStreamedIndex);
    return neededCapacity - int(remainingCapacity);
}

// 返回超出EOF的字符数量，没超出则返回 <= 0 的数
int StreamReassembler::exceedEOF(const size_t index, const size_t length) {
    if (_eofIndex < 0) {
        return -1;
    } else {
        return index + length - _eofIndex;
    }
}
