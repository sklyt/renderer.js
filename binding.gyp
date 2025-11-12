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
        "src/shared_buffer.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/include"
      ],
      "conditions": [
        ["OS=='linux'", {
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
          "libraries": [
            "-lraylib",
            "-framework", "OpenGL",
            "-framework", "Cocoa",
            "-framework", "IOKit",
            "-framework", "CoreVideo"
          ],
          "xcode_settings": {
            "OTHER_CPLUSPLUSFLAGS": ["-std=c++17", "-frtti", "-fexceptions"]
          }
        }],
        ["OS=='win'", {
                      "include_dirs": [
            "C:/vcpkg/installed/x64-windows/include"  # Changed to x64-windows
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
        }]
      ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
    }
  ]
}