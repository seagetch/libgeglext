project('geglext',
  'c', 'cpp',
  license: 'GPL3+',
  version: '0.0.1',
  meson_version: '>=0.45.0',
  default_options: [
    'c_std=gnu11',
    'cpp_std=gnu++14',
    'buildtype=debugoptimized',
  ],
)

# Making releases on the stable branch:
#   micro_version += 1;
#   interface_age += 1;
#   binary_age += 1;
# if any functions have been added,
#    set interface_age to 0.
# if backwards compatibility has been broken,
#    set binary_age _and_ interface_age to 0.

pkgconfig = import('pkgconfig')
i18n      = import('i18n')
gnome     = import('gnome')
#python    = import('python').find_installation()

cc        = meson.get_compiler('c')
cpp       = meson.get_compiler('cpp')
buildtype = get_option('buildtype')
config    = configuration_data()

################################################################################
# Project infos

version = meson.project_version()
array_version = version.split('.')
major_version = array_version[0].to_int()
minor_version = array_version[1].to_int()
micro_version = array_version[2].to_int()

api_version = '@0@.@1@'.format(major_version, minor_version)
api_name    = meson.project_name() + '-' + api_version
gettext_package = api_name

stability_version_number = (major_version != 0 ? minor_version : micro_version)
stable = (stability_version_number % 2 == 0)

config.set        ('GEGLEXT_MAJOR_VERSION',  '@0@'.format(major_version))
config.set        ('GEGLEXT_MINOR_VERSION',  '@0@'.format(minor_version))
config.set        ('GEGLEXT_MICRO_VERSION',  '@0@'.format(micro_version))
config.set        ('GEGLEXT_UNSTABLE',       not stable)

config.set_quoted ('GEGLEXT_LIBRARY',        '@0@'.format(api_name))
config.set_quoted ('GETTEXT_PACKAGE',     '@0@'.format(gettext_package))

# Libtool versionning
interface_age = 1

binary_age = 100 * minor_version + micro_version
lt_current = binary_age - interface_age
so_version  = '@0@.@1@.@2@'.format(0, lt_current, interface_age)

################################################################################
# Host system detection

host_os = host_machine.system()
os_win32  = host_os.contains('mingw') or host_os.contains('windows')
os_android= host_os.contains('android')
os_osx    = host_os.contains('darwin')

if os_osx and cc.get_id() != 'clang'
  error('You should use Clang on OSx.')
endif


host_cpu_family = host_machine.cpu_family()
message('Host machine cpu family: ' + host_cpu_family)

if   host_cpu_family == 'x86'
  have_x86 = true
  config.set10('ARCH_X86',    true)
elif host_cpu_family == 'x86_64'
  have_x86 = true
  config.set10('ARCH_X86',    true)
  config.set10('ARCH_X86_64', true)
elif host_cpu_family == 'ppc'
  have_ppc = true
  config.set10('ARCH_PPC',    true)
elif host_cpu_family == 'ppc64'
  have_ppc = true
  config.set10('ARCH_PPC',    true)
  config.set10('ARCH_PPC64',  true)
endif

################################################################################
# Compiler arguments

cflags_common = []
cflags_c      = []
cflags_cpp    = []

cflags_common += [
  '-DHAVE_CONFIG_H',
]

if buildtype == 'debugoptimized' or buildtype == 'release'
  cflags_common += ['-Ofast']
endif

cflags_common += [
  '-Winit-self',
  '-Wmissing-declarations',
  '-Wpointer-arith',
  '-Wno-deprecated-declarations',
]
cflags_c = [
  '-Wmissing-prototypes',
  '-Wold-style-definition',
]

cflags_c   = cflags_common + cflags_c
cflags_cpp = cflags_common + cflags_cpp

add_project_arguments(cc.get_supported_arguments(cflags_c), language: 'c')
add_project_arguments(cpp.get_supported_arguments(cflags_cpp), language: 'cpp')

################################################################################
# Utilities

bash        = find_program('bash')

perl        = find_program('perl5', 'perl', required: false)
asciidoc    = find_program('asciidoc',      required: false)
enscript    = find_program('enscript',      required: false)
rsvg_convert= find_program('rsvg-convert',  required: false)
ruby        = find_program('ruby',          required: false)

################################################################################
# Required Dependencies

config.set('HAVE_UNISTD_H',    cc.has_header('unistd.h'))
config.set('HAVE_EXECINFO_H',  cc.has_header('execinfo.h'))
config.set('HAVE_FSYNC',       cc.has_function('fsync'))
config.set('HAVE_MALLOC_TRIM', cc.has_function('malloc_trim'))

math    = cc.find_library('m', required: false)
libdl   = cc.find_library('dl', required : false)
thread  = dependency('threads')

babl      = dependency('babl',        version: '>=0.1.44')
gegl      = dependency('gegl',        version: '>=0.3')
glib      = dependency('glib-2.0',    version: '>=2.44.0')
gobject   = dependency('gobject-2.0', version: '>=2.44.0')
gmodule   = dependency('gmodule-2.0', version: '>=2.44.0')
gthread   = dependency('gthread-2.0', version: '>=2.44.0')
#gio_os    = os_win32 ? 'gio-windows-2.0' : 'gio-unix-2.0'
#gio       = [
#            dependency('gio-2.0',     version: '>=2.44.0'),
#            dependency(gio_os,        version: '>=2.44.0'),
#]
#json_glib = dependency('json-glib-1.0', version: '>=1.2.6')

# Required libraries eventually provided in subprojects/ subdir

#poly2tri_c= dependency('poly2tri-c',  version: '>=0.0.0',
#  fallback: [ 'poly2tri-c', 'poly2tri_c' ],
#  required: false,
#)

#libnsgif = dependency('libnsgif',
#  fallback: [ 'libnsgif', 'libnsgif' ],
#)

################################################################################
# Optionnal Dependencies

################################################################################
# Subdirs

configure_file(
  output: 'config.h',
  configuration: config
)

rootInclude = include_directories('.')

#argvs_extract = find_program('tools/argvs_extract.sh')

subdir('src')
#subdir('po')
#if get_option('docs')
#  subdir('docs')
#endif


pkgconfig.generate(filebase: 'geglext-' + api_version,
  name: 'GEGLEXT',
  description: 'Extension to Generic Graphics Library',
  version: meson.project_version(),
  variables: 'pluginsdir=' + '${prefix}' +'/'+ get_option('libdir') +'/'+ api_name,
#  requires: [
#    gobject,
#    gmodule,
#    babl,
#    gegl
#  ],
  libraries: [
    geglext_lib
  ],
  subdirs: 'src',
)



message('\n'.join(['',
'Building GEGLEXT with prefix=@0@'.format(get_option('prefix')),
'',
'']))
