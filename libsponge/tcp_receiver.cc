#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // set the initial sequence number if necessary
    if (seg.header().syn) {
        _init_seqno.emplace(seg.header().seqno);
        _syn_received = true;
        _fin_received = false;
    };
    if (seg.header().fin) {
        _fin_received = true;
        _next_ackno.reset();
    };

    if (_syn_received) {
        const std::string data = seg.payload().copy();
        const uint64_t index = unwrap(seg.header().seqno, _init_seqno.value(), 0);
        const bool eof = seg.header().fin;

        _reassembler.push_substring(data, index, eof);

        if (_next_ackno.has_value())
            _next_ackno.emplace(_next_ackno.value() + seg.payload().size() + seg.header().syn + seg.header().fin);
        else
            _next_ackno.emplace(_init_seqno.value() + seg.payload().size() + seg.header().syn + seg.header().fin);
    };
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    return _next_ackno;
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}
