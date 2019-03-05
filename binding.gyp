{
    "targets": [
        {
            "target_name": "tray",
            "sources": [
                "src/data.cc",
                "src/icon-object.cc",
                "src/menu-object.cc",
                "src/napi-win32.cc",
                "src/parse_guid.cc",
                "src/tray.cc"
            ],
            "defines": [
                "_WIN32_WINNT=_WIN32_WINNT_WIN7",
                "_UNICODE",
                "UNICODE"
            ],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "AdditionalOptions": [
                        "/std:c++17"
                    ],
                    "WarningLevel": 4,
                    "DisableSpecificWarnings": [
                        4100
                    ]
                }
            }
        }
    ]
}
