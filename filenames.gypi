{
  'variables': {
    'lib_sources': [
        'src/app/common/chrome_version.h',
        'src/app/content_client.h',
        'src/app/content_client.cc',
        'src/app/main.h',
        'src/app/main.cc',
        'src/app/main_delegate.h',
        'src/app/main_delegate.cc',
        'src/app/main_delegate_mac.mm',
        'src/browser/browser_client.h',
        'src/browser/browser_client.cc',
        'src/renderer/renderer_client.h',
        'src/renderer/renderer_client.cc'
    ],
    'lib_sources_nss': [
      'chromium_src/chrome/browser/certificate_manager_model.cc',
      'chromium_src/chrome/browser/certificate_manager_model.h',
    ],
    'framework_sources': [
      'src/api/meson.h',
      'src/api/meson.cc'
    ],
    'locales': [
      'am', 'ar', 'bg', 'bn', 'ca', 'cs', 'da', 'de', 'el', 'en-GB',
      'en-US', 'es-419', 'es', 'et', 'fa', 'fi', 'fil', 'fr', 'gu', 'he',
      'hi', 'hr', 'hu', 'id', 'it', 'ja', 'kn', 'ko', 'lt', 'lv',
      'ml', 'mr', 'ms', 'nb', 'nl', 'pl', 'pt-BR', 'pt-PT', 'ro', 'ru',
      'sk', 'sl', 'sr', 'sv', 'sw', 'ta', 'te', 'th', 'tr', 'uk',
      'vi', 'zh-CN', 'zh-TW',
    ],
  },
}
