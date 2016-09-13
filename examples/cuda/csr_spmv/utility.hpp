#pragma once

#include <numeric>
#include <limits>
#include <vector>
#include <random>
#include <algorithm>


template<class IndexVector, class ValueVector>
void create_simple_csr_spmv_problem(IndexVector& row_offsets,
                                    IndexVector& column_indices,
                                    ValueVector& values,
                                    ValueVector& x,
                                    ValueVector& y)
{
  // construct a representation for the following matrix
  //    [10  0 20]
  //    [ 0  0  0]
  //    [ 0  0 30]
  //    [40 50 60]

  int num_columns = 3;
  int num_rows = 4;
  int num_nonzeros = 6;

  row_offsets.resize(num_rows + 1);
  row_offsets[0] = 0; // first offset is always 0
  row_offsets[1] = 2;
  row_offsets[2] = 2;
  row_offsets[3] = 3;
  row_offsets[4] = 6; // last offset is always num_nonzeros

  values.resize(num_nonzeros);
  column_indices.resize(num_nonzeros);

  column_indices[0] = 0; values[0] = 10;
  column_indices[1] = 2; values[1] = 20;
  column_indices[2] = 2; values[2] = 30;
  column_indices[3] = 0; values[3] = 40;
  column_indices[4] = 1; values[4] = 50;
  column_indices[5] = 2; values[5] = 60;

  x.resize(num_columns);
  std::fill(x.begin(), x.end(), 1);

  y.resize(num_rows);
  y[0] =  30;
  y[1] =   0;
  y[2] =  30;
  y[3] = 150;
}


template<class IndexVector, class ValueVector>
void laplacian_5pt(int  n,
                   int& num_rows,
                   int& num_columns,
                   IndexVector& row_offsets,
                   IndexVector& column_indices,
                   ValueVector& values)
{
  num_rows = n*n;
  num_columns = num_rows;
  int num_nonzeros = 5*n*n - 4*n;

  row_offsets.resize(num_rows + 1);
  column_indices.resize(num_nonzeros);
  values.resize(num_nonzeros);

  int nz = 0;

  for(int i = 0; i < n; ++i)
  {
    for(int j = 0; j < n; ++j)
    {
      int idx = n*i + j;

      if(i > 0)
      {
        column_indices[nz] = idx - n;
        values[nz] = -1;
        ++nz;
      }

      if(j > 0)
      {
        column_indices[nz] = idx - 1;
        values[nz] = -1;
        ++nz;
      }

      column_indices[nz] = idx;
      values[nz] = 4;
      ++nz;

      if(j < n - 1)
      {
        column_indices[nz] = idx + 1;
        values[nz] = -1;
        ++nz;
      }

      if(i < n - 1)
      {
        column_indices[nz] = idx + n;
        values[nz] = -1;
        ++nz;
      }
      
      row_offsets[idx + 1] = nz;
    }
  }
}


float max_relative_error(const float* a, const float* b, int n)
{
  float max_error = 0;

  float eps = std::sqrt(std::numeric_limits<float>::epsilon());

  for(int i = 0; i < n; ++i)
  {
    float error = std::abs(a[i] - b[i]);
    if(error != 0)
    {
      max_error = std::max(max_error, error/(std::abs(a[i]) + std::abs(b[i]) + eps));
    }
  }

  return max_error;
}


template<class Vector>
bool almost_equal(const Vector& a, const Vector& b,
                  typename Vector::value_type threshold = 5 * std::sqrt(std::numeric_limits<typename Vector::value_type>::epsilon()))
{
  return (a.size() == b.size()) && max_relative_error(a.data(), b.data(), a.size()) <= threshold;
}


template<class Vector>
void fill_random(Vector& vec, int seed = 13)
{
  using value_type = typename Vector::value_type;
  auto dist = std::uniform_real_distribution<value_type>(-1.0, 1.0);
  auto rng = std::bind(dist, std::default_random_engine());
  std::generate(vec.begin(), vec.end(), rng);
}

