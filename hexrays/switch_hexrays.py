# --- IDAPython plugin ---
import sys
import idaapi
import ida_diskio
import ida_idaapi
import ida_hexrays

from ctypes import *

# Hold a reference of IDA kernel library and IDA calling convention
lib_ida_name = "ida64" if ida_idaapi.__EA64__ else "ida"
if sys.platform == "win32":
    lib_ida = windll["%s.dll" % lib_ida_name]
    IDAFUNCTYPE = WINFUNCTYPE
elif sys.platform == "linux2":
    lib_ida = cdll["lib%s.so" % lib_ida_name]
    IDAFUNCTYPE = CFUNCTYPE
elif sys.platform == "darwin":
    lib_ida = cdll["lib%s.dylib" % lib_ida_name]
    IDAFUNCTYPE = CFUNCTYPE

# Plugin related APIs are not exported by IDAPython
# NOTE: Only used APIs are included
class ida_plugins:

    # The structure of plugin description block
    class plugin_t(Structure):
        _fields_ = [
            ('version', c_int),
            ('flags', c_int),
            ('init', IDAFUNCTYPE(c_int)),
            ('term', IDAFUNCTYPE(None)),
            ('run', IDAFUNCTYPE(c_bool, c_size_t)),
            ('comment', c_char_p),
            ('help', c_char_p),
            ('wanted_name', c_char_p),
            ('wanted_hotkey', c_char_p)
        ]

    # Specify the function prototypes
    lib_ida.find_plugin.argtypes = [c_char_p, c_bool]
    lib_ida.find_plugin.restype = POINTER(plugin_t)
    lib_ida.run_plugin.argtypes = [POINTER(plugin_t), c_size_t]
    lib_ida.run_plugin.restype = c_bool

    @staticmethod
    def find_plugin(name, load_if_needed = False):
        '''
        Find a user-defined plugin and optionally load it.

        Parameters
            name - short plugin name without path and extension,
                or absolute path to the file name
            load_if_needed - if the plugin is not present in the memory,
                try to load it

        Returns
            pointer to plugin description block
        '''
        return lib_ida.find_plugin(name, load_if_needed)

    @staticmethod
    def run_plugin(ptr, arg):
        '''
        Run a loaded plugin with the specified argument.

        Parameters
            ptr - pointer to plugin description block
            arg - argument to run with
        '''
        return lib_ida.run_plugin(ptr, arg)

    @staticmethod
    def get_plugin_full_path(name):
        return ida_diskio.getsysfile(name, ida_diskio.PLG_SUBDIR)

class hx_switch_plugin_t(idaapi.plugin_t):

    # Hook class for Hex-Rays
    class hx_hook_t(ida_hexrays.Hexrays_Hooks):
    
        # `Hexrays_Hooks` members
        def __init__(self):
            # Call parent's constructor first
            # Otherwise there will be tons of `TypeError`s about `self`
            super(self.__class__, self).__init__()
            self.hx_views = {}
    
        def open_pseudocode(self, vu):
            # `vdui_t::view_idx` is the only unique and hashable thing
            # `vdui_t::ct` represents the view containing pseudocode, as per SDK
            self.hx_views[vu.view_idx] = vu.ct
            return 0
    
        def close_pseudocode(self, vu):
            del self.hx_views[vu.view_idx]
            return 0
    
        def close_hx_views(self):
            # NOTE: `self.close_pseudocode()` got called in this loop
            #       `dict.values()` returns a temp list
            for ct in self.hx_views.values():
                idaapi.close_tform(ct, 0)

    # `plugin_t` members
    flags = 0
    comment = 'Switch current Hex-Rays Decompiler between alternatives'
    help = comment
    wanted_name = 'Switch Hex-Rays Decompiler'
    wanted_hotkey = 'Alt-F5'

    plugin_t_run_dummy = WINFUNCTYPE(c_bool, c_size_t)(lambda arg: True)
    hx_alternatives = [
        #'hexx64', # This only matches the current loaded decompiler
        ida_plugins.get_plugin_full_path('hexx64.dll'),
        ida_plugins.get_plugin_full_path('hexx64_7.5_7.x.bin')
    ]

    def init(self):
        self.current_idx = -1
        self.hx_hook = None
        
        # Figure out the current decompiler
        for idx, plugin_id in enumerate(hx_switch_plugin_t.hx_alternatives):
            ptr = ida_plugins.find_plugin(plugin_id)
            if ptr:
                self.current_idx = idx
                print '[+] Current decompiler: [%d]: %s' % (idx, repr(plugin_id))
                break
        else:
            print '[!] Default decompiler not found'
            return idaapi.PLUGIN_SKIP
        
        self.hx_hook = hx_switch_plugin_t.hx_hook_t()
        if not self.hx_hook.hook():
            print '[!] Decompiler hook failed'
            return idaapi.PLUGIN_SKIP
        
        # Now that we ensured that initialization was a success
        # Time to put in more branding info
        addon_info = idaapi.addon_info_t()
        addon_info.id = 'net.ezforever.hexrays.hx_switch'
        addon_info.name = hx_switch_plugin_t.wanted_name
        addon_info.producer = 'Eric Zhang (EZForever)'
        addon_info.version = '0.0.0'
        addon_info.url = 'https://github.com/EZForever'
        addon_info.freeform = 'This plugin is licensed under The Unlicense.'
        idaapi.register_addon(addon_info)
        
        # Keep us in memory since we need to hold the current state
        return idaapi.PLUGIN_KEEP

    def run(self, arg):
        new_idx = (self.current_idx + 1) % len(hx_switch_plugin_t.hx_alternatives)
        
        plugin_id = hx_switch_plugin_t.hx_alternatives[self.current_idx]
        new_plugin_id = hx_switch_plugin_t.hx_alternatives[new_idx]
        
        print '[+] Switching to [%d]: %s' % (new_idx, repr(new_plugin_id))
        
        ptr = ida_plugins.find_plugin(plugin_id)
        if not ptr:
            print '[!] Unable to locate the plugin description block'
            return False
        
        # Close all pseudocode views, otherwise IDA will crash on unload
        self.hx_hook.close_hx_views()
        
        # Temporarily unhook, avoid messing with `hexdsp`
        self.hx_hook.unhook()
        
        # Set unload flag
        ptr.contents.flags |= idaapi.PLUGIN_UNL
        
        # Replace the "run plugin" function with a dummy, so no more nags
        ptr.contents.run = hx_switch_plugin_t.plugin_t_run_dummy
        
        # Call the plugin and cue IDA to unload it
        ida_plugins.run_plugin(ptr, 0)
        
        # Load the new plugin
        ida_plugins.find_plugin(new_plugin_id, True)
        
        # Sometimes `ida_hexrays` forget to re-initialize, causing crash on
        # switching decompilers and closing database
        ida_hexrays.init_hexrays_plugin()
        
        # Hook again to record views
        self.hx_hook.hook()
        
        self.current_idx = new_idx
        return True

    def term(self):
        if self.hx_hook:
            self.hx_hook.unhook()

def PLUGIN_ENTRY():
    return hx_switch_plugin_t()

