#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    ByteStream _output;          //!< The reassembled in-order byte stream
    size_t _capacity;            //!< The maximum number of bytes
    bool _canMaxEndIndexChange = true;
    bool *_saved;                //!< bitmap of taken indexes
    char *_chars;                //!<  the value in the
    // TODO: refactor
    bool _eofIsSet = false;         // 表示是否已经设置过EOF
    uint64_t _beginIndex = 0;         // assembler中存储了的部分开始的Index, 包含
    uint64_t _endIndex = 0;           // assembler中存储了的部分结束的结束的Index, 不包含
    uint64_t _eofIndex = 0;           // eof对应的Index, 0则表示还没设置, 不可以取到
    uint64_t _nextIndexToStream = 0;   // 下一个要输出给stream的index

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    StreamReassembler(const StreamReassembler& other);

    StreamReassembler& operator=(const StreamReassembler& other);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    int exceedCapacity(const size_t index, const size_t length); // return the exceed amount

    int exceedEOF(const size_t index, const size_t length);     // return the exceed amount
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
