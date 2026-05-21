// ## License / Credits
//
// - Original Dear ImGui SDL3+OpenGL3 Example code
//   Copyright (c) 2014-2026 Omar Cornut (Dear ImGui)
//
// - LeafEdit
//   Copyright (c) 2026 sinokadev

#include <SDL3/SDL.h>
#include <fontconfig/fontconfig.h>
#include <stdio.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_freetype.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#define TITLE_PREFIX "file.txt - LeafEdit "
#define VERSION "a0.1"

void UpdateWindowTitle(SDL_Window *window, const std::string &filename,
                       bool is_dirty) {
    std::string title =
        (is_dirty ? "*" : "") + filename + " - LeafEdit " VERSION;
    SDL_SetWindowTitle(window, title.c_str());
}

size_t GetByteOffset(const std::string &str, int char_index) {
    size_t byte_offset = 0;
    int count = 0;
    for (size_t i = 0; i < str.size() && count < char_index;) {
        unsigned char c = (unsigned char)str[i];
        if (c < 0x80)
            i += 1;
        else if (c < 0xE0)
            i += 2;
        else if (c < 0xF0)
            i += 3;
        else
            i += 4;
        byte_offset = i;
        count++;
    }
    return byte_offset;
}

int GetPreviousCharByteLength(const std::string &str, int byte_offset) {
    if (byte_offset <= 0) return 0;

    int len = 1;
    while (len <= 4 && byte_offset - len >= 0) {
        unsigned char c = (unsigned char)str[byte_offset - len];

        if ((c & 0xC0) != 0x80) {
            return len;
        }
        len++;
    }
    return 1;
}

std::string GetLinuxSystemMonospaceFontPath() {
    std::string font_path = "";
    if (!FcInit())
        return font_path;

    FcConfig *fc_config = FcConfigGetCurrent();
    FcPattern *pattern = FcPatternCreate();

    FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *)"monospace");

    FcConfigSubstitute(fc_config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    FcPattern *match = FcFontMatch(fc_config, pattern, &result);

    if (match) {
        FcChar8 *file_path = nullptr;
        if (FcPatternGetString(match, FC_FILE, 0, &file_path) ==
            FcResultMatch) {
            font_path = (const char *)file_path;
        }
        FcPatternDestroy(match);
    }

    FcPatternDestroy(pattern);

    return font_path;
}

void ConfigureLinuxNativeFont(ImFontConfig &config) {
    config.PixelSnapH = true;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.FontLoaderFlags = 0;

    if (!FcInit()) {
        config.FontLoaderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
        return;
    }

    FcConfig *fc_config = FcConfigGetCurrent();
    FcPattern *pattern = FcPatternCreate();

    FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *)"Noto Sans CJK KR");

    FcConfigSubstitute(fc_config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    FcPattern *match = FcFontMatch(fc_config, pattern, &result);

    if (match) {
        FcBool antialias = FcTrue;
        int hinting = FcTrue;
        int hint_style = FC_HINT_SLIGHT;

        FcPatternGetBool(match, FC_ANTIALIAS, 0, &antialias);
        FcPatternGetInteger(match, FC_HINTING, 0, &hinting);
        FcPatternGetInteger(match, FC_HINT_STYLE, 0, &hint_style);

        if (antialias == FcFalse) {
            config.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_MonoHinting |
                                      ImGuiFreeTypeBuilderFlags_Monochrome;
        } else {
            if (hinting == FcFalse || hint_style == FC_HINT_NONE) {
                config.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_NoHinting;
            } else if (hint_style == FC_HINT_SLIGHT ||
                       hint_style == FC_HINT_MEDIUM) {
                config.FontLoaderFlags |=
                    ImGuiFreeTypeBuilderFlags_LightHinting;
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

int main(int, char **) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char *glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    const char *glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                   SDL_WINDOW_HIDDEN |
                                   SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window *window =
        SDL_CreateWindow(TITLE_PREFIX VERSION, (int)(1280 * main_scale),
                         (int)(800 * main_scale), window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsLight();

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImFontConfig config;

    config.PixelSnapH = true;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.FontLoaderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;

    float font_size = 23.0f;

    ImFont *font = io.Fonts->AddFontFromFileTTF(
        GetLinuxSystemMonospaceFontPath().c_str(), font_size, &config,
        io.Fonts->GetGlyphRangesKorean());

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static std::vector<std::string> m_Lines = {""};
    static int m_CursorLine = 0;
    static int m_CursorByteOffset = 0;
    static bool m_IsDirty = false;
    static bool m_CursorChanged = false;

    static std::string m_CompositionText = "";

    SDL_StartTextInput(window);

    bool done = false;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
                SDL_Delay(10);
                continue;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_BACKSPACE) {
                    std::string &line = m_Lines[m_CursorLine];
                    if (m_CursorByteOffset > 0) {
                        int bytes_to_remove =
                            GetPreviousCharByteLength(line, m_CursorByteOffset);

                        if (bytes_to_remove > 0) {
                            line.erase(m_CursorByteOffset - bytes_to_remove,
                                       bytes_to_remove);
                            m_CursorByteOffset -= bytes_to_remove;
                        }
                    } else if (m_CursorLine > 0) {
                        m_CursorByteOffset = m_Lines[m_CursorLine - 1].size();
                        m_Lines[m_CursorLine - 1] += m_Lines[m_CursorLine];
                        m_Lines.erase(m_Lines.begin() + m_CursorLine);
                        m_CursorLine--;
                    }
                    m_CursorChanged = true;
                }

                if (event.key.key == SDLK_RETURN) {
                    std::string &current_line = m_Lines[m_CursorLine];

                    std::string next_line_text =
                        current_line.substr(m_CursorByteOffset);

                    current_line = current_line.substr(0, m_CursorByteOffset);

                    m_Lines.insert(m_Lines.begin() + m_CursorLine + 1,
                                   next_line_text);

                    m_CursorLine++;

                    m_CursorByteOffset = 0;

                    m_CursorChanged = true;
                }

                if ((SDL_GetModState() & SDLK_LCTRL) &&
                    event.key.key == SDLK_S) {
                    std::ofstream s_file;

                    s_file.open("file.txt");
                    if (s_file.is_open()) {
                        for (size_t i = 0; i < m_Lines.size(); ++i) {
                            if (i > 0) {
                                s_file << "\n";
                            }
                            s_file << m_Lines[i];
                        }
                    }

                    SDL_SetWindowTitle(window, TITLE_PREFIX VERSION);

                    s_file.close();

                    m_IsDirty = false;
                    UpdateWindowTitle(window, "file.txt", m_IsDirty);
                }
            }

            if (event.type == SDL_EVENT_TEXT_EDITING) {
                m_CompositionText = event.edit.text;
                continue;
            }

            if (event.type == SDL_EVENT_TEXT_INPUT) {
                if (!m_IsDirty) {
                    m_IsDirty = true;
                    UpdateWindowTitle(window, "file.txt", m_IsDirty);
                }

                m_CompositionText.clear();
                std::string input_str = event.text.text;

                m_Lines[m_CursorLine].insert(m_CursorByteOffset, input_str);

                m_CursorByteOffset += (int)input_str.size();

                if (m_CursorByteOffset > (int)m_Lines[m_CursorLine].size()) {
                    m_CursorByteOffset = (int)m_Lines[m_CursorLine].size();
                }

                m_CursorChanged = true;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        {
            ImGuiWindowFlags window_flags =
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_MenuBar;

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

            static float f = 0.0f;
            static int counter = 0;

            if (ImGui::Begin("LeafEdit", nullptr, window_flags)) {

                if (ImGui::BeginMenuBar()) {

                    if (ImGui::BeginMenu("File")) {
                        if (ImGui::MenuItem("Open")) { /* ... */
                        }
                        if (ImGui::MenuItem("Save")) { /* ... */
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Edit")) {
                        if (ImGui::MenuItem("Undo")) { /* ... */
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }

                ImVec2 temp_size = ImGui::GetContentRegionAvail();

                if (ImGui::BeginChild("Editor", temp_size, ImGuiChildFlags_None,
                                      ImGuiWindowFlags_HorizontalScrollbar)) {
                    ImDrawList *draw_list = ImGui::GetWindowDrawList();
                    ImVec2 start_pos = ImGui::GetCursorScreenPos();

                    float max_width = 100.0f;
                    float total_height =
                        m_Lines.size() * (ImGui::GetTextLineHeight() + 6.0f);

                    for (const auto &line : m_Lines) {
                        float line_w =
                            ImGui::CalcTextSize(line.c_str()).x + 80.0f;
                        if (line_w > max_width)
                            max_width = line_w;
                    }

                    ImGui::Dummy(ImVec2(max_width, total_height));

                    if (m_CursorChanged) {
                        float line_height = ImGui::GetTextLineHeight() + 6.0f;
                        float target_y =
                            start_pos.y + (m_CursorLine * line_height);

                        ImGui::SetScrollFromPosY(
                            target_y - ImGui::GetWindowPos().y, 0.5f);

                        m_CursorChanged = false;
                    }

                    ImFont *current_font = ImGui::GetFont();
                    float total_line_height = ImGui::GetTextLineHeight() + 6.0f;
                    float char_width = current_font
                                           ->CalcTextSizeA(ImGui::GetFontSize(),
                                                           FLT_MAX, -1.0f, "A")
                                           .x;

                    for (size_t i = 0; i < m_Lines.size(); i++) {
                        float box_top_y = start_pos.y + (i * total_line_height);
                        float text_draw_y = box_top_y + 3.0f;
                        char num_buf[16];
                        sprintf(num_buf, "%3zu", i + 1);
                        draw_list->AddText(
                            ImVec2(start_pos.x + 10.0f, text_draw_y),
                            ImColor(120, 120, 120), num_buf);

                        float text_start_x = start_pos.x + 60.0f;

                        if (i == (size_t)m_CursorLine) {

                            size_t safe_offset = m_CursorByteOffset;
                            if (safe_offset > m_Lines[i].size())
                                safe_offset = m_Lines[i].size();

                            std::string left_text =
                                m_Lines[i].substr(0, safe_offset);
                            draw_list->AddText(
                                ImVec2(text_start_x, text_draw_y),
                                ImColor(30, 30, 30), left_text.c_str());

                            float left_width =
                                current_font
                                    ->CalcTextSizeA(ImGui::GetFontSize(),
                                                    FLT_MAX, -1.0f,
                                                    left_text.c_str())
                                    .x;
                            float current_x = text_start_x + left_width;

                            float cursor_x = text_start_x + left_width;
                            float cursor_y = box_top_y;

                            SDL_Rect area = {(int)cursor_x, (int)cursor_y, 1,
                                             (int)total_line_height};
                            SDL_SetTextInputArea(window, &area, 0);
                            if (!m_CompositionText.empty()) {
                                draw_list->AddText(
                                    ImVec2(current_x, text_draw_y),
                                    ImColor(100, 100, 100),
                                    m_CompositionText.c_str());

                                float comp_width =
                                    current_font
                                        ->CalcTextSizeA(
                                            ImGui::GetFontSize(), FLT_MAX,
                                            -1.0f, m_CompositionText.c_str())
                                        .x;

                                draw_list->AddLine(
                                    ImVec2(current_x, box_top_y +
                                                          total_line_height -
                                                          2.0f),
                                    ImVec2(current_x + comp_width,
                                           box_top_y + total_line_height -
                                               2.0f),
                                    ImColor(100, 100, 100), 1.5f);

                                current_x += comp_width;
                            } else {
                                if (fmod(ImGui::GetTime(), 1.0f) < 0.5f) {
                                    draw_list->AddRectFilled(
                                        ImVec2(current_x, box_top_y + 2.0f),
                                        ImVec2(current_x + 2.0f,
                                               box_top_y + total_line_height -
                                                   2.0f),
                                        ImColor(0, 0, 0));
                                }
                            }

                            std::string right_text =
                                m_Lines[i].substr(safe_offset);
                            draw_list->AddText(ImVec2(current_x, text_draw_y),
                                               ImColor(30, 30, 30),
                                               right_text.c_str());
                        } else {
                            draw_list->AddText(
                                ImVec2(text_start_x, text_draw_y),
                                ImColor(30, 30, 30), m_Lines[i].c_str());
                        }
                    }
                }

                ImGui::EndChild();
            }
            ImGui::End();
        }

        ImGui::Render();

        int display_w, display_h;
        SDL_GetWindowSizeInPixels(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(clear_color.x * clear_color.w,
                     clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    FcFini();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}