"""Run the fetched GLAD package even with an isolated embeddable Python."""

import runpy
import sys

sys.path.insert(0, sys.argv.pop(1))
runpy.run_module("glad", run_name="__main__")
