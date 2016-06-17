// ImGui - standalone example application for Glfw + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>
#include <dirent.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

struct Picture
{
    int width;
    int height;
    int channel;
    int index;
    unsigned char *data;
};

struct FileList
{
    int current;
    int file_count;
    char **files;
};

struct DrawData
{
    ImVec2 pos;
    ImVec2 uv;

    DrawData() { pos = ImVec2(); uv = ImVec2(); }
    DrawData(const ImVec2& p, const ImVec2& u) { pos = ImVec2(p.x, p.y); uv = ImVec2(u.x, u.y); }
};

FileList make_file_list(const char *folder)
{
    FileList file_list = {};
    file_list.current = -1;
    DIR *dir;
    struct dirent *dir_entry;
    dir = opendir(folder);
    if (dir)
    {
        while ((dir_entry = readdir(dir)) != NULL)
        {
            if (dir_entry->d_type == DT_REG)
            {
                file_list.files = (char **)realloc(file_list.files, (file_list.file_count + 1) * sizeof(char *));
                file_list.files[file_list.file_count++] = strdup(dir_entry->d_name);
            }
        }
        closedir(dir);
    }

    return file_list;
}

bool file_list_getter(void* data, int idx, const char** out_text)
{
    const char** items = (const char**)data;
    if (out_text)
        *out_text = items[idx];
    return true;
}

void check_shader(void (*param_getter)(GLuint program, GLenum pname, GLint *params),
                  GLuint handle, GLenum param,
                  void (*log_getter)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog))
{
    GLint result = GL_FALSE;
    int info_log_length = 0;
    param_getter(handle, param, &result);
    param_getter(handle, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 0)
    {
        int output_length;
        char error_messages[info_log_length+1];
        log_getter(handle, info_log_length, &output_length, error_messages);
        error_messages[info_log_length] = '\0';
        fprintf(stderr, "%s", error_messages);
    }

}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

int main(int argc, char* argv[])
{
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Fluid Simulator", NULL, NULL);
    glfwMakeContextCurrent(window);
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    // Setting opengl stuff
    const GLchar *vertex_shader =
        "#version 330\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "out vec2 Frag_UV;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 330\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = texture( Texture, Frag_UV );\n"
        "}\n";

    GLuint shader_handle = glCreateProgram();
    GLuint vert_handle = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vert_handle, 1, &vertex_shader, 0);
    glShaderSource(frag_handle, 1, &fragment_shader, 0);
    glCompileShader(vert_handle); check_shader(glGetShaderiv, vert_handle, GL_COMPILE_STATUS, glGetShaderInfoLog);
    glCompileShader(frag_handle); check_shader(glGetShaderiv, frag_handle, GL_COMPILE_STATUS, glGetShaderInfoLog);
    glAttachShader(shader_handle, vert_handle);
    glAttachShader(shader_handle, frag_handle);
    glLinkProgram(shader_handle); check_shader(glGetProgramiv, shader_handle, GL_LINK_STATUS, glGetProgramInfoLog);

    GLuint attrib_location_projmtx = glGetUniformLocation(shader_handle, "ProjMtx");
    GLuint attrib_location_position = glGetAttribLocation(shader_handle, "Position");
    GLuint attrib_location_uv = glGetAttribLocation(shader_handle, "UV");

    GLuint texture_handle;
    glGenTextures(1, &texture_handle);
    glBindTexture(GL_TEXTURE_2D, texture_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint vbo_handle;
    glGenBuffers(1, &vbo_handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle);

    GLuint vao_handle;
    glGenVertexArrays(1, &vao_handle);
    glBindVertexArray(vao_handle);

    glEnableVertexAttribArray(attrib_location_position);
    glEnableVertexAttribArray(attrib_location_uv);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(attrib_location_position, 2, GL_FLOAT, GL_FALSE, sizeof(DrawData), (GLvoid*)OFFSETOF(DrawData, pos));
    glVertexAttribPointer(attrib_location_uv, 2, GL_FLOAT, GL_FALSE, sizeof(DrawData), (GLvoid*)OFFSETOF(DrawData, uv));
#undef OFFSETOF

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    const char *folder = argv[1];
    FileList file_list = make_file_list(folder);
    Picture pic = {};
    pic.index = -1;

    DrawData draw_data[4] = {};

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Press A? %s", ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A))? "yes" : "no");
            ImGui::Text("%d", file_list.file_count);
            ImGui::Combo("Files", &file_list.current, file_list_getter, (void *)file_list.files, file_list.file_count);
        }

        if (file_list.current >= 0 && file_list.current != pic.index)
        {
            if (pic.data)
            {
                stbi_image_free(pic.data);
            }

            char filename[512];
            strncpy(filename, folder, sizeof(filename));
            strncat(filename, file_list.files[file_list.current], sizeof(filename));
            pic.index = file_list.current;
            pic.data = stbi_load(filename, &pic.width, &pic.height, &pic.channel, STBI_rgb_alpha);
            printf("%d, %d, %p, %d\n", pic.width, pic.height, pic.data, pic.channel);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pic.width, pic.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic.data);

            draw_data[0] = DrawData(ImVec2(0.0f, 0.0f), ImVec2(0.0f, 0.0f));
            draw_data[1] = DrawData(ImVec2((float)pic.width, 0.0f), ImVec2(1.0f, 0.0f));
            draw_data[2] = DrawData(ImVec2(0.0f, (float)pic.height), ImVec2(0.0f, 1.0f));
            draw_data[3] = DrawData(ImVec2((float)pic.width, (float)pic.height), ImVec2(1.0f, 1.0f));
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        const float ortho_projection[4][4] =
            {
                { 2.0f/ImGui::GetIO().DisplaySize.x, 0.0f,                   0.0f, 0.0f },
                { 0.0f,                  2.0f/-ImGui::GetIO().DisplaySize.y, 0.0f, 0.0f },
                { 0.0f,                  0.0f,                              -1.0f, 0.0f },
                {-1.0f,                  1.0f,                               0.0f, 1.0f },
            };
        glUseProgram(shader_handle);
        glUniformMatrix4fv(attrib_location_projmtx, 1, GL_FALSE, &ortho_projection[0][0]);
        glBindVertexArray(vao_handle);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_handle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)IM_ARRAYSIZE(draw_data) * sizeof(DrawData),
                     (GLvoid *)draw_data, GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, IM_ARRAYSIZE(draw_data));

        ImGui::Render();

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    return 0;
}
