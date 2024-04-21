project('quickerStella','c','cpp',
  version: '1.0.0',
  license: 'GPL-3.0-only',
  default_options : ['cpp_std=c++20', 'default_library=shared', 'buildtype=release'],
  subproject_dir : 'extern'
)

# Loading dependencies
subdir('source')

# Do not build any targets if this is a subproject
if meson.is_subproject() == false

# Common application flags
commonCompileArgs = [ '-Wall', '-Wfatal-errors' ]

# Grabbing jaffarCommon dependency

jaffarCommonSubproject = subproject('jaffarCommon')
jaffarCommonDependency = jaffarCommonSubproject.get_variable('jaffarCommonDependency')

# Building playback tool

if get_option('buildPlayer') == true

  newStellaPlayer = executable('newStellaPlayer',
    'source/player.cpp',
    cpp_args            : [ commonCompileArgs, '-DNCURSES' ],
    dependencies        : [ newStellaDependency,
                            jaffarCommonDependency
                          ],
    include_directories : include_directories(['source']),
    link_args           : [ '-lncurses' ],
  )

  baseStellaPlayer = executable('baseStellaPlayer',
    'source/player.cpp',
    cpp_args            : [ commonCompileArgs, '-DNCURSES' ],
    dependencies        : [ baseStellaDependency,
                            jaffarCommonDependency,
                          ],
    include_directories : include_directories(['source']),
    link_args           : [ '-lncurses' ],
  )

endif

# Building tester tool for QuickerSnes9x

newStellaTester = executable('newStellaTester',
  'source/tester.cpp',
  cpp_args            : [ commonCompileArgs ], 
  dependencies        : [ newStellaDependency, jaffarCommonDependency ],
)

# Building tester tool for the original snes9x

baseStellaTester = executable('baseStellaTester',
  'source/tester.cpp',
  cpp_args            : [ commonCompileArgs ],
  dependencies        : [ baseStellaDependency, jaffarCommonDependency ],
)

# Building tests
subdir('tests')

endif # If not subproject
