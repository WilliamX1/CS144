#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity{capacity} {}

void StreamReassembler::discard_substrings() {
    // discard the part exceeding the capacity
    size_t rmn_cap = _output.remaining_capacity(), uabd_bytes = unassembled_bytes();
    while (uabd_bytes > rmn_cap) {
        auto iter = _substrings.rbegin();
        if (uabd_bytes - rmn_cap >= iter->data.size()) {
            // directly discard the whole piece
            _substrings.pop_back();
            uabd_bytes -= iter->data.size();
        } else {
            // discard part of the piece
            iter->data.erase(iter->data.begin() + uabd_bytes - rmn_cap);
            uabd_bytes = rmn_cap;
            break;
        };
    };
}

void StreamReassembler::merge_substrings() {
    auto iter_prev = _substrings.begin(), iter_next = iter_prev;
    if (iter_next != _substrings.end())
        ++iter_next;

    while (iter_next != _substrings.end()) {
        uint64_t prev_end = iter_prev->index + iter_prev->data.size();
        uint64_t next_begin = iter_next->index, next_end = iter_next->index + iter_next->data.size();
        // prev_begin <= next_begin < next_end <= prev_end
        if (next_end <= prev_end)
            iter_next = _substrings.erase(iter_next);
        else if (next_begin <= prev_end) {
            // prev_begin <= next_begin <= prev_end < next_end
            iter_prev->data.append(iter_next->data.substr(prev_end - next_begin));
            iter_next = _substrings.erase(iter_next);
        } else {
            // not overlap
            iter_prev++;
            iter_next++;
        };
    };
    return;
}

void StreamReassembler::write_substrings() {
    while (!_substrings.empty()) {
        auto iter = _substrings.begin();
        size_t idx_begin = iter->index, idx_end = iter->index + iter->data.size();
        if (_nextbyte >= idx_end)
            _substrings.erase(iter);
        else if (_nextbyte < idx_begin)
            break;
        else {
            // just regard the piece as a incoming part and reuse the function to handle it
            const size_t writebytes = _output.write(iter->data.substr(_nextbyte - idx_begin));
            _nextbyte += writebytes;
            _substrings.erase(iter);
        };
    };
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t index_begin = index, index_end = index + data.size();
    if (eof) {
        _eof = true;
        _end_index = index_end;
    };

    // already have read it or store it in the ByteStream
    // simply discard it
    if (_nextbyte >= index_end) {
    } else if (index_begin <= _nextbyte && _nextbyte < index_end) {
        // directly write it into the ByteStream as much as we can
        const size_t writebytes = _output.write(data.substr(_nextbyte - index_begin));
        _nextbyte += writebytes;

        write_substrings();
    } else if (_nextbyte < index_begin) {
        // insert and combine the part into the _substrings, find the correct position to insert
        auto iter = _substrings.begin();
        while (iter != _substrings.end()) {
            if (iter->index >= index_begin)
                break;
            else
                iter++;
        };

        _substrings.insert(iter, substring(data, index, eof));
    };

    merge_substrings();
    discard_substrings();
    if (empty())
        _output.end_input();
    return;
}

size_t StreamReassembler::unassembled_bytes() const {
    // the substring in vector must not overlap
    size_t cnt = 0;
    for (const substring sub : _substrings)
        cnt += sub.data.size();
    return cnt;
}

bool StreamReassembler::empty() const { return _eof && _nextbyte >= _end_index; }
