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
        // just accept the first syn
        if (_syn_received) return;

        _syn_received = true;
        // init `_init_seqno` and `_next_ackno`
        _init_seqno.emplace(seg.header().seqno);
        _next_ackno.emplace(_init_seqno.value() + 1);
    };

    if (_syn_received) {
        // push substring into StreamReassembler
        const std::string data = seg.payload().copy();
        const uint64_t stream_index = unwrap(seg.header().seqno + seg.header().syn, _init_seqno.value(), _reassembler.get_abs_seqno()) - 1;
        const bool eof = seg.header().fin;
        _reassembler.push_substring(data, stream_index, eof);
        // evaluate next ackno
        _next_ackno.emplace(wrap(_reassembler.get_abs_seqno(), _init_seqno.value()) + 1);
        
        if (_reassembler.empty())
            _next_ackno.emplace(_next_ackno.value() + 1);
    };
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    return _next_ackno;
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}
