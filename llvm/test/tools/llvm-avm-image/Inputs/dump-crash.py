# Runs the repository's AVD1 inspector, so the lit tests and the command-line
# tool share one independent decoder. This file lives seven directories below
# the repository root.
import os
import runpy
import sys

here = os.path.dirname(os.path.abspath(__file__))
tool = os.path.join(here, *([os.pardir] * 7), "tools", "dump-crash.py")
sys.argv[0] = tool
runpy.run_path(tool, run_name="__main__")
