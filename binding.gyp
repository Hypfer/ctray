{
    "targets": [
        {
            "target_name": "tray",
            "cflags": ["-g -Wall -Wextra -pedantic"], # tray/Makefile:13 
            "ldflags": ["-g"],
            "cflags!": [ "-fno-exceptions" ],
            "cflags_cc!": [ "-fno-exceptions" ],
            "cflags_cc" : ["-std=c++17"],
            "sources": [],
            "include_dirs": [
                "src",
                "<!@(node -p \"require('node-addon-api').include\")"
            ],
            "dependencies": [],
            'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
            'conditions': [ # Initials checks on tray/Makefile
                ["OS=='linux'", {
                    "sources": ["src/linux/tray.cc"],
                    "cflags+": ["-DTRAY_APPINDICATOR=1", "<!@(pkg-config --cflags ayatana-appindicator3-0.1)"],
                    "ldflags+": ["<!@(pkg-config --libs ayatana-appindicator3-0.1)"],
                }],
                ["OS=='win'", {
                    "sources": ["src/win/tray.cc"],
                    "defines":["TRAY_WINAPI=1"]
                }],
                ["OS=='mac'", {
                    "sources": ["src/mac/tray.cc"],
                    "cflags+": ["-DTRAY_APPKIT=1"],
                    "dflags+": ["-framework Cocoa"]
                }]
            ]
        }
    ]
}
