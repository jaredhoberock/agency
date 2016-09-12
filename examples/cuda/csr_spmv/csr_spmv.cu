#include "utility.hpp"

#include <agency/agency.hpp>
#include <agency/cuda.hpp>

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

void parallel_csr_spmv(int num_rows,
                       const int* row_offsets,
                       const int* column_indices,
                       const float* values,
                       const float* x,
                       float *y)
{
  constexpr int block_size = 128;
  constexpr int warp_size = 32;

  // choose the number of blocks
  constexpr int warps_per_block = block_size / warp_size;
  constexpr int max_threads = 30 * 1024;
  constexpr int max_blocks = max_threads / block_size;
  int num_blocks = (num_rows + warps_per_block - 1) / warps_per_block;
  num_blocks = std::min(max_blocks, num_blocks);

  using namespace agency;

  bulk_invoke(cuda::grid(num_blocks, block_size), [=] __device__ (cuda::grid_agent& self)
  {
    __shared__ volatile float sdata[warps_per_block * warp_size + warp_size/2];      // padded to avoid reduction ifs
    __shared__ volatile int ptrs[warps_per_block][2];
    
    const int thread_id   = block_size * blockIdx.x + threadIdx.x;  // global thread index
    const int thread_lane = threadIdx.x & (warp_size-1);            // thread index within the warp
    const int warp_id     = thread_id   / warp_size;                // global warp index
    const int warp_lane   = threadIdx.x / warp_size;                // warp index within the CTA
    const int num_warps   = warps_per_block * gridDim.x;   // total number of active warps

    for(int row = warp_id; row < num_rows; row += num_warps)
    {
      // use two threads to fetch row_offsets[row] and row_offsets[row+1]
      // this is considerably faster than the straightforward version
      if(thread_lane < 2)
      {
        ptrs[warp_lane][thread_lane] = row_offsets[row + thread_lane];
      }

      const int row_start = ptrs[warp_lane][0];                   //same as: row_start = Ap[row];
      const int row_end   = ptrs[warp_lane][1];                   //same as: row_end   = Ap[row+1];

      // compute local sum
      float sum = 0;
      for(int jj = row_start + thread_lane; jj < row_end; jj += warp_size)
      {
        sum += values[jj] * x[column_indices[jj]];
      }

      // reduce local sums to row sum (ASSUME: warpsize 32)
      sdata[threadIdx.x] = sum;
      sdata[threadIdx.x] = sum = sum + sdata[threadIdx.x + 16]; 
      sdata[threadIdx.x] = sum = sum + sdata[threadIdx.x +  8];
      sdata[threadIdx.x] = sum = sum + sdata[threadIdx.x +  4];
      sdata[threadIdx.x] = sum = sum + sdata[threadIdx.x +  2];
      sdata[threadIdx.x] = sum = sum + sdata[threadIdx.x +  1];

      // first thread writes warp result
      if(thread_lane == 0)
      {
        y[row] += sdata[threadIdx.x];
      }
    } // end for row
  });
}


int main()
{
  // create shorthand for the vectors we will use in this program
  using index_vector = std::vector<int, agency::cuda::allocator<int>>;
  using value_vector = std::vector<float, agency::cuda::allocator<float>>;

  {
    // test our implementation's correctness on simple SPMV problem
    index_vector row_offsets;
    index_vector column_indices;
    value_vector values;

    value_vector x, reference;

    create_simple_csr_spmv_problem(row_offsets, column_indices, values, x, reference);
    int num_rows = row_offsets.size() - 1;

    value_vector y(num_rows);

    parallel_csr_spmv(num_rows, row_offsets.data(), column_indices.data(), values.data(), x.data(), y.data());

    std::cout << "y: [";
    for(int row = 0; row < y.size(); ++row)
    {
      std::cout << " " << y[row];
    }
    std::cout << " ]" << std::endl;

    assert(almost_equal(reference, y));
  }

  {
    index_vector row_offsets;
    index_vector column_indices;
    value_vector values;

    // create a CSR matrix
    int num_rows = 128;
    int num_columns = num_rows;
    laplacian_5pt(num_rows, row_offsets, column_indices, values);

    // XXX should generate random matrix values

    // XXX should generate random vector values
    value_vector x(num_columns, 1);

    // compute reference solution
    value_vector reference(num_rows);
    sequential_csr_spmv(num_rows, row_offsets.data(), column_indices.data(), values.data(), x.data(), reference.data());

    // multiply
    value_vector y(num_rows);
    parallel_csr_spmv(num_rows, row_offsets.data(), column_indices.data(), values.data(), x.data(), y.data());

    assert(almost_equal(reference, y));
  }

  std::cout << "OK" << std::endl;

  return 0;
}

