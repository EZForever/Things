#!/usr/bin/env python2
# svgshot.py - Volatility Framework extension that draws screenshots in SVG, thus supports Unicode characters
# Requirements: volatility, svgwrite

# Code modified from https://github.com/volatilityfoundation/volatility/blob/master/volatility/plugins/gui/screenshot.py
# And https://gist.github.com/gumblex/ab4b0563e22b65ebedf125297f47b0df
# ----------

# Volatility
# Copyright (C) 2007-2013 Volatility Foundation
# Copyright (C) 2010,2011,2012 Michael Hale Ligh <michael.ligh@mnin.org>
# Copyright (C) 2009 Brendan Dolan-Gavitt 
#
# This file is part of Volatility.
#
# Volatility is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Volatility is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Volatility.  If not, see <http://www.gnu.org/licenses/>.
#

import os

import volatility.plugins.gui.editbox as editbox
import volatility.plugins.malware.cmdhistory as cmdhistory
import volatility.plugins.gui.windowstations as windowstations
import volatility.debug as debug

try:
    import svgwrite, svgwrite.container
    has_svgwrite = True
except ImportError:
    has_svgwrite = False

EMBED_CSS = '''
.children {}

.window {}

.window.visible {}

.window.hidden {
    display: none;
}

.window > .border {
    fill: white;
    stroke: black;
}

.window > .client {
    fill: white;
    stroke: gray;
}

.window > .caption {
    fill: black;
}

.editbox {}

.editbox > .text {
    fill: gray;
    font-family: monospace;
}

.editbox > .undo {
    fill: gray;
    display: none;
    font-family: monospace;
}

.console {}

.console > .text {
    fill: gray;
    font-family: monospace;
}

.console > .history {
    fill: gray;
    display: none;
    font-family: monospace;
}
'''

class SVGShot(windowstations.WndScan):
    """Save a pseudo-screenshot based on GDI windows"""

    # Embedded commands
    CmdEditbox = None
    CmdConsoles = None

    def __init__(self, config, *args, **kwargs):
        windowstations.WndScan.__init__(self, config, *args, **kwargs)

        config.add_option("DUMP-DIR", short_option = 'D', type = "string",
                          help = "Output directory", action = "store")
        config.add_option("EDITBOX", default = False, action = "store_true",
                          help = "Process editbox texts")
        config.add_option("CONSOLE", default = False, action = "store_true",
                          help = "Process console texts")
        
        self.CmdEditbox = editbox.Editbox(config)
        self.CmdConsoles = cmdhistory.Consoles(config)

    def win_class(self, win):
        return ' '.join(('window', 'visible' if 'WS_VISIBLE' in str(win.style) else 'hidden'))
    
    def win_desc(self, win):
        # '$IMAGE:$PID#$HWND'
        return '%s:%d#%d' % (str(win.head.pti.ppi.Process.ImageFileName), win.head.pti.ppi.Process.UniqueProcessId, win.head.h)

    def svg_multiline(self, svg, container, text, x, y, class_):
        lines = text.split('\r\n')
        svgText = svg.text(lines[0], insert = (x, y), class_ = class_)
        for line in lines[1 : ]:
            svgText.add(svg.tspan(line, x = [x], dx = [0], dy = ['1em']))
        container.add(svgText)

    def render_text(self, outfd, data):

        if not has_svgwrite:
            debug.error("Please install svgwrite")

        if not self._config.DUMP_DIR or not os.path.isdir(self._config.DUMP_DIR):
            debug.error("Please supply an existing --dump-dir")

        # Run embedded commands and save results
        if self._config.EDITBOX:
            editboxes = [ctrl for _, _, _, _, _, ctrl in self.CmdEditbox.calculate()]
        if self._config.CONSOLE:
            consoles = [(task, console) for task, console in self.CmdConsoles.calculate()]

        seen = []

        for window_station in data:
            for desktop in window_station.desktops():

                offset = desktop.PhysicalAddress
                if offset in seen:
                    continue
                seen.append(offset)

                # The foreground window 
                win = desktop.DeskInfo.spwnd

                # Some desktops don't have any windows
                if not win:
                    debug.warning("{0}\{1}\{2} has no windows".format(
                        desktop.dwSessionId, window_station.Name, desktop.Name))
                    continue

                svg = svgwrite.Drawing(size = (unicode(win.rcWindow.right + 1), unicode(win.rcWindow.bottom + 1)))
                svg.add(svgwrite.container.Style(EMBED_CSS))

                levelGroups = []
                lastlevel = -1

                # Traverse windows
                for win, level in desktop.windows(win = win):

                    if level > lastlevel:
                        # Current window is a child of last one, create a new group
                        svgGroup = svgwrite.container.Group(class_ = 'children')
                        levelGroups.append(svgGroup)
                    elif level < lastlevel:
                        # children enum finished, collapse children groups
                        for _ in range(lastlevel, level, -1):
                            svgGroup = levelGroups.pop()
                            levelGroups[-1].elements[-1].add(svgGroup)

                    svgGroup = svgwrite.container.Group(class_ = self.win_class(win))
                    svgGroup.set_desc(desc = self.win_desc(win))
                    
                    winSize = win.rcWindow.get_tup()
                    cliSize = win.rcClient.get_tup()
                    
                    l, t, r, b = winSize
                    svgGroup.add(svg.rect((unicode(l), unicode(t)), (unicode(r - l), unicode(b - t)), class_ = 'border'))
                    if(winSize != cliSize):
                        l, t, r, b = cliSize
                        svgGroup.add(svg.rect((unicode(l), unicode(t)), (unicode(r - l), unicode(b - t)), class_ = 'client'))
                    
                    # Create labels for the windows
                    if win.strName: 
                        svgGroup.add(svg.text(str(win.strName).replace('\r\n', '\n'), insert = (winSize[0] + 2, winSize[1] + 16), class_ = 'caption'))
                    
                    # If enabled `editbox` and is a editbox...
                    if self._config.EDITBOX:
                        ctrls = [ctrl for ctrl in editboxes if ctrl.hWnd == win.head.h]
                        if len(ctrls) > 0:
                            ctrlText = ctrls[0].get_text()
                            ctrlUndo = ctrls[0].get_undo()
                            svgEditbox = svgwrite.container.Group(class_ = 'editbox')
                            if ctrlText:
                                self.svg_multiline(svg, svgEditbox, str(ctrlText), cliSize[0] + 2, cliSize[1] + 12, 'text')
                            if ctrlUndo:
                                self.svg_multiline(svg, svgEditbox, str(ctrlUndo), cliSize[0] + 2, cliSize[1] + 12, 'undo')
                            svgGroup.add(svgEditbox)
                    
                    # If enabled `consoles` and process has a console...
                    if self._config.CONSOLE:
                        # `task` is the console host (conhost/csrss)
                        # FIXME: This filtering is hacky
                        # FIXME: str(String) will destroy Unicode characters but str(unicode(String)) will not
                        cons = [console for task, console in consoles if task.UniqueProcessId == win.head.pti.ppi.Process.UniqueProcessId and str(unicode(console.Title.dereference())) in (str(win.strName) if win.strName else '')]
                        if len(cons) > 0:
                            svgConsole = svgwrite.container.Group(class_ = 'console')
                            for screen in cons[0].get_screens():
                                screenText = [str(unicode(row.Chars.dereference())[0 : screen.ScreenX]).rstrip() for row in screen.Rows.dereference() if row.Chars.is_valid()]
                                lastIdx = 0
                                for idx, row in enumerate(reversed(screenText)):
                                    if len(row) > 0:
                                        lastIdx = idx
                                        break
                                screenText = screenText[0 : len(screenText) - lastIdx] if lastIdx > 0 else []
                                # FIXME: Unicode characters always repeat once
                                self.svg_multiline(svg, svgConsole, '\r\n'.join(screenText), cliSize[0] + 2, cliSize[1] + 12, 'text')
                                #self.svg_multiline(svg, svgConsole, '\r\n'.join(screen.get_buffer()), cliSize[0] + 2, cliSize[1] + 12, 'text')
                            for hist in cons[0].get_histories():
                                histText = [str(cmd.Cmd) for _, cmd in hist.get_commands() if cmd.Cmd]
                                self.svg_multiline(svg, svgConsole, '\r\n'.join(histText), cliSize[0] + 2, cliSize[1] + 12, 'history')
                            svgGroup.add(svgConsole)
                    
                    levelGroups[-1].add(svgGroup)
                    lastlevel = level
                
                # Collapse all children, then add to svg
                for _ in range(len(levelGroups), 1, -1):
                    svgGroup = levelGroups.pop()
                    levelGroups[-1].elements[-1].add(svgGroup)
                
                svg.add(levelGroups[0])
                
                file_name = "session_{0}.{1}.{2}.svg".format(
                    desktop.dwSessionId,
                    window_station.Name, desktop.Name)

                file_name = os.path.join(self._config.DUMP_DIR,
                    file_name)

                try:
                    svg.saveas(file_name, pretty = True, indent = 4)
                    result = "Wrote {0}".format(file_name)
                except SystemError, why:
                    result = why
                #except Exception, e:
                #    import traceback
                #    traceback.print_exc()
                #    import code
                #    code.interact(local = locals(), banner = '')

                outfd.write("{0}\n".format(result))

# ----------

import sys

import volatility.conf
import volatility.registry
import volatility.commands
import volatility.addrspace

def main():
    # Setup Volatility Framework
    debug.setup()
    volatility.registry.PluginImporter()
    
    # Initialize config object (cmdline parsing, etc)
    volconf = volatility.conf.ConfObject()
    volatility.registry.register_global_options(volconf, volatility.commands.Command)
    volatility.registry.register_global_options(volconf, volatility.addrspace.BaseAddressSpace)
    
    # Add SVGShot parameters, and seal options
    cmdObj = SVGShot(volconf)
    volconf.parse_options()
    
    # Execute command
    cmdObj.render_text(sys.stdout, cmdObj.calculate())
    
    return 0

# Standard main() invoking routine
if __name__ == '__main__':
    try:
        exit(main())
    except KeyboardInterrupt:
        exit(130)

