import os
import platform
import re
import subprocess
import tempfile

import lit.formats
import lit.util

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst
from lit.llvm.subst import FindTool

config.name = "ClangPlugins"
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)
config.suffixes = [".cpp"]
config.test_source_root = os.path.dirname(__file__)
# config.test_exec_root = os.path.join(config.clang_obj_root, "test")
llvm_config.use_default_substitutions()
llvm_config.use_clang()
# config.substitutions.append(("%src_include_dir", config.clang_src_dir + "/include"))
config.substitutions.append(("%target_triple", config.target_triple))
config.substitutions.append(("%PATH%", config.environment["PATH"]))

plugin_includes = " ".join(f"-I{s}" for s in config.plugin_includes.split(";"))
plugin_opts = " ".join(s.replace("-fplugin=", "-load ") for s in config.plugin_opts.split(";"))
config.substitutions.append(("%plugin_opts%", f"{plugin_opts} {plugin_includes}"))

tools = ["clang", "clang++"]
llvm_config.add_tool_substitutions(tools, config.llvm_tools_dir)


# import platform
# import lit.formats
# from lit.llvm import llvm_config
# from lit.llvm.subst import ToolSubst

# config.name = 'ClangPlugins'
# config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)
# config.suffixes = ['.cpp']
# config.test_source_root = os.path.dirname(__file__)
# config.substitutions.append(('%shlibext', config.llvm_shlib_ext))
# config.substitutions.append(('%shlibdir', config.llvm_shlib_dir))
