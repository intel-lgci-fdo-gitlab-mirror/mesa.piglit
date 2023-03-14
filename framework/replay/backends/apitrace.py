# coding=utf-8
#
# Copyright (c) 2014, 2016-2017, 2019-2020 Intel Corporation
# Copyright © 2019, 2021 Collabora Ltd.
# Copyright © 2019-2020 Valve Corporation.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# SPDX-License-Identifier: MIT


""" Module providing an apitrace dump backend for replayer """

import subprocess
import sys
from os import path

from framework import core, exceptions

from .abstract import DumpBackend, dump_handler
from .register import Registry

_LOOP_TIMES = core.get_option('PIGLIT_REPLAY_LOOP_TIMES',
                              ('replay', 'loop_times'),
                              default='150')

__all__ = [
    'REGISTRY',
    'APITraceBackend',
]


def _collect_frame_times(stream):
    """
    - `stream` Output stream of `apitrace replay` with the `--pframes "opengl:GPU Duration"` option enabled.
    - `return` The frame times collected from parsing the stream.
    """
    assert len(stream) > 0
    # Store frame times as we read lines
    frame_times = []

    for line in stream:
        # Look for frame events
        if not line.startswith('frame'):
            continue
        # Lines of the form "frame	22156000"
        fields = line.rstrip('\r\n').split('\t')
        frame_time = int(fields[1])
        frame_times.append(frame_time)

    return frame_times[-int(_LOOP_TIMES):]


def _run_command(cmd: str, env: dict) -> subprocess.CompletedProcess:
    ret: subprocess.CompletedProcess = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env
    )
    logoutput = f"[profile_trace] Running: {' '.join(cmd)}\n".encode()
    print(logoutput.decode(errors='replace'))
    if ret.returncode:
        # Print the command logs if the process returns with an error code.
        stdout = ret.stdout.decode(errors="replace")
        print(f"[profile_trace] {stdout}")
        stderr = ret.stderr.decode(errors="replace")
        print(f"[profile_trace] {stderr}")
        raise RuntimeError(
            f'[profile_trace] Process failed with error code: {ret.returncode}'
        )
    return ret


class APITraceBackend(DumpBackend):
    """ replayer's apitrace dump backend

    This backend uses apitrace for replaying its traces.

    It supports OpenGL/ES traces, to be replayed with eglretrace, and DXGI
    traces, to be replayed with d3dretrace on Wine.

    However, the binary paths to the apitrace, eglretrace, d3dretrace and wine
    binaries are configurable. Hence, using wine could be omitted, for example.

    """

    def __init__(self, trace_path, output_dir=None, calls=None, **kwargs):
        super(APITraceBackend, self).__init__(trace_path, output_dir, calls,
                                              **kwargs)
        extension = path.splitext(self._trace_path)[1]

        if extension == '.trace':
            eglretrace_bin = core.get_option('PIGLIT_REPLAY_EGLRETRACE_BINARY',
                                             ('replay', 'eglretrace_bin'),
                                             default='eglretrace')
            self._retrace_cmd = [eglretrace_bin]
        elif extension == '.trace-dxgi':
            wine_bin = core.get_option('PIGLIT_REPLAY_WINE_BINARY',
                                       ('replay', 'wine_bin'),
                                       default='wine')
            wine_d3dretrace_bin = core.get_option(
                'PIGLIT_REPLAY_WINE_D3DRETRACE_BINARY',
                ('replay', 'wine_d3dretrace_bin'),
                default='d3dretrace')
            self._retrace_cmd = [wine_bin, wine_d3dretrace_bin]
        else:
            raise exceptions.PiglitFatalError(
                'Invalid trace_path: "{}" tried to be dumped '
                'by the APITraceBackend.\n'.format(self._trace_path))

    def _get_last_frame_call(self):
        cmd_wrapper = self._retrace_cmd[:-1]
        if cmd_wrapper:
            apitrace_bin = core.get_option(
                'PIGLIT_REPLAY_WINE_APITRACE_BINARY',
                ('replay', 'wine_apitrace_bin'),
                default='apitrace')
        else:
            apitrace_bin = core.get_option('PIGLIT_REPLAY_APITRACE_BINARY',
                                           ('replay', 'apitrace_bin'),
                                           default='apitrace')
        cmd = cmd_wrapper + [apitrace_bin,
                             'dump', '--calls=frame', self._trace_path]
        ret = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=sys.stderr)
        logoutput = '[dump_trace_images] Running: {}\n'.format(
            ' '.join(cmd)) + ret.stdout.decode(errors='replace')
        print(logoutput)
        for line in reversed(ret.stdout.decode(errors='replace').splitlines()):
            split = line.split(None, 1)
            if split and split[0].isnumeric():
                return int(split[0])
        return -1

    @dump_handler
    def dump(self):
        outputprefix = '{}-'.format(path.join(self._output_dir,
                                              path.basename(self._trace_path)))
        if not self._calls:
            self._calls = [str(self._get_last_frame_call())]
        if core.get_option('PIGLIT_REPLAY_OPTIONS', ('replay', 'options'), default=None) is not None:
            self._retrace_cmd += [core.get_option('PIGLIT_REPLAY_OPTIONS', ('replay', 'options'))]
        cmd = self._retrace_cmd + ['--headless',
                                   '--snapshot=' + ','.join(self._calls),
                                   '--snapshot-prefix=' + outputprefix,
                                   self._trace_path]
        self._run_logged_command(cmd, None)

    @staticmethod
    def profile(trace_path):
        return APITraceBackend(trace_path)._profile()

    def _profile(self):
        # We need to run in singlethread mode to avoid a use after free bug
        # in which the OpenGL context is queried for further results after
        # it has been destroyed in the replay thread when replay finishes
        cmd = self._retrace_cmd + ['--headless',
                                   '--benchmark',
                                   '--singlethread',
                                   '--loop=%s' % _LOOP_TIMES,
                                   '--pframes',
                                   'opengl:GPU Duration',
                                   self._trace_path]
        ret = _run_command(cmd, None)
        return _collect_frame_times(ret.stdout.decode().splitlines())


REGISTRY = Registry(
    extensions=['.trace', '.trace-dxgi'],
    backend=APITraceBackend,
)
