#ifndef CPP_BASE_DATA_STRUCT_COUNT_MIN_SKETCH_H_
#define CPP_BASE_DATA_STRUCT_COUNT_MIN_SKETCH_H_

#include <glog/logging.h>

#include "cpp-base/hash/hash.h"
#include "cpp-base/integral_types.h"
#include "cpp-base/template_util.h"
#include "cpp-base/type_traits.h"
#include "cpp-base/macros.h"

namespace cpp_base {

// Count-Min Sketch can be based on a matrix of uint8, uint16 or uint32 cells.
// This struct helps enforcing this when instantiating from CountMinSketch.
template <class T> struct valid_type_for_cms;
template<> struct valid_type_for_cms<uint8> {};
template<> struct valid_type_for_cms<uint16> {};
template<> struct valid_type_for_cms<uint32> {};

// Keeps an approximate count for each key using a Count-Min Sketch. Template
// variable T must be either uint8, uint16, or uint32. Then, the sketch can
// count up to 255, 65,535, and 4,294,967,295 respectively. Beware of overflow.
// This class is NOT thread-safe.
template <class T>
class CountMinSketch {
  public:
    // delta: probability of error. The magnitude of error (a.k.a. epsilon)
    // is computed from the given memory budget and delta.
    CountMinSketch(int64 mem_budget_bytes, double delta)
            : num_rows_(ceil(log(1. / delta))),
              num_cols_(mem_budget_bytes / sizeof(T) / num_rows_),
              sum_counts_(0) {
        CHECK_GT(mem_budget_bytes, 0);
        CHECK_GT(delta, 0);
        CHECK_GT(num_rows_, 0);
        CHECK_GT(num_cols_, 0);

        matrix_ = new T*[num_rows_];
        for (int64 row = 0; row < num_rows_; ++row) {
            matrix_[row] = new T[num_cols_];
            memset(matrix_[row], 0, sizeof(T) * num_cols_);
        }

        double epsilon = exp(1.) / num_cols_;
        LOG(INFO) << "Inited a " << num_rows_ << "-by-" << num_cols_
                  << " count min sketch with epsilon=" << epsilon
                  << " and delta=" << delta << " and mem_gb="
                  << num_rows_ * num_cols_ * sizeof(T) / 1024. / 1024 / 1024;
    }

    ~CountMinSketch() {
        for (int64 row = 0; row < num_rows_; ++row)
            delete[] matrix_[row];
        delete[] matrix_;
    }

    // Returns the current count for the key.
    T GetCount(uint64 key) const {
        T min = std::numeric_limits<T>::max();
        for (int64 row = 0; row < num_rows_; ++row) {
            int64 col = Hash64NumWithSeed(key, row + 1) % num_cols_;
            min = std::min(min, matrix_[row][col]);
        }
        return min;
    }

    // Returns the current (updated) count for the key.
    T AddCount(uint64 key, int64 inc) {
        const T max = std::numeric_limits<T>::max();
        T min = max;
        for (int64 row = 0; row < num_rows_; ++row) {
            int64 col = Hash64NumWithSeed(key, row + 1) % num_cols_;
            int64 value = matrix_[row][col] + inc;
            // Don't allow going below 0 -- no negative counts. Also watch for overflow for type T.
            if (value < 0) value = 0;
            if (value > max) value = max;
            matrix_[row][col] = value;
            if (value < min) min = value;
        }
        sum_counts_ += inc;
        return min;
    }

    // Return the current (updated) count for the key.
    T Increment(uint64 key) { return AddCount(key, 1); }
    T Decrement(uint64 key) { return AddCount(key, -1); }

    void Clear() {
        for (int64 row = 0; row < num_rows_; ++row)
            memset(matrix_[row], 0, sizeof(T) * num_cols_);
        sum_counts_ = 0;
    }

    int64 SumCounts() const { return sum_counts_; }

    // Prints a distribution of the values of the count-min sketch cells.
    void DumpDistrOfCellValues() const {
        // Find out the frequency of values 1, 2, ..., 100 in CMS cells.
        vector<int64> freq(100);
        int64 sum_freqs = 0;
        for (int64 row = 0; row < num_rows_; ++row) {
            for (int64 col = 0; col < num_cols_; ++col) {
                int64 value = matrix_[row][col];
                if (value < freq.size() - 1)
                    freq[value]++;
                else
                    freq.back()++;
                ++sum_freqs;
            }
        }

        // Print into a stringstream and then into LOG(INFO).
        std::stringstream ss;
        for (int i = 0; i < freq.size(); ++i) {
            // Print the value, the raw and the normalized (percentage) frequency.
            double percent = freq[i] * 100. / sum_freqs;
            int old_len = ss.str().length();
            ss << i << ": " << freq[i] << " (" << percent << "%): ";
            int new_len = ss.str().length();

            // Show some bars representing the percentage value.
            int padding = std::max(25 - (new_len - old_len), 0);
            for (int j = 0; j < padding; ++j) { ss << ' '; }
            for (int j = 0; j < std::round(percent); ++j) { ss << '*'; }
            ss << '\n';
        }
        LOG(INFO) << "Count-min sketch distribution of cell values:\n" << ss.str();
    }

  private:
    const int64 num_rows_;
    const int64 num_cols_;
    T** matrix_;
    int64 sum_counts_;  // Overall sum across all increments and decrements

    // Dummy variable to catch invalid <T> instantiations at compile/link time.
    valid_type_for_cms<T> im_here_to_catch_instantiations_with_invalid_types_;

    DISALLOW_COPY_AND_ASSIGN(CountMinSketch);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_COUNT_MIN_SKETCH_H_
