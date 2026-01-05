{
  "targets": [
    {
      "target_name": "renderer",
      "sources": [ 
        "src/addon.cpp", 
        "src/renderer.cpp", 
        "src/renderer_wrapper.cpp", 
        "src/input_manager.cpp", 
        "src/debug/debugger_wrapper.cpp",
        "src/debug/debugger.cpp",
        "src/input_wrapper.cpp",
        "src/shared_buffer.cpp",
        "src/audio_manager.cpp",
        "src/audio_wrapper.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/include"
      ],
      "conditions": [
        ["OS=='win'", {
          "sources": [ "src/console_win.cpp" ],
          "include_dirs": [
            "C:/vcpkg/installed/x64-windows/include"
          ],
          "libraries": [
            "C:/vcpkg_installed/x64-windows/lib/raylib.lib",
            "opengl32.lib",
            "gdi32.lib",
            "winmm.lib"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": { 
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++20"]
            }
          }
        }],
        ["OS=='linux'", {
          "sources": [ "src/console_posix.cpp" ],
          "libraries": [
            "-lraylib",
            "-lGL",
            "-lm",
            "-lpthread",
            "-ldl",
            "-lrt",
            "-lX11"
          ],
          "cflags!": ["-fno-exceptions"],
          "cflags_cc!": ["-fno-exceptions"]
        }],
        ["OS=='mac'", {
          "sources": [ "src/console_posix.cpp" ],
          "libraries": [ 
              "-lraylib",
              "-framework Foundation",
              "-framework Cocoa",
              "-framework IOKit",
              "-framework CoreVideo",
              "-framework OpenGL"
          ],
          "xcode_settings": {
            "OTHER_CPLUSPLUSFLAGS": ["-std=c++17", "-frtti", "-fexceptions"]
          },
          "link_settings": {
            "library_dirs": [ 
                "/System/Library/Frameworks"
            ]
          }
        }]
      ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
    }
  ]
}