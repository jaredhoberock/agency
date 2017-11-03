/// \file
/// \brief Include this file to use the bulk_invoke() family of functions.
///

#pragma once

///
/// \defgroup control_structures Control Structures
/// \brief Control structures create execution.
///
///
/// The primary way Agency programs create execution is by invoking a
/// **control structure**. Control structures are functions invoked via
/// composition with an **execution policy**. Execution policies
/// parameterize control structures by describing the properties of the
/// requested execution.
///
/// For example, the following code snipped uses the bulk_invoke() control
/// structure with the \ref par execution policy to require the parallel execution
/// of ten invocations of a lambda function:
///
/// ~~~~{.cpp}
/// using namespace agency;
///
/// bulk_invoke(par(10), [](parallel_agent& self)
/// {
///   // task body here
///   ...
/// });
/// ~~~~

#include <agency/detail/config.hpp>
#include <agency/functional/bulk_invoke/bulk_invoke.hpp>
#include <agency/functional/bulk_invoke/default_bulk_invoke.hpp>

