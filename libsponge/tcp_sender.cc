#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timer(retx_timeout) {}

void TCPSender::_remove_acked_outstanding_segments() {
    for (auto iter = _segments_outstanding.begin(); iter != _segments_outstanding.end(); ) {
        TCPSegment tcp_segment = *iter;
        // if the `ackno` is greater than all of the sequence numbers in the segment
        if (_ackno >= unwrap(tcp_segment.header().seqno + tcp_segment.length_in_sequence_space(), _isn, _next_seqno)) {
            iter = _segments_outstanding.erase(iter);
        } else iter++;
    };
}

char TCPSender::_get_char_indexed_ackno() {
    for (auto iter = _segments_outstanding.begin(); iter != _segments_outstanding.end(); ) {
        TCPSegment tcp_segment = *iter;
        uint64_t start_no = unwrap(tcp_segment.header().seqno, _isn, _next_seqno);
        uint64_t end_no = start_no + tcp_segment.length_in_sequence_space();
        if (start_no <= _ackno && _ackno < end_no) {
            return tcp_segment.payload().at(0);
        }
    };
    return {};
}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t ans = 0;
    for (const TCPSegment& tcp_segment : _segments_outstanding) {
        uint64_t start_seqno = unwrap(tcp_segment.header().seqno, _isn, _next_seqno);
        uint64_t end_seqno = start_seqno + tcp_segment.length_in_sequence_space();
        if (_ackno < start_seqno)
            ans += end_seqno - start_seqno;
        else if (_ackno < end_seqno) 
            ans += end_seqno - _ackno;
        else
            cout << "Warning: redundent outstanding segments detected...";
    };
    return ans;
}

void TCPSender::fill_window() {
    cout << "fill_window: _ackno: " << _ackno << " next_seqno: " << _next_seqno << "window_size: " << _window_size << endl;
    // if the window size is zero, act like the window size is one
    // send a single byte that gets rejected by the receiver
    // if (_window_size == 0) {
    //     TCPSegment tcp_segment_to_send;
    //     tcp_segment_to_send.header().seqno = _isn + _ackno;

    //     std::string fake_data;
    //     fake_data += _get_char_indexed_ackno();

    //     Buffer buffer(std::move(fake_data));
    //     tcp_segment_to_send.payload() = buffer;
    //     _segments_out.push(tcp_segment_to_send);

    //     // start retransmission running
    //     // if (!_retransmission_timer.is_running())
    //     //     _retransmission_timer.restart();
    //     // else 
    //         // _retransmission_timer.start(_initial_retransmission_timeout);
    // } else 
    {
        // reads from ByteStream and sends as many bytes as possible in the form of TCPSegments
        // as long as there are new bytes to be read and spce available in the window
        while (_next_seqno - _ackno < max(_window_size, 1U)) {
            cout << "_ackno: " << _ackno << " _next_seqno: " << _next_seqno << endl;
            TCPSegment tcp_segment_to_send;
            if (!_syn_sent) {
                _syn_sent = tcp_segment_to_send.header().syn = true;
            };
            
            tcp_segment_to_send.header().seqno = _isn + _next_seqno;

            size_t data_len = min(max(_window_size, 1U) - (_next_seqno - _ackno) - tcp_segment_to_send.length_in_sequence_space(), TCPConfig::MAX_PAYLOAD_SIZE);
            data_len = min(data_len, _stream.buffer_size());
            
            // don't add FIN if this would make the segment exceed the receiver's window
            if (_stream.input_ended() && max(_window_size, 1U) - (_next_seqno - _ackno) > data_len && !_fin_sent) {
                _fin_sent = tcp_segment_to_send.header().fin = true;
            };
    
            std::string data = _stream.read(data_len);
            Buffer buffer(std::move(data));
            tcp_segment_to_send.payload() = buffer;
            cout << "len: " << data_len << " data: " << data << endl;
            // if the segment contains data, send it
            if (tcp_segment_to_send.length_in_sequence_space() > 0) {
                _segments_out.push(tcp_segment_to_send);
                _segments_outstanding.push_back(tcp_segment_to_send);

                // start retransmission running
                if (!_retransmission_timer.is_running() && _window_size)
                //     _retransmission_timer.restart();
                // else 
                    _retransmission_timer.start(_initial_retransmission_timeout);

                _next_seqno += tcp_segment_to_send.length_in_sequence_space();
            } else break;;
            // if there is no other input, break the loop
            if (_stream.buffer_empty())
                break;
        };
    }


}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size = window_size;
    // ignore impossible ackno (beyond next seqno) 
    if (unwrap(ackno, _isn, _next_seqno) > _next_seqno)
        return;
    // if ackno is greater than any previous ackno
    if (unwrap(ackno, _isn, _next_seqno) > _ackno) {
        // remove any that have now been fully acknowledged outstanding segments
        _ackno = unwrap(ackno, _isn, _next_seqno);

        _remove_acked_outstanding_segments();
        cout << "bytes_in_flight: " << bytes_in_flight() << endl;
        // fill the window again if new space has opened up
        cout << "fill_window, _stream.input_ended(): " << _stream.input_ended() <<  " _segments_outstanding.size(): " << _segments_outstanding.size() << endl;
        fill_window();
        // set the RTO back to its initial value
        _retransmission_timer.reset_rto(_initial_retransmission_timeout);
        // if the sender has any outstanding data, restart the retransmission timer
        if (!_segments_outstanding.empty()) {
            _retransmission_timer.start(_initial_retransmission_timeout);
        };
        // reset the count of `consecutive retransmissions` back to zero
        _count_consecutive_retransmissions = 0;
    };
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // retransmit the earliest segment that hasn't been fully ack by the TCP receiver
    _retransmission_timer.add(ms_since_last_tick);
    if (_retransmission_timer.is_expired()) {
        // find the earliest (lowest sequence number) segment
        if (!_segments_outstanding.empty()) {
            uint64_t lsn = UINT64_MAX;
            TCPSegment earliest_segment;
            for (const TCPSegment& tcp_segment : _segments_outstanding) {
                uint64_t seqno = unwrap(tcp_segment.header().seqno, _isn, _next_seqno);
                if (seqno < lsn) {
                    lsn = seqno;
                    earliest_segment = tcp_segment;
                };
            };
            _segments_out.push(earliest_segment);
        };

        cout << "window_size: " << _window_size << endl;
        // If the window size is nonzero: 
        if (_window_size > 0) {
            // increment the number of consecutive retransmissions, this will be used by TCPConnection
            _count_consecutive_retransmissions++;
            // double the value of RTO
            _retransmission_timer.double_rto();
        };
        cout << "rto: " << _retransmission_timer.get_rto() << endl;
        // reset the retransmission timer and start it
        _retransmission_timer.start(_retransmission_timer.get_rto());
    };
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _count_consecutive_retransmissions;
}

void TCPSender::send_empty_segment() {
    TCPSegment tcp_segment_to_send;
    _segments_out.push(tcp_segment_to_send);
}
