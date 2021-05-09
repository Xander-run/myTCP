#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {
    this->_capacity = capacity;
}

size_t ByteStream::write(const string &data) {
    size_t length = data.length();
    if (length > _capacity - this->buffer.size()) {
        length = _capacity - this->buffer_size();
    }
    for (size_t i = 0; i < length; i++) {
        buffer.push_back(data[i]);
    }
    this->bytesWritten += length;
    return length;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length = len;
    if (length > this->buffer.size()) {
        length = this->buffer_size();
    }
    return string().assign(this->buffer.begin(), this->buffer.begin() + length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t length = len;
    if (length > this->buffer.size()) {
        length = this->buffer.size();
    }
    for (size_t i = 0; i < length; i++) {
        this->buffer.pop_front();
    }
    this->bytesRead += length;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t length = len;
    if (length > this->buffer.size()) {
        length = this->buffer.size();
    }
    string res = string().assign(this->buffer.begin(), this->buffer.begin() + length);
    for (size_t i = 0; i < length; i++) {
        this->buffer.pop_front();
    }
    this->bytesRead += length;
    return res;
}

void ByteStream::end_input() {
    this->inputEnded = true;
}

bool ByteStream::input_ended() const {
    return this->inputEnded;
}

size_t ByteStream::buffer_size() const {
    return this->buffer.size();
}

bool ByteStream::buffer_empty() const {
    return buffer_size() == 0;
}

bool ByteStream::eof() const {
    return buffer_empty() && input_ended();
}

size_t ByteStream::bytes_written() const {
    return this->bytesWritten;
}

size_t ByteStream::bytes_read() const {
    return this->bytesRead;
}

size_t ByteStream::remaining_capacity() const {
    return this->_capacity - buffer_size();
}
