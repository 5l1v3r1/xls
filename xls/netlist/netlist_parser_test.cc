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

#include "xls/netlist/netlist_parser.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/strings/substitute.h"
#include "xls/common/status/matchers.h"
#include "xls/netlist/fake_cell_library.h"

namespace xls {
namespace netlist {
namespace rtl {
namespace {

TEST(NetlistParserTest, EmptyModule) {
  std::string netlist = R"(module main(); endmodule)";
  Scanner scanner(netlist);
  XLS_ASSERT_OK_AND_ASSIGN(CellLibrary cell_library, MakeFakeCellLibrary());
  XLS_ASSERT_OK_AND_ASSIGN(std::unique_ptr<Netlist> n,
                           Parser::ParseNetlist(&cell_library, &scanner));
  XLS_ASSERT_OK_AND_ASSIGN(const Module* m, n->GetModule("main"));
  EXPECT_EQ("main", m->name());
}

TEST(NetlistParserTest, EmptyModuleWithComment) {
  std::string netlist = R"(
// This is a module named main.
module main();
  // This area left intentionally blank.
endmodule)";
  Scanner scanner(netlist);
  XLS_ASSERT_OK_AND_ASSIGN(CellLibrary cell_library, MakeFakeCellLibrary());
  XLS_ASSERT_OK_AND_ASSIGN(std::unique_ptr<Netlist> n,
                           Parser::ParseNetlist(&cell_library, &scanner));
  XLS_ASSERT_OK_AND_ASSIGN(const Module* m, n->GetModule("main"));
  EXPECT_EQ("main", m->name());
}

TEST(NetlistParserTest, WireMultiDecl) {
  std::string netlist = R"(module main();
  wire foo, bar, baz;
endmodule)";
  Scanner scanner(netlist);
  XLS_ASSERT_OK_AND_ASSIGN(CellLibrary cell_library, MakeFakeCellLibrary());
  XLS_ASSERT_OK_AND_ASSIGN(std::unique_ptr<Netlist> n,
                           Parser::ParseNetlist(&cell_library, &scanner));
  XLS_ASSERT_OK_AND_ASSIGN(const Module* m, n->GetModule("main"));

  XLS_ASSERT_OK_AND_ASSIGN(NetRef foo, m->ResolveNet("foo"));
  EXPECT_EQ("foo", foo->name());
  XLS_ASSERT_OK_AND_ASSIGN(NetRef bar, m->ResolveNet("bar"));
  EXPECT_EQ("bar", bar->name());
  XLS_ASSERT_OK_AND_ASSIGN(NetRef baz, m->ResolveNet("baz"));
  EXPECT_EQ("baz", baz->name());
}

TEST(NetlistParserTest, InverterModule) {
  std::string netlist = R"(module main(a, z);
  input a;
  output z;
  INV inv_0(.A(a), .ZN(z));
endmodule)";
  Scanner scanner(netlist);
  XLS_ASSERT_OK_AND_ASSIGN(CellLibrary cell_library, MakeFakeCellLibrary());
  XLS_ASSERT_OK_AND_ASSIGN(std::unique_ptr<Netlist> n,
                           Parser::ParseNetlist(&cell_library, &scanner));
  XLS_ASSERT_OK_AND_ASSIGN(const Module* m, n->GetModule("main"));
  EXPECT_EQ("main", m->name());
  XLS_ASSERT_OK_AND_ASSIGN(NetRef a, m->ResolveNet("a"));
  EXPECT_EQ("a", a->name());
  XLS_ASSERT_OK_AND_ASSIGN(NetRef z, m->ResolveNet("z"));
  EXPECT_EQ("z", z->name());

  XLS_ASSERT_OK_AND_ASSIGN(Cell * c, m->ResolveCell("inv_0"));
  EXPECT_EQ(cell_library.GetEntry("INV").value(), c->cell_library_entry());
  EXPECT_EQ("inv_0", c->name());
}

TEST(NetlistParserTest, AOI21WithMultiBitInput) {
  std::string netlist = R"(module main(i, o);
  input [2:0] i;
  output o;
  AOI21 aoi21_0(.A(i[2]), .B(i[1]), .C(i[0]), .ZN(o));
endmodule)";
  Scanner scanner(netlist);
  XLS_ASSERT_OK_AND_ASSIGN(CellLibrary cell_library, MakeFakeCellLibrary());
  XLS_ASSERT_OK_AND_ASSIGN(std::unique_ptr<Netlist> n,
                           Parser::ParseNetlist(&cell_library, &scanner));
  XLS_ASSERT_OK_AND_ASSIGN(const Module* m, n->GetModule("main"));
  EXPECT_EQ("main", m->name());
  XLS_ASSERT_OK_AND_ASSIGN(NetRef i0, m->ResolveNet("i[0]"));
  EXPECT_EQ("i[0]", i0->name());
  XLS_ASSERT_OK_AND_ASSIGN(NetRef i1, m->ResolveNet("i[1]"));
  EXPECT_EQ("i[1]", i1->name());
  XLS_ASSERT_OK_AND_ASSIGN(NetRef i2, m->ResolveNet("i[2]"));
  EXPECT_EQ("i[2]", i2->name());
  EXPECT_THAT(m->ResolveNet("i[3]"),
              status_testing::StatusIs(
                  absl::StatusCode::kNotFound,
                  ::testing::HasSubstr("Could not find net: i[3]")));

  XLS_ASSERT_OK_AND_ASSIGN(Cell * c, m->ResolveCell("aoi21_0"));
  EXPECT_EQ(cell_library.GetEntry("AOI21").value(), c->cell_library_entry());
  EXPECT_EQ("aoi21_0", c->name());
}

TEST(NetlistParserTest, NumberFormats) {
  std::string netlist = R"(module main();
  wire z0, z1, z2, z3, z4, z5, z6, z7, z8, z9, z10, z11;
  INV inv_0(.A(10), .ZN(z0));
  INV inv_1(.A(1'b1), .ZN(z1));
  INV inv_2(.A(1'o1), .ZN(z2));
  INV inv_3(.A(1'd1), .ZN(z3));
  INV inv_4(.A(1'h1), .ZN(z4));
  INV inv_5(.A(1'B1), .ZN(z5));
  INV inv_6(.A(1'O1), .ZN(z6));
  INV inv_7(.A(1'D1), .ZN(z7));
  INV inv_8(.A(1'H1), .ZN(z8));
  INV inv_9(.A(10'o777), .ZN(z9));
  INV inv_10(.A(20'd100), .ZN(z10));
  INV inv_11(.A(30'hbeef), .ZN(z11));
endmodule)";

  Scanner scanner(netlist);
  XLS_ASSERT_OK_AND_ASSIGN(CellLibrary cell_library, MakeFakeCellLibrary());
  XLS_ASSERT_OK_AND_ASSIGN(std::unique_ptr<Netlist> n,
                           Parser::ParseNetlist(&cell_library, &scanner));
  XLS_ASSERT_OK_AND_ASSIGN(const Module* m, n->GetModule("main"));
  EXPECT_EQ("main", m->name());
}

TEST(NetlistParserTest, MoreNumberFormats) {
  auto make_module = [](const std::string& number) {
    std::string module_base = R"(module main();
wire z0;
INV inv_0(.A($0), .ZN(z0));
endmodule)";
    return absl::Substitute(module_base, number);
  };

  std::vector<std::pair<std::string, int64>> test_cases({
      {"1'b1", 1},
      {"1'o1", 1},
      {"8'd255", 255},
      {"8'sd127", 127},
      {"8'sd255", -1},
      {"8'sd253", -3},
  });

  // For each test case, make sure we can find a netlist for the given number
  // (matching the Verilog number string) in the module.
  for (const auto& test_case : test_cases) {
    std::string module_text = make_module(test_case.first);
    Scanner scanner(module_text);
    XLS_ASSERT_OK_AND_ASSIGN(CellLibrary cell_library, MakeFakeCellLibrary());
    XLS_ASSERT_OK_AND_ASSIGN(std::unique_ptr<Netlist> n,
                             Parser::ParseNetlist(&cell_library, &scanner));
    XLS_ASSERT_OK_AND_ASSIGN(const Module* m, n->GetModule("main"));
    XLS_ASSERT_OK(m->ResolveNumber(test_case.second).status());
  }
}

}  // namespace
}  // namespace rtl
}  // namespace netlist
}  // namespace xls
