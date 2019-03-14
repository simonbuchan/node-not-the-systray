{
    "targets": [
        {
            "target_name": "tray",
            "include_dirs": [
                "src"
            ],
            "sources": [
                "src/napi/props.cc",
                "src/napi/win32.cc",
                "src/data.cc",
                "src/icon-object.cc",
                "src/menu-object.cc",
                "src/parse_guid.cc",
                "src/module.cc"
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
