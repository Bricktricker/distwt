#include "mpi_launcher.hpp"

#include <string>
#include <vector>

#include <distwt/common/util.hpp>
#include <distwt/common/wt_sequential.hpp>

#include <distwt/mpi/file_partition_reader.hpp>

#include <distwt/mpi/histogram.hpp>
#include <distwt/mpi/effective_alphabet.hpp>
#include <distwt/mpi/bit_vector.hpp>

#include <distwt/mpi/wt_nodebased.hpp>
#include <distwt/mpi/wm.hpp>

#include <distwt/mpi/result.hpp>

class mpi_wm_dd {
public:

template<typename sym_t>
static void start(
    MPIContext& ctx,
    const std::string& input_filename,
    const size_t prefix,
    const size_t in_rdbufsize,
    const std::string& local_filename,
    const std::string& output) {

    Result::Time time;
    double t0 = ctx.time();

    auto dt = [&](){
        const double t = ctx.time();
        const double dt = t - t0;
        t0 = t;
        return dt;
    };

    // Determine input partition
    FilePartitionReader<sym_t> input(ctx, input_filename, prefix);
    const size_t local_num = input.local_num();
    const size_t rdbufsize = (in_rdbufsize > 0) ? in_rdbufsize : local_num;

    if(local_filename.length() > 0) {
        // Extract local part
        ctx.cout_master() << "Extract partition to "
            << local_filename << " ..." << std::endl;

        input.extract_local(local_filename, rdbufsize);

        // Synchronize
        ctx.cout_master() << "Synchronizing ..." << std::endl;
        ctx.synchronize();
    }

    time.input = dt();

    // Compute histogram
    ctx.cout_master() << "Compute histogram ..." << std::endl;
    Histogram<sym_t> hist(ctx, input, rdbufsize);

    time.hist = dt();

    // Compute effective alphabet
    EffectiveAlphabet<sym_t> ea(hist);

    // Transform text and cache in RAM
    ctx.cout_master() << "Compute effective transformation ..." << std::endl;
    std::vector<sym_t> etext(local_num);
    {
        size_t i = 0;
        ea.transform(input, [&](sym_t x){ etext[i++] = x; }, rdbufsize);
    }

    time.eff = dt();

    // local construction
    ctx.cout_master() << "Compute local WTs ..." << std::endl;
    auto wt_nodes = WaveletTreeNodebased(hist,
    [&](WaveletTree::bits_t& bits, const WaveletTreeBase& wt){

        bits.resize(wt.num_nodes());
        wt_pc<sym_t, idx_t>(wt, bits, etext);
    });

    // Clean up
    etext.clear();
    etext.shrink_to_fit();

    // Synchronize
    ctx.cout_master() << "Done. Synchronizing ..." << std::endl;
    ctx.synchronize();

    time.construct = dt();

    // Merge
    WaveletMatrix wm = wt_nodes.merge_to_matrix(ctx, input, hist, true);
    time.merge = dt();

    // write to disk if needed
    if(output.length() > 0) {
        ctx.synchronize();
        ctx.cout_master() << "Writing WM to disk ..." << std::endl;

        if(ctx.rank() == 0) {
            hist.save(output + "." + WaveletMatrixBase::histogram_extension());
            wm.save_z(output +  + "." + WaveletMatrixBase::z_extension());
        }

        wm.save(ctx, output);
    }

    // Synchronize for exit
    ctx.cout_master() << "Waiting for exit signals ..." << std::endl;
    ctx.synchronize();

    // gather stats
    Result result("mpi-wm-dd", ctx, input, wm.sigma(), time);

    ctx.cout_master() << result.readable() << std::endl
                      << result.sqlplot() << std::endl;
}

};

int main(int argc, char** argv) {
    return mpi_launch<mpi_wm_dd>(argc, argv);
}
