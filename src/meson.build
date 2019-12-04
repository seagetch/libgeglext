geglext_cflags = [
  '-DLIBDIR="@0@"'.format(get_option('prefix') +'/'+ get_option('libdir')),
  '-DG_LOG_DOMAIN="@0@"'.format('GEGLEXT'),
  '-DGEGL_LOCALEDIR="@0@"'.format(get_option('prefix') +'/'+ get_option('localedir'))
]

geglext_version_h = configure_file(
  input : 'geglext-version.h.in',
  output: 'geglext-version.h',
  configuration: config,
)

geglext_introspectable_headers = files(
  'geglboundary.h',
  'gegloperationbucketfill.h'
) + [
  geglext_version_h,
]

geglext_sources = files(
  'geglboundary.c',
  'gegloperationbucketfill.cc'
)

geglext_headers = geglext_introspectable_headers

#subdir('property-types')

#gegl_headers += gegl_introspectable_headers

install_headers(geglext_headers,
  subdir: api_name
)

geglext_ldflags = os_osx ? ['-framework', 'OpenCL'] : []

geglext_lib = library(api_name,
  geglext_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    babl,
    gegl,
    glib,
    math,
    gmodule,
  ],
  c_args: [ geglext_cflags, ],
  link_args: [ geglext_ldflags, ],
  install: true,
  version: so_version,
)


#introspection_sources = geglext_introspectable_headers + files(
#  'geglext-introspection-support.h',
#)


#if get_option('introspection')
#
#  geglext_gir = gnome.generate_gir(gegl_lib,
#    sources: introspection_sources,
#    nsversion: api_version,
#    namespace: 'GeglExt',
#    identifier_prefix: 'GeglExt',
#    symbol_prefix: 'geglext',
#    header: 'geglext.h',
#    includes: [ 'GLib-2.0', 'GObject-2.0', 'Babl-0.1', 'Babl-0.3' ],
#    install: true,
#  )

#  if vapigen.found()
#     gnome.generate_vapi(api_name,
#       metadata_dirs: '.',
#       sources: geglext_gir[0],
#       install: true,
#     )
#  endif
#endif
