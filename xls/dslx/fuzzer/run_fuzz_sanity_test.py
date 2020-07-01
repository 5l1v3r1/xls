# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Lint as: python3

"""Sanity run of a few samples in a single shard for the fuzzer."""

import random

from xls.dslx.fuzzer import ast_generator
from xls.dslx.fuzzer import run_fuzz
from absl.testing import absltest


class RunFuzzSanityTest(absltest.TestCase):

  KWARGS = {
      'calls_per_sample': 4,
      'save_temps': False,
      'sample_count': 8,
      'codegen': True
  }

  def _get_options(self) -> ast_generator.AstGeneratorOptions:
    return ast_generator.AstGeneratorOptions(disallow_divide=True)

  def test_a_few_samples(self):
    run_fuzz.run_fuzz(random.Random(0), self._get_options(), **self.KWARGS)


if __name__ == '__main__':
  absltest.main()
