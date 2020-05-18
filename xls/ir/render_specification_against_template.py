# Lint as: python3
#
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

"""Renders the specification metadata against the commandline-provided template.

Output C++ source code is printed to stdout.
"""

from absl import app

import jinja2

from xls.common import runfiles
from xls.ir import op_specification as specification


def main(argv):
  env = jinja2.Environment(undefined=jinja2.StrictUndefined)
  template = env.from_string(runfiles.get_contents_as_text(argv[1]))
  print('// DO NOT EDIT: this file is AUTOMATICALLY GENERATED and should not '
        'be changed.')
  print(template.render(spec=specification))


if __name__ == '__main__':
  app.run(main)