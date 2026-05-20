// Dear ImGui: standalone example application for SDL3 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"
#include <stdio.h>
#include <fontconfig/fontconfig.h>
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

std::string GetLinuxSystemMonospaceFontPath() {
    std::string font_path = ""; // 실패 시 빈 문자열

    if (!FcInit()) return font_path;

    FcConfig* fc_config = FcConfigGetCurrent();
    FcPattern* pattern = FcPatternCreate();

    // [핵심] 특정 폰트 이름 대신 "monospace"를 주어 시스템 고정폭 대표 서체를 요청합니다.
    FcPatternAddString(pattern, FC_FAMILY, (const FcChar8*)"monospace");
    
    FcConfigSubstitute(fc_config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    
    FcResult result;
    // 시스템 매칭 엔진 가동
    FcPattern* match = FcFontMatch(fc_config, pattern, &result);
    
    if (match) {
        FcChar8* file_path = nullptr;
        // 매칭된 폰트의 실제 디스크 파일 경로(FC_FILE)를 긁어옵니다.
        if (FcPatternGetString(match, FC_FILE, 0, &file_path) == FcResultMatch) {
            font_path = (const char*)file_path;
        }
        FcPatternDestroy(match);
    }
    
    FcPatternDestroy(pattern);
    // FcFini(); // 프로그램 종료 시에만 호출하는 것이 안전하므로 여기선 생략 가능
    
    return font_path;
}

void ConfigureLinuxNativeFont(ImFontConfig& config) {
    config.PixelSnapH = true;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.FontLoaderFlags = 0;

    if (!FcInit()) {
        config.FontLoaderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
        return;
    }

    FcConfig* fc_config = FcConfigGetCurrent();
    FcPattern* pattern = FcPatternCreate();

    FcPatternAddString(pattern, FC_FAMILY, (const FcChar8*)"Noto Sans CJK KR");
    
    FcConfigSubstitute(fc_config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    
    FcResult result;
    FcPattern* match = FcFontMatch(fc_config, pattern, &result);
    
    if (match) {
        FcBool antialias = FcTrue;
        int hinting = FcTrue;
        int hint_style = FC_HINT_SLIGHT;

        FcPatternGetBool(match, FC_ANTIALIAS, 0, &antialias);
        FcPatternGetInteger(match, FC_HINTING, 0, &hinting);
        FcPatternGetInteger(match, FC_HINT_STYLE, 0, &hint_style);

        if (antialias == FcFalse) {
            config.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_MonoHinting | ImGuiFreeTypeBuilderFlags_Monochrome;
        } else {
// [수정] 웹 브라우저 느낌을 내기 위해 힌팅 강도를 한 단계씩 낮추어 매핑합니다.
            if (hinting == FcFalse || hint_style == FC_HINT_NONE) {
                config.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_NoHinting;
            } else if (hint_style == FC_HINT_SLIGHT || hint_style == FC_HINT_MEDIUM) {
                // Medium 힌팅까지는 브라우저처럼 부드러운 LightHinting으로 처리하는 것이 정신건강에 좋습니다.
                config.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_LightHinting;
            } else if (hint_style == FC_HINT_FULL) {
                config.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_MonoHinting;
            }
        }

        FcPatternDestroy(match);
    } else {
        config.FontLoaderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
    }
    
    FcPatternDestroy(pattern);
}

int main(int, char**)
{
    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    ImFontConfig config;
    
    config.PixelSnapH = true;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.FontLoaderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;

    float font_size = 23.0f; 

    ImFont* font = io.Fonts->AddFontFromFileTTF(
        GetLinuxSystemMonospaceFontPath().c_str(),
        font_size,
        &config, 
        io.Fonts->GetGlyphRangesKorean()
    );

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static std::vector<std::string> m_Lines = { "안녕하세요" };
    static int m_CursorLine = 0;
    static int m_CursorByteOffset = 15;

    static std::string m_CompositionText = "";

    SDL_StartTextInput(window);

    // Main loop
    bool done = false;

    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
            
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
            {
                SDL_Delay(10);
                continue;
            }
// 2. 글자 조립 중 (Composition)
    if (event.type == SDL_EVENT_TEXT_EDITING)
    {
        m_CompositionText = event.edit.text;
        continue; 
    }

    // 3. 글자 완성 및 일반 문자/스페이스 입력 (Commit)
    if (event.type == SDL_EVENT_TEXT_INPUT)
    {
        // 글자가 완성되었으므로 조립 버퍼는 비웁니다.
        m_CompositionText.clear();

        std::string input_str = event.text.text;

        std::cout << input_str << std::endl;

        // [안전 장치] 커서 오프셋이 현재 줄의 길이를 초과했다면 맨 뒤로 강제 고정
        if (m_CursorByteOffset < 0) m_CursorByteOffset = 0;
        if (m_CursorByteOffset > (int)m_Lines[m_CursorLine].size()) {
            m_CursorByteOffset = (int)m_Lines[m_CursorLine].size();
        }

        // 현재 정확한 커서 위치에 입력된 문자(한글이든 공백이든)를 삽입
        m_Lines[m_CursorLine].insert(m_CursorByteOffset, input_str);

        // [핵심] 삽입한 바이트 크기만큼 커서를 '즉시' 전진시킵니다.
        // 다음 루프에서 공백(" ") 처리가 들어올 때 이 전진된 커서 뒤에 붙게 됩니다.
        m_CursorByteOffset += input_str.size();
        
        continue;
    }


        }


        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | 
                                            ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoMove | 
                                            ImGuiWindowFlags_NoResize | 
                                            ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoBringToFrontOnFocus;

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!", nullptr, window_flags);



            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 start_pos = ImGui::GetCursorScreenPos();

            ImFont* current_font = ImGui::GetFont();
            float total_line_height = ImGui::GetTextLineHeight() + 6.0f; // 줄높이 + 행간 마진
            float char_width = current_font->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "A").x;



            for (size_t i = 0; i < m_Lines.size(); i++){
                float box_top_y = start_pos.y + (i * total_line_height);
                float text_draw_y = box_top_y + 3.0f; // 마진 절반 내려서 정중앙 정렬

                // 줄 번호 그리기 영역 (가로 폭 60px 가정)
                char num_buf[16]; sprintf(num_buf, "%3zu", i + 1);
                draw_list->AddText(ImVec2(start_pos.x + 10.0f, text_draw_y), ImColor(120, 120, 120), num_buf);

                // 본문이 시작되는 X 좌표
                float text_start_x = start_pos.x + 60.0f;

                if (i == (size_t)m_CursorLine)
                {
                    // [중요] 현재 커서가 있는 줄은 세 단계로 쪼개어 그립니다.
                    
                    // 1단계: 커서 앞쪽 텍스트 그리기
                    std::string left_text = m_Lines[i].substr(0, m_CursorByteOffset);
                    draw_list->AddText(ImVec2(text_start_x, text_draw_y), ImColor(30, 30, 30), left_text.c_str());

                    // 앞쪽 텍스트의 가로 길이를 계산합니다. (ImGui 함수 활용)
                    float left_width = current_font->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, left_text.c_str()).x;
                    float current_x = text_start_x + left_width;

                    float cursor_x = text_start_x + left_width; 
                    float cursor_y = box_top_y;

                    // SDL3에게 "내 에디터의 커서 위치는 여기야!"라고 알림
                    SDL_Rect area = { (int)cursor_x, (int)cursor_y, 1, (int)total_line_height };
                    SDL_SetTextInputArea(window, &area, 0); // 0은 커서 위치 오프셋입니다.

                    // 2단계: OS가 조합 중인 한글이 있다면 중간에 끼워 넣어서 그리기
                    if (!m_CompositionText.empty()) {
                        // 조합 중인 한글은 눈에 띄게 붉은 계열로 그립니다.
                        draw_list->AddText(ImVec2(current_x, text_draw_y), ImColor(100, 100, 100), m_CompositionText.c_str());
                        
                        float comp_width = current_font->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, m_CompositionText.c_str()).x;
                        
                        // 브라우저 느낌을 내기 위해 조합 중인 글자 아래에 빨간 밑줄을 그어줍니다.
                        draw_list->AddLine(
                            ImVec2(current_x, box_top_y + total_line_height - 2.0f),
                            ImVec2(current_x + comp_width, box_top_y + total_line_height - 2.0f),
                            ImColor(100, 100, 100), 1.5f
                        );

                        current_x += comp_width; // 가상 커서 위치를 조합 문자의 끝으로 이동
                    }
                    else
                    {
                        // 조합 중인 글자가 없을 때만 진짜 깜빡이는 바(Bar) 커서를 그립니다.
                        if (fmod(ImGui::GetTime(), 1.0f) < 0.5f) {
                            draw_list->AddRectFilled(
                                ImVec2(current_x, box_top_y + 2.0f),
                                ImVec2(current_x + 2.0f, box_top_y + total_line_height - 2.0f),
                                ImColor(0, 0, 0)
                            );
                        }
                    }

                    // 3단계: 커서 뒤쪽 나머지 텍스트 그리기
                    std::string right_text = m_Lines[i].substr(m_CursorByteOffset);
                    draw_list->AddText(ImVec2(current_x, text_draw_y), ImColor(30, 30, 30), right_text.c_str());
                }
                else
                {
                    // 커서가 없는 일반 줄은 한 번에 편하게 그립니다.
                    draw_list->AddText(ImVec2(text_start_x, text_draw_y), ImColor(30, 30, 30), m_Lines[i].c_str());
                }
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        int display_w, display_h;
        SDL_GetWindowSizeInPixels(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    FcFini();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}