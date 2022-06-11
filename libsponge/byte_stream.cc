#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : cap{capacity}, bytesread{0}, byteswritten{0}, endin{false} {}

size_t ByteStream::write(const string &data) {
    size_t rmn_cap = this->remaining_capacity();
    size_t writebytes = rmn_cap < data.size() ? rmn_cap : data.size();

    this->bytestream.append(data.substr(0, writebytes));
    this->byteswritten += writebytes;
    return writebytes;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const { return this->bytestream.substr(0, len); }

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    this->bytestream.erase(0, len);
    this->bytesread += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string ans = this->bytestream.substr(0, len);
    pop_output(len);
    return ans;
}

void ByteStream::end_input() { this->endin = true; }

bool ByteStream::input_ended() const { return this->endin; }

size_t ByteStream::buffer_size() const { return this->bytestream.size(); }

bool ByteStream::buffer_empty() const { return this->bytestream.empty(); }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return this->byteswritten; }

size_t ByteStream::bytes_read() const { return this->bytesread; }

size_t ByteStream::remaining_capacity() const { return this->cap - this->bytestream.size(); }
