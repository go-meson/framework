{
    'variables': {
        'project_name%': 'meson',
        'product_name%': 'Meson',
        'company_name%': 'GitHub, Inc',
        'company_abbr%': 'github',
        'version%': '0.0.1',
        'js2c_input_dir': '<(SHARED_INTERMEDIATE_DIR)/js2c',
    },
    'includes': [
        'filenames.gypi',
        # Hummm... add native-mate?
    ],
    'target_defaults': {
        'defines': [
            'MESON_PRODUCT_NAME="<(product_name)"',
            'MESON_PROJECT_NAME="<(project_name)"',
        ],
        'conditions': [
            ['OS=="mac"', {
                'mac_framework_dirs': [
                    '<(source_root)/external_binaries',
                ],
            }],
        ],
    },
    'targets': [
        {
            'target_name': '<(project_name)_framework',
            'product_name': '<(product_name) Framework',
            'type': 'shared_library',
            'dependencies': [
                '<(project_name)_lib',
            ],
            'sources': [
                '<@(framework_sources)',
            ],
            'include_dirs': [
                './src',
                'vendor',
                '<(libchromiumcontent_src_dir)',
            ],
            'export_dependent_settings': [
                '<(project_name)_lib',
            ],
            'link_settings': {
                'libraries': [
                '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
                '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
                ],
            },
            'mac_bundle': 1,
            'mac_bundle_resources': [
                'src/app/common/resources/mac/MainMenu.xib',
                '<(libchromiumcontent_dir)/content_shell.pak',
                '<(libchromiumcontent_dir)/icudtl.dat',
                '<(libchromiumcontent_dir)/natives_blob.bin',
                '<(libchromiumcontent_dir)/snapshot_blob.bin',
            ],
            'xcode_settings': {
                'MESON_BUNDLE_ID': 'com.<(company_abbr).<(project_name).framework',
                'INFOPLIST_FILE': 'src/app/common/resources/mac/Info.plist',
                'LD_DYLIB_INSTALL_NAME': '@rpath/<(product_name) Framework.framework/<(product_name) Framework',
                'LD_RUNPATH_SEARCH_PATHS': [
                    '@loader_path/Libraries',
                ],
                'OTHER_LDFLAGS': [
                    '-ObjC',
                ],
              },
            'copies': [
                {
                'variables': {
                    'conditions': [
                    ['libchromiumcontent_component', {
                        'copied_libraries': [
                        '<@(libchromiumcontent_shared_libraries)',
                        '<@(libchromiumcontent_shared_v8_libraries)',
                        ],
                    }, {
                        'copied_libraries': [
                        '<(libchromiumcontent_dir)/libffmpeg.dylib',
                        ],
                    }],
                    ],
                },
                'destination': '<(PRODUCT_DIR)/<(product_name) Framework.framework/Versions/A/Libraries',
                'files': [
                    '<@(copied_libraries)',
                ],
                },
            ],
            'postbuilds': [
                {
                'postbuild_name': 'Add symlinks for framework subdirectories',
                'action': [
                    'tools/mac/create-framework-subdir-symlinks.sh',
                    '<(product_name) Framework',
                    'Libraries',
                ],
                },
                {
                'postbuild_name': 'Copy locales',
                'action': [
                    'tools/mac/copy-locales.py',
                    '-d',
                    '<(libchromiumcontent_dir)/locales',
                    '${BUILT_PRODUCTS_DIR}/<(product_name) Framework.framework/Resources',
                    '<@(locales)',
                ],
                },
            ]
        },
        {
            'target_name': '<(project_name)_lib',
            'type': 'static_library',
            'dependencies': [
                'vendor/brightray/brightray.gyp:brightray',
            ],
            'defines': [
                # This is defined in skia/skia_common.gypi.
                'SK_SUPPORT_LEGACY_GETTOPDEVICE',
                # Disable warnings for g_settings_list_schemas.
                'GLIB_DISABLE_DEPRECATION_WARNINGS',
                # Defined in Chromium but not exposed in its gyp file.
                'V8_USE_EXTERNAL_STARTUP_DATA',
                'ENABLE_PLUGINS',
                'ENABLE_PEPPER_CDMS',
                'USE_PROPRIETARY_CODECS',
            ],
            'sources': [
                '<@(lib_sources)',
            ],
            'include_dirs': [
                './src',
                'vendor',
                'vendor/brightray',
                # Include atom_natives.h.
                '<(SHARED_INTERMEDIATE_DIR)',
                '<(libchromiumcontent_src_dir)/v8/include',
                # The `third_party/WebKit/Source/platform/weborigin/SchemeRegistry.h` is using `platform/PlatformExport.h`.
                '<(libchromiumcontent_src_dir)/third_party/WebKit/Source',
                # The 'third_party/libyuv/include/libyuv/scale_argb.h' is using 'libyuv/basic_types.h'.
                '<(libchromiumcontent_src_dir)/third_party/libyuv/include',
                # The 'third_party/webrtc/modules/desktop_capture/desktop_frame.h' is using 'webrtc/base/scoped_ptr.h'.
                '<(libchromiumcontent_src_dir)/third_party/',
                '<(libchromiumcontent_src_dir)/components/cdm',
                '<(libchromiumcontent_src_dir)/third_party/widevine',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    '.',
                ],
            },
            'export_dependent_settings': [
                'vendor/brightray/brightray.gyp:brightray',
            ],
            'conditions': [
                ['libchromiumcontent_component', {
                    'link_settings': {
                        'libraries': [ '<@(libchromiumcontent_v8_libraries)' ],
                    },
                }],
                ['OS=="mac" and mas_build==0', {
                    'dependencies': [
                    ],
                    'link_settings': {
                        # Do not link with QTKit for mas build.
                        'libraries': [
                            '$(SDKROOT)/System/Library/Frameworks/QTKit.framework',
                        ],
                    },
                    'xcode_settings': {
                        # ReactiveCocoa which is used by Squirrel requires using __weak.
                        'CLANG_ENABLE_OBJC_WEAK': 'YES',
                    },
                }],  # OS=="mac" and mas_build==0
            ],
        },
    ]
}
