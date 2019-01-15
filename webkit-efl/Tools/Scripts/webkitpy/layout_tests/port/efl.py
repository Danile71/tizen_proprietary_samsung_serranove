# Copyright (C) 2011 ProFUSION Embedded Systems. All rights reserved.
# Copyright (C) 2011 Samsung Electronics. All rights reserved.
# Copyright (C) 2012 Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""WebKit Efl implementation of the Port interface."""

import logging
import os
import signal
import subprocess

from webkitpy.layout_tests.models.test_configuration import TestConfiguration
from webkitpy.layout_tests.port.webkit import WebKitPort
from webkitpy.layout_tests.port.pulseaudio_sanitizer import PulseAudioSanitizer

_log = logging.getLogger(__name__)

#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
# Please do not enable jhbuild since Tizen is not using it
#class EflDriver(WebKitDriver):
    #def cmd_line(self, pixel_tests, per_test_args):
        #wrapper_path = self._port.path_from_webkit_base("Tools", "efl", "run-with-jhbuild")
        #return [wrapper_path] + WebKitDriver.cmd_line(self, pixel_tests, per_test_args)
#endif

class EflPort(WebKitPort, PulseAudioSanitizer):
    port_name = 'efl'
#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
    TARGET_LIST = ['U1SLP']
#endif

    def __init__(self, *args, **kwargs):
        WebKitPort.__init__(self, *args, **kwargs)

#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
# Please do not enable jhbuild since Tizen is not using it
        #self._jhbuild_wrapper_path = self.path_from_webkit_base('Tools', 'efl', 'run-with-jhbuild')
        #self.set_option_default('wrapper', self._jhbuild_wrapper_path)
#endif
        self.webprocess_cmd_prefix = self.get_option('webprocess_cmd_prefix')

    def _port_flag_for_scripts(self):
        return "--efl"

#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
# Please do not enable jhbuild since Tizen is not using it
    #def _driver_class(self):
        #return EflDriver
#endif

    def setup_test_run(self):
        self._unload_pulseaudio_module()

    def setup_environ_for_server(self, server_name=None):
        env = super(EflPort, self).setup_environ_for_server(server_name)
        # If DISPLAY environment variable is unset in the system
        # e.g. on build bot, remove DISPLAY variable from the dictionary
        if not 'DISPLAY' in os.environ:
            del env['DISPLAY']
        env['TEST_RUNNER_INJECTED_BUNDLE_FILENAME'] = self._build_path('lib', 'libTestRunnerInjectedBundle.so')
        env['TEST_RUNNER_PLUGIN_PATH'] = self._build_path('lib')
        if self.webprocess_cmd_prefix:
            env['WEB_PROCESS_CMD_PREFIX'] = self.webprocess_cmd_prefix
#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
        env['TEST_RUNNER_FONT_CONF_PATH'] = self._build_path('../..', 'Tools/DumpRenderTree/gtk/fonts')
#endif
        return env

    def default_timeout_ms(self):
        # Tests run considerably slower under gdb
        # or valgrind.
        if self.get_option('webprocess_cmd_prefix'):
            return 350 * 1000
        return super(EflPort, self).default_timeout_ms()

    def clean_up_test_run(self):
        self._restore_pulseaudio_module()

    def _generate_all_test_configurations(self):
        return [TestConfiguration(version=self._version, architecture='x86', build_type=build_type) for build_type in self.ALL_BUILD_TYPES]

#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
    # FIXME: Need to use system apache configuration. target -> obs -> sbs order.
    def _path_to_apache(self):
        if os.getenv('HOSTNAME') in self.TARGET_LIST:
            return self._filesystem.join('/wtr', 'apache', 'bin', 'httpd')
        if os.getenv('SBOX_SESSION_DIR') == None:
            return '/usr/local/apache2/bin/httpd'
        return self._filesystem.join(os.environ['HOME'], 'sbs-install', 'apache', 'bin', 'httpd')

    def _path_to_apache_config_file(self):
        if os.getenv('HOSTNAME') in self.TARGET_LIST:
            return self._filesystem.join(self.layout_tests_dir(), 'platform', 'efl-tizen', 'http', 'conf', 'apache2-target-httpd.conf')
        if os.getenv('SBOX_SESSION_DIR') == None:
            return self._filesystem.join(self.layout_tests_dir(), 'platform', 'efl-tizen', 'http', 'conf', 'apache2-suse-httpd.conf')
        return self._filesystem.join(self.layout_tests_dir(), 'platform', 'efl-tizen', 'http', 'conf', 'apache2-debian-httpd.conf')
#endif

    def _path_to_driver(self):
        return self._build_path('bin', self.driver_name())

    def _path_to_image_diff(self):
        return self._build_path('bin', 'ImageDiff')

#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
# Please do not enable jhbuild since Tizen is not using it
    #def _image_diff_command(self, *args, **kwargs):
        #return [self._jhbuild_wrapper_path] + super(EflPort, self)._image_diff_command(*args, **kwargs)
#endif

    # FIXME: I doubt EFL wants to override this method.
    def check_build(self, needs_http):
        return self._check_driver()

#if ENABLE(TIZEN_WEBKIT2_EFL_WTR)
    def _path_to_webcore_library(self):
        static_path = self._build_path('lib', 'libwebcore_efl.a')
        dyn_path = self._build_path('lib', 'libwebcore_efl.so')
        return static_path if self._filesystem.exists(static_path) else dyn_path
#endif

    def show_results_html_file(self, results_filename):
        # FIXME: We should find a way to share this implmentation with Gtk,
        # or teach run-launcher how to call run-safari and move this down to WebKitPort.
        run_launcher_args = ["file://%s" % results_filename]
        # FIXME: old-run-webkit-tests also added ["-graphicssystem", "raster", "-style", "windows"]
        # FIXME: old-run-webkit-tests converted results_filename path for cygwin.
        self._run_script("run-launcher", run_launcher_args)

#if ENABLE(TIZEN_WEBKIT_EFL_DRT) || ENABLE(TIZEN_WEBKIT2_EFL_WTR)
    def baseline_search_path(self):
        # FIXME: We need some option check(s) here to distinguish drt between open source and tizen.
        search_paths = []
        search_paths.append('efl-tizen')
        search_paths.append(self.name())
        if self.name() != self.port_name:
            search_paths.append(self.port_name)
        return map(self._webkit_baseline_path, search_paths)

    def _skipped_file_search_paths(self):
        search_paths = []
        # FIXME: We need some option check(s) here to distinguish drt between open source and tizen.
        search_paths.append('efl-tizen')
        return search_paths

    def path_to_test_expectations_file(self):
        return self._filesystem.join(self.layout_tests_dir(), 'platform', 'efl-tizen', 'TestExpectations')
#endif
