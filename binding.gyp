{
  'includes': [ 'common.gypi' ],
  'targets': [
    {
      'target_name': '<(module_name)',
      'product_dir': '<(module_path)',
      'sources': [ './src/node_map_to_zoom.cpp', './src/node.cpp' ],
      'include_dirs': [
        '<!(node -e \'require("nan")\')',
        'include',
        'mason_packages/.link/include',
        'deps/geojson-cpp/include', 
        'deps/wagyu/include', 
        'deps/vector-tile/include'
      ],
      'ldflags': [
        '-Wl,-z,now',
      ],
      'xcode_settings': {
        'OTHER_LDFLAGS':[
          '-Wl,-bind_at_load'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++14',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      }
    }
  ]
}
