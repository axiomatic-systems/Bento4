import os
from SCons.Script import Split

def generate(env, gcc_cross_prefix=None, gcc_strict=True, gcc_stop_on_warning=True,
             gcc_extra_options=[]):
    if env.has_key('stop_on_warning'): gcc_stop_on_warning = env['stop_on_warning']

    ### compiler flags
    gcc_extra_options = Split(gcc_extra_options) # normalize
    if gcc_strict:
        env.AppendUnique(CCFLAGS = ['-pedantic', '-Wall',  '-W',  '-Wundef', '-Wno-long-long'])
        env.AppendUnique(CFLAGS  = ['-Wmissing-prototypes', '-Wmissing-declarations'])
    else:
        env.AppendUnique(CCFLAGS = ['-Wall'])
    
    compiler_defines = ['-D_REENTRANT']
    env.AppendUnique(CCFLAGS  = compiler_defines)
    env.AppendUnique(CPPFLAGS = compiler_defines)
    
    if env['build_config'] == 'Debug':
        env.AppendUnique(CCFLAGS = '-g')
    else:
        env.AppendUnique(CCFLAGS = '-O3')
    
    if gcc_stop_on_warning:
        env.AppendUnique(CCFLAGS = ['-Werror'])
        
    if gcc_cross_prefix:
        env['ENV']['PATH'] += os.environ['PATH']
        env['AR']     = gcc_cross_prefix+'-ar'
        env['RANLIB'] = gcc_cross_prefix+'-ranlib'
        env['CC']     = gcc_cross_prefix+'-gcc'
        env['CXX']    = gcc_cross_prefix+'-g++'
        env['LINK']   = gcc_cross_prefix+'-g++'

    env.Append(CCFLAGS = gcc_extra_options)
