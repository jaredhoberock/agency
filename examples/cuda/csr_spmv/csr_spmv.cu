#include "utility.hpp"

#include <iostream>
#include <vector>
#include <cassert>

void sequential_csr_spmv(int num_rows,
                         const int* row_offsets,
                         const int* column_indices,
                         const float* values,
                         const float* x,
                         float *y)
{
  for(int row = 0; row < num_rows; ++row)
  {
    float dot = 0;
    
    int row_start = row_offsets[row];
    int row_end = row_offsets[row + 1];

    for(int value_idx = row_start; value_idx < row_end; ++value_idx)
    {
      dot += values[value_idx] * x[column_indices[value_idx]];
    }

    y[row] += dot;
  }
}


int main()
{
  std::vector<int> row_offsets;
  std::vector<int> column_indices;
  std::vector<float> values;

  std::vector<float> x, reference;

  create_simple_csr_spmv_problem(row_offsets, column_indices, values, x, reference);

  std::vector<float> y(x.size());

  sequential_csr_spmv(row_offsets.size() - 1, row_offsets.data(), column_indices.data(), values.data(), x.data(), y.data());

  assert(almost_equal(reference, y));

  std::cout << "y: [";
  for(int row = 0; row < y.size(); ++row)
  {
    std::cout << " " << y[row];
  }
  std::cout << " ]" << std::endl;

  std::cout << "OK" << std::endl;

  return 0;
}

