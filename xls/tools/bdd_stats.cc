// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <limits>

#include "absl/status/status.h"
#include "absl/time/clock.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/init_xls.h"
#include "xls/common/integral_types.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"
#include "xls/common/status/statusor.h"
#include "xls/examples/sample_packages.h"
#include "xls/ir/ir_parser.h"
#include "xls/ir/package.h"
#include "xls/passes/bdd_function.h"

const char* kUsage = R"(
Builds a BDD from XLS IR and prints various metrics about the BDD. Usage:

To gather BDD stats of an IR file:
   bdd_stats <ir_file>

To gather BDD stats of a set of benchmarks:
   bdd_stats --benchmarks=sha256,crc32
   bdd_stats --benchmarks=all
)";

ABSL_FLAG(int64, bdd_minterm_limit, 0,
          "Maximum number of minterms before truncating the BDD subgraph "
          "and declaring a new variable. If zero, then no limit.");
ABSL_FLAG(std::vector<std::string>, benchmarks, {},
          "Comma-separated list of benchmarks gather BDD stats about.");

namespace xls {
namespace {

// Return list of pairs of {name, Package} for the specified bechmarks.
xabsl::StatusOr<std::vector<std::pair<std::string, std::unique_ptr<Package>>>>
GetBenchmarks(absl::Span<const std::string> benchmark_names) {
  std::vector<std::pair<std::string, std::unique_ptr<Package>>> packages;
  std::vector<std::string> names;
  if (benchmark_names.size() == 1 && benchmark_names.front() == "all") {
    XLS_ASSIGN_OR_RETURN(names, sample_packages::GetBenchmarkNames());
  } else {
    names = std::vector<std::string>(benchmark_names.begin(),
                                     benchmark_names.end());
  }
  for (const std::string& name : names) {
    XLS_ASSIGN_OR_RETURN(
        std::unique_ptr<Package> package,
        sample_packages::GetBenchmark(name, /*optimized=*/true));
    packages.push_back({name, std::move(package)});
  }
  return packages;
}

absl::Status RealMain(absl::string_view input_path) {
  std::vector<std::pair<std::string, std::unique_ptr<Package>>> packages;
  if (absl::GetFlag(FLAGS_benchmarks).empty()) {
    XLS_QCHECK(!input_path.empty());
    std::string path;
    if (input_path == "-") {
      path = "/dev/stdin";
    } else {
      path = std::string(input_path);
    }
    XLS_ASSIGN_OR_RETURN(std::string contents, GetFileContents(path));
    XLS_ASSIGN_OR_RETURN(std::unique_ptr<Package> package,
                         Parser::ParsePackage(contents, path));
    packages.push_back({path, std::move(package)});
  } else {
    XLS_ASSIGN_OR_RETURN(packages,
                         GetBenchmarks(absl::GetFlag(FLAGS_benchmarks)));
  }

  absl::Duration total_time;
  for (const auto& pair : packages) {
    const std::string& name = pair.first;
    const auto& package = pair.second;
    if (packages.size() > 1) {
      // Use endl to flush cout so the banner appears before starting work on
      // the BDD.
      std::cout << "================== " << name << std::endl;
    }
    XLS_ASSIGN_OR_RETURN(Function * entry, package->EntryFunction());
    absl::Time start = absl::Now();
    XLS_ASSIGN_OR_RETURN(
        std::unique_ptr<BddFunction> bdd_function,
        BddFunction::Run(entry, absl::GetFlag(FLAGS_bdd_minterm_limit)));
    absl::Duration bdd_time = absl::Now() - start;
    total_time += bdd_time;
    std::cout << "BDD construction time: " << bdd_time << "\n";
    std::cout << "BDD node count: " << bdd_function->bdd().size() << "\n";
    std::cout << "BDD variable count: " << bdd_function->bdd().variable_count()
              << "\n";

    int64 number_bits = 0;
    for (Node* node : entry->nodes()) {
      number_bits += node->GetType()->GetFlatBitCount();
    }
    std::cout << "Bits in graph: " << number_bits << "\n";

    int64 max_minterms = 0;
    for (int64 i = 0; i < bdd_function->bdd().size(); ++i) {
      max_minterms = std::max(
          max_minterms, bdd_function->bdd().minterm_count(BddNodeIndex(i)));
    }
    if (max_minterms == std::numeric_limits<int32>::max()) {
      std::cout << "Maximum minterms of any expression: INT32_MAX\n";
    } else {
      std::cout << "Maximum minterms of any expression: " << max_minterms
                << "\n";
    }
  }

  if (packages.size() > 1) {
    std::cout << "\nTotal construction time: " << total_time << "\n";
  }

  return absl::OkStatus();
}

}  // namespace
}  // namespace xls

int main(int argc, char** argv) {
  std::vector<absl::string_view> positional_arguments =
      xls::InitXls(kUsage, argc, argv);

  if (positional_arguments.empty() && absl::GetFlag(FLAGS_benchmarks).empty()) {
    XLS_LOG(QFATAL) << absl::StreamFormat(
        "Expected invocation:\n  %s <path>\n  %s "
        "--benchmarks=<benchmark-names>",
        argv[0], argv[0]);
  }

  XLS_QCHECK_OK(xls::RealMain(positional_arguments[0]));
  return EXIT_SUCCESS;
}
