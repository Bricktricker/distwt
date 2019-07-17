#pragma once

#include <string>
#include <vector>

#include <distwt/common/histogram.hpp>
#include <distwt/common/wt.hpp>

class WaveletMatrixBase : public WaveletTreeBase {
protected:
    std::vector<size_t> m_z;

public:
    static inline std::string histogram_extension() {
        return "hist";
    }

    static inline std::string level_extension(size_t level) {
        return "lv_" + std::to_string(level+1);
    }

    static inline std::string z_extension() {
        return "z";
    }

    WaveletMatrixBase(const HistogramBase& hist);

    void save_z(const std::string& filename) const;

    inline size_t z(size_t level) const {
        return m_z[level];
    }
};
