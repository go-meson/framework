{
    'includes': [
        'toolchain.gypi',
        'vendor/brightray/brightray.gypi',
    ],
    'variables': {
        'component%': 'static_library',
        'V8_BASE': '',
        'v8_postmortem_support': 'false',
        'v8_enable_i18n_support': 'false',
        'v8_inspector': 'false',
    },
    # Settings to compile node under Windows.
    'target_defaults': {
        'target_conditions': [
        ],
        'msvs_cygwin_shell': 0, # Strangely setting it to 1 would make building under cygwin fail.
        'msvs_disabled_warnings': [
            4005,  # (node.h) macro redefinition
            4091,  # (node_extern.h) '__declspec(dllimport)' : ignored on left of 'node::Environment' when no variable is declared
            4189,  # local variable is initialized but not referenced
            4201,  # (uv.h) nameless struct/union
            4267,  # conversion from 'size_t' to 'int', possible loss of data
            4302,  # (atldlgs.h) 'type cast': truncation from 'LPCTSTR' to 'WORD'
            4458,  # (atldlgs.h) declaration of 'dwCommonButtons' hides class member
            4503,  # decorated name length exceeded, name was truncated
            4800,  # (v8.h) forcing value to bool 'true' or 'false'
            4819,  # The file contains a character that cannot be represented in the current code page
            4838,  # (atlgdi.h) conversion from 'int' to 'UINT' requires a narrowing conversion
            4996,  # (atlapp.h) 'GetVersionExW': was declared deprecated
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'WarnAsError': 'false',
          },
        },
        'xcode_settings': {
          'GCC_TREAT_WARNINGS_AS_ERRORS': 'NO',
          'WARNING_CFLAGS': [
            '-Wno-unknown-warning-option',
            '-Wno-parentheses-equality',
            '-Wno-unused-function',
            '-Wno-sometimes-uninitialized',
            '-Wno-pointer-sign',
            '-Wno-sign-compare',
            '-Wno-string-plus-int',
            '-Wno-unused-variable',
            '-Wno-deprecated-declarations',
            '-Wno-return-type',
            '-Wno-gnu-folding-constant',
            '-Wno-shift-negative-value',
            '-Wno-varargs', # https://git.io/v6Olj
          ],
        }
    },
    'conditions': [
        # The breakdpad on Windows assumes Debug_x64 and Release_x64 configurations.
        # The breakdpad on Mac assumes Release_Base configuration.
        ['OS=="mac"', {
            'target_defaults': {
                'configurations': {
                    'Release_Base': {
                    },
                },
            },
        }],  # OS=="mac"
    ],
}
