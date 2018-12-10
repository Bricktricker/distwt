#include <distwt/mpi/result.hpp>

#include <tlx/math/abs_diff.hpp>

Result::Result(
    const std::string& algo,
    const MPIContext& ctx,
    const FilePartitionReader& input,
    const size_t alphabet,
    const double time_input,
    const double time_construct) {

    m_algo = algo;
    m_nodes = ctx.num_nodes();
    m_workers_per_node = ctx.num_workers_per_node();
    m_input = input.filename();
    m_size = input.total_size();
    m_alphabet = alphabet;
    m_time_input = time_input;
    m_time_construct = time_construct;

    // gather distributed stats
    m_memory = ctx.gather_max_alloc();

    auto traffic = ctx.gather_traffic();
    m_traffic = traffic.tx + traffic.tx_est;
    m_traffic_asym = tlx::abs_diff(
        traffic.tx + traffic.tx_est,
        traffic.rx + traffic.rx_est);
}
