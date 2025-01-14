#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>
#include <vector>

//! \brief The "sender" part of a TCP implementation.

//! TCP Retransmission Timer
class TCPTimer {
  private:
    //! whether the timer is running
    bool _is_running;

    //! microseconds since the timer started, reset to zero when the timer starts or restarts
    size_t _ms_since_started;

    //! retransmission timer for the connection currently
    size_t _current_retransmission_timeout;

  public:
    TCPTimer(size_t initial_retransmission_timeout) : _is_running{false}, _ms_since_started{0}, _current_retransmission_timeout{initial_retransmission_timeout} {};

    bool is_running() const {
      return _is_running;
    };

    void start(size_t initial_retransmission_timeout) {
      _is_running = true;
      _ms_since_started = 0;
      _current_retransmission_timeout = initial_retransmission_timeout;
    };

    void restart() {
      _ms_since_started = 0;
    };

    void stop() {
      _is_running = false;
    };

    void add(size_t ms_since_last_tick) {
      _ms_since_started += ms_since_last_tick;
    };

    bool is_expired() const {
      return _is_running && (_ms_since_started >= _current_retransmission_timeout);
    };

    void double_rto() {
      _current_retransmission_timeout <<= 1;
    };

    void reset_rto(size_t initial_retransmission_timeout) {
      _current_retransmission_timeout = initial_retransmission_timeout;
    };

    size_t get_rto() const {
      return _current_retransmission_timeout;
    };
};

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! outbound queue of segments that the TCPSerder wants to resend
    std::vector<TCPSegment> _segments_outstanding{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    //! the (absolute) sequence number for the ack byte
    uint64_t _ackno{0};

    //! the retransmission timer
    TCPTimer _retransmission_timer;

    //! the number of consecutive retransmissions
    unsigned int _count_consecutive_retransmissions{0};

    //! the window size, init with 1
    unsigned int _window_size{1};

    //! indicate whether already send SYN, make it true after sending the segment with SYN
    bool _syn_sent{false};

    //! indicate whether already send FIN, make it true after sending the segment with FIN
    bool _fin_sent{false};

    //! remove any that have now been fully acknowledged outstanding segments
    void _remove_acked_outstanding_segments();

    //! get the char indexed at the first byte of the segment which contains _ackno, used for fill_window when window_size is zero
    char _get_char_indexed_ackno() const;

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
