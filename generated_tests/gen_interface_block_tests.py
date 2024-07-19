#!/usr/bin/env python3
# coding=utf-8

# Copyright (c) 2020, 2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""Generate interface block linker tests"""

import os
import argparse
import sys

from mako import exceptions

from modules import utils
from textwrap import dedent
from templates import template_dir

TEMPLATES = template_dir(os.path.basename(os.path.splitext(__file__)[0]))

def get_array_index(size):
    return "[" + str(size) + "]" if size > 1 else ""

class BasicType(object):
    def __init__(self, type_name, subtype=None, size=0, array_size=1, qualifiers=''):
        self.size = size
        self.subtype = subtype
        self._qualifiers = qualifiers
        self.array_size = array_size
        self.name = type_name

    def __str__(self):
        return "{}{}".format(self.name, get_array_index(self.array_size))

    def qualifiers(self):
        all = set()
        base_type = self
        while base_type is not None:
            all.add(base_type._qualifiers)
            base_type = base_type.subtype
        result = " ".join(all).strip()
        return result + " " if len(result) else ""

    def depth(self):
        count = 0
        base_type = self
        while base_type.has_subtype():
            base_type = base_type.subtype
            count += 1
        return count

    def has_subtype(self):
        return self.subtype is not None

    def is_array(self):
        return self.array_size > 1


def apply_indent(text, prefix):
    result = ""
    for line in text.split('\n'):
        if len(line):
            result += "{}{}\n".format(prefix, line)
    return result.strip("\n")


def save_shader_text(filepath, shader_text):
    with open(filepath, 'w') as test_file:
        test_file.write(shader_text)


def get_tests_path(folder):
    return os.path.join("spec", "arb_enhanced_layouts", folder)


def get_index_var_name(dim):
    return chr(ord('a') + dim)


def get_layout(location_idx):
    return "layout(location = {}) ".format(location_idx) if location_idx else ""


def generate_assignment(base_type, dst, src):
    deref = ""
    dimension = 0
    dimension_num = base_type.depth()
    while base_type.has_subtype():
        if base_type.is_array():
            deref += "[a]"
        deref += "[{}]".format(get_index_var_name(dimension_num - dimension))
        base_type = base_type.subtype
        dimension += 1
    # `convert` is a predefined function in mako file
    return "{dst} += convert({src}{deref});".format(dst=dst, src=src,
                                                    deref=deref)

def generate_conversion_func(base_type):
    gen_text_mask="""\
                   for (int {idx} = {start}; {idx} < {len}; {idx}++) {{
                   {inner_text}
                   }}"""

    root_type = base_type
    dimension = 0
    dimension_num = base_type.depth()
    loop_code = generate_assignment(base_type, "color", "indata")
    while base_type.has_subtype():
        idxname = get_index_var_name(dimension_num - dimension)
        loop_code = dedent(gen_text_mask).format(idx=idxname,
                                                 start=0,
                                                 len=base_type.size,
                                                 inner_text=apply_indent(loop_code, " " * 4))
        base_type = base_type.subtype
        dimension += 1

    if root_type.is_array():
        loop_code = dedent(gen_text_mask).format(idx='a',
                                                 start=0,
                                                 len=root_type.array_size,
                                                 inner_text=apply_indent(loop_code, " " * 4))

    calc_color = dedent("""\
    vec4 calc_color(in {} indata{}) {{
        vec4 color = vec4(0.0);
    {}
        return color;
    }}
    """).format(root_type.name, get_array_index(root_type.array_size), apply_indent(loop_code, " " * 4))
    return calc_color


def create_args_parser():
    parser = argparse.ArgumentParser(
        description="Generate interface block linker tests")

    parser.add_argument(
        '--script',
        dest='script',
        action='store_true',
        default=False,
        help="Generate script running the tests (for development)")

    parser.add_argument(
        '--oob-verbose-mode',
        dest='oob_verbose_mode',
        action='store_true',
        default=False,
        help="Generate a lot of tests checking the out-of-bounds cases (for development)")
    return parser


class TestEnv(object):
    def __init__(self, extensions = []):
        self.extensions = extensions

    def generate_matching_test(self, test_types, filename, block_location):
        declare_var_template = "{qualifiers}{base_type} {prefix}{var_number}{array_size};\n"

        extensions = ["GL_ARB_enhanced_layouts",
                      "GL_ARB_separate_shader_objects",
                      "GL_ARB_arrays_of_arrays"] + self.extensions

        calc_color_func = ""
        declare_uniform_types = ""
        declare_vs_output_types = ""
        assign_vs_output_types = ""
        calc_output_color = ""

        unique_types=[]
        var_number=1
        for base_type in test_types:
            declare_uniform_types += declare_var_template.format(qualifiers = "uniform ",
                                                                 base_type = base_type.name,
                                                                 prefix="vs_uniform_",
                                                                 var_number = var_number,
                                                                 array_size = get_array_index(base_type.array_size))
            declare_vs_output_types += declare_var_template.format(qualifiers = base_type.qualifiers(),
                                                                   base_type = base_type.name,
                                                                   prefix="var_",
                                                                   var_number = var_number,
                                                                   array_size = get_array_index(base_type.array_size))
            assign_vs_output_types += ("vs_output.var_{var_number} = vs_uniform_{var_number};\n").format(var_number = var_number)
            calc_output_color += ("fs_output += calc_color(vs_output.var_{var_number});\n").format(var_number = var_number)
            if str(base_type) not in unique_types:
                unique_types.append(str(base_type))
                calc_color_func += generate_conversion_func(base_type)
            var_number+=1

        templ = TEMPLATES.get_template('basic.shader_test.mako')
        try:
            shadertext = templ.render_unicode(declare_uniform_types=declare_uniform_types,
                                            declare_vs_output_types=apply_indent(declare_vs_output_types, " " * 4),
                                            assign_vs_output_types=apply_indent(assign_vs_output_types, " " * 4),
                                            calc_output_color=apply_indent(calc_output_color, " " * 4),
                                            calc_color=calc_color_func,
                                            extensions=extensions,
                                            layout=get_layout(block_location))
        except:
            print(exceptions.text_error_template().render(), file=sys.stderr)
            raise
        save_shader_text(filename, shadertext)
        print(filename)

    def test_matching(self, test_types, filename):
        for block_location in [None, 1]:
            location_name = "_loc_{}".format(block_location) if block_location else ""
            fullname=get_tests_path("") + filename + location_name + ".shader_test"
            self.generate_matching_test(test_types, fullname, block_location)
        return self


def main():
    """Main function."""

    parser = create_args_parser()
    args = parser.parse_args()

    if args.script:
        print("#!/bin/bash")
        print("SHADER_RUNNER=bin/shader_runner")

    utils.safe_makedirs(get_tests_path(""))

    flat_qualifier="flat"
    ALL_ATTR_TYPES = [
        BasicType("float"),
        BasicType("vec2", BasicType("float"), 2),
        BasicType("vec3", BasicType("float"), 3),
        BasicType("vec4", BasicType("float"), 4),

        BasicType("int" , qualifiers=flat_qualifier),
        BasicType("ivec2", BasicType("int"), 2, qualifiers=flat_qualifier),
        BasicType("ivec3", BasicType("int"), 3, qualifiers=flat_qualifier),
        BasicType("ivec4", BasicType("int"), 4, qualifiers=flat_qualifier),

        BasicType("uint" , qualifiers=flat_qualifier),
        BasicType("uvec2", BasicType("uint"), 2, qualifiers=flat_qualifier),
        BasicType("uvec3", BasicType("uint"), 3, qualifiers=flat_qualifier),
        BasicType("uvec4", BasicType("uint"), 4, qualifiers=flat_qualifier),

        BasicType("mat2"  , BasicType("vec2", BasicType("float"), 2), 2),
        BasicType("mat3x2", BasicType("vec2", BasicType("float"), 2), 3),
        BasicType("mat4x2", BasicType("vec2", BasicType("float"), 2), 4),

        BasicType("mat2x3", BasicType("vec3", BasicType("float"), 3), 2),
        BasicType("mat3"  , BasicType("vec3", BasicType("float"), 3), 3),
        BasicType("mat4x3", BasicType("vec3", BasicType("float"), 3), 4),

        BasicType("mat2x4", BasicType("vec4", BasicType("float"), 4), 2),
        BasicType("mat3x4", BasicType("vec4", BasicType("float"), 4), 3),
        BasicType("mat4"  , BasicType("vec4", BasicType("float"), 4), 4),
    ]

    ALL_ATTR_TYPES_FP64 = [
        BasicType("double", qualifiers=flat_qualifier),
        BasicType("dvec2", BasicType("double"), 2, qualifiers=flat_qualifier),
        BasicType("dvec3", BasicType("double"), 3, qualifiers=flat_qualifier),
        BasicType("dvec4", BasicType("double"), 4, qualifiers=flat_qualifier),

        BasicType("dmat2"  , BasicType("dvec2", BasicType("double"), 2), 2, qualifiers=flat_qualifier),
        BasicType("dmat3x2", BasicType("dvec2", BasicType("double"), 2), 3, qualifiers=flat_qualifier),
        BasicType("dmat4x2", BasicType("dvec2", BasicType("double"), 2), 4, qualifiers=flat_qualifier),

        BasicType("dmat2x3", BasicType("dvec3", BasicType("double"), 3), 2, qualifiers=flat_qualifier),
        BasicType("dmat3"  , BasicType("dvec3", BasicType("double"), 3), 3, qualifiers=flat_qualifier),
        BasicType("dmat4x3", BasicType("dvec3", BasicType("double"), 3), 4, qualifiers=flat_qualifier),

        BasicType("dmat2x4", BasicType("dvec4", BasicType("double"), 4), 2, qualifiers=flat_qualifier),
        BasicType("dmat3x4", BasicType("dvec4", BasicType("double"), 4), 3, qualifiers=flat_qualifier),
        BasicType("dmat4"  , BasicType("dvec4", BasicType("double"), 4), 4, qualifiers=flat_qualifier),
    ]

    ALL_ATTR_TYPES_64BIT = [
        BasicType("int64_t", qualifiers=flat_qualifier),
        BasicType("i64vec2", BasicType("int64_t"), 2, qualifiers=flat_qualifier),
        BasicType("i64vec3", BasicType("int64_t"), 3, qualifiers=flat_qualifier),
        BasicType("i64vec4", BasicType("int64_t"), 4, qualifiers=flat_qualifier),

        BasicType("uint64_t", qualifiers=flat_qualifier),
        BasicType("u64vec2", BasicType("uint64_t"), 2, qualifiers=flat_qualifier),
        BasicType("u64vec3", BasicType("uint64_t"), 3, qualifiers=flat_qualifier),
        BasicType("u64vec4", BasicType("uint64_t"), 4, qualifiers=flat_qualifier),
    ]


    for i in range(0, 3, 1):
        ATTR_TYPES = []
        for j in range(0 + i, 21 + i, 3):
            ATTR_TYPES.append(ALL_ATTR_TYPES[j])
        TestEnv().test_matching(ATTR_TYPES, ("matching_basic_types_{}").format(i + 1))


    for i in range(0, 2, 1):
        ATTR_TYPES = []
        for j in range(0, 4, 1):
            ATTR_TYPES.append(ALL_ATTR_TYPES_64BIT[i + 2 * j])
        TestEnv(["GL_ARB_gpu_shader_int64"]).test_matching(ATTR_TYPES, ("matching_64bit_types_{}").format(i + 1))

    for i in range(0, 3, 1):
        ATTR_TYPES = [ALL_ATTR_TYPES_FP64[0],
                      ALL_ATTR_TYPES_FP64[1],
                      ALL_ATTR_TYPES_FP64[2],
                      ALL_ATTR_TYPES_FP64[3]]

        ATTR_TYPES.append(ALL_ATTR_TYPES_FP64[4 + i])
        ATTR_TYPES.append(ALL_ATTR_TYPES_FP64[7 + i])
        ATTR_TYPES.append(ALL_ATTR_TYPES_FP64[10 + i])
        TestEnv(["GL_ARB_gpu_shader_fp64"]).test_matching(ATTR_TYPES, ("matching_fp64_types_{}").format(i + 1))


    #test https://gitlab.freedesktop.org/mesa/mesa/-/issues/3320
    ATTR_TYPES = [
        BasicType("vec2", BasicType("float"), 2),
        BasicType("vec3", BasicType("float"), 3),
        BasicType("float" , qualifiers=flat_qualifier),
        BasicType("vec3", BasicType("float"), 3, array_size=3),
        BasicType("vec4", BasicType("float"), 4),
        BasicType("vec3", BasicType("float"), 3),
        BasicType("vec3", BasicType("float"), 3),
    ]
    TestEnv().test_matching(ATTR_TYPES, ("matching_basic_types_custom"))

    #TestEnv(args, ATTR_TYPES, get_tests_path("execution/io-block-oob/basic-types"), [], True).test_oob()
    #TestEnv(args, ATTR_TYPES_FP64, get_tests_path("execution/io-block-oob/fp64-types"), ["GL_ARB_gpu_shader_fp64"], True).test_oob()
    #TestEnv(args, ATTR_TYPES_64BIT, get_tests_path("execution/io-block-oob/64bit-types"), ["GL_ARB_gpu_shader_int64"], True).test_oob()


if __name__ == '__main__':
    main()
