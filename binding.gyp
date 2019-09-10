{
    "targets": [
        {
            "target_name": "notify_icon",
            "include_dirs": [
                "src"
            ],
            "sources": [
                "src/napi/core.cc",
                "src/napi/props.cc",
                "src/napi/win32.cc",
                "src/data.cc",
                "src/icon-object.cc",
                "src/menu-object.cc",
                "src/notify-icon.cc",
                "src/notify-icon-message-loop.cc",
                "src/notify-icon-object.cc",
                "src/reg-icon-stream.cc",
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
                },
                # https://github.com/nodejs/node/issues/29501
                "VCLinkerTool": {
                    "OptimizeReferences": 2,             # /OPT:REF
                    "EnableCOMDATFolding": 2,            # /OPT:ICF
                    "LinkIncremental": 1                # disable incremental linking
                }
            }
        }
    ]
}
