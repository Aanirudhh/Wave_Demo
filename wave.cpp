#include <windows.h>
#include <math.h>
#include <omp.h>

// Framebuffer
static bool       running       = true;
static void*      buffer_memory = nullptr;
static const int  WIDTH         = 800;
static const int  HEIGHT        = 600;
static BITMAPINFO bmi;

// Timing
static double perf_freq;
static double time_elapsed = 0.0;   // seconds since start
static LARGE_INTEGER last_tick;

// Palette
// 256-entry smooth HSV-style palette baked at startup
static unsigned int palette[256];

static void build_palette() {
    for (int i = 0; i < 256; i++) {
        float t = i / 256.0f;
        // Three sine waves offset by 120° give a smooth rainbow cycle
        float r = 0.5f + 0.5f * sinf(t * 6.2832f + 0.0f);
        float g = 0.5f + 0.5f * sinf(t * 6.2832f + 2.094f);
        float b = 0.5f + 0.5f * sinf(t * 6.2832f + 4.189f);
        int ri = (int)(r * 255);
        int gi = (int)(g * 255);
        int bi = (int)(b * 255);
        palette[i] = (ri << 16) | (gi << 8) | bi;
    }
}

// Plasma modes
// Press 1-5 to switch between modes
static int mode = 0;
static const int NUM_MODES = 5;

static void render_plasma(double t) {
    unsigned int* pixels = (unsigned int*)buffer_memory;
    float ft = (float)t;

    #pragma omp parallel for schedule(dynamic, 8)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float fx = (float)x / WIDTH;
            float fy = (float)y / HEIGHT;
            float v = 0.0f;

            switch (mode) {
            case 0: // Classic: sum of sine waves
                v  = sinf(fx * 10.0f + ft);
                v += sinf(fy * 10.0f + ft * 0.7f);
                v += sinf((fx + fy) * 8.0f + ft * 1.3f);
                v += sinf(sqrtf(fx*fx + fy*fy) * 12.0f - ft * 2.0f);
                break;

            case 1: // Ripple rings from center
            {
                float cx = fx - 0.5f + 0.3f * sinf(ft * 0.7f);
                float cy = fy - 0.5f + 0.3f * cosf(ft * 0.5f);
                float dist = sqrtf(cx*cx + cy*cy);
                v = sinf(dist * 20.0f - ft * 3.0f);
                v += sinf(dist * 10.0f + ft);
            } break;

            case 2: // Interference pattern — two moving sources
            {
                float ax = 0.3f + 0.2f * sinf(ft * 0.8f);
                float ay = 0.5f + 0.2f * cosf(ft * 0.6f);
                float bx = 0.7f + 0.2f * cosf(ft * 0.9f);
                float by = 0.5f + 0.2f * sinf(ft * 0.7f);
                float da = sqrtf((fx-ax)*(fx-ax) + (fy-ay)*(fy-ay));
                float db = sqrtf((fx-bx)*(fx-bx) + (fy-by)*(fy-by));
                v = sinf(da * 30.0f - ft * 2.0f) + sinf(db * 30.0f - ft * 2.5f);
            } break;

            case 3: // Tunnel / zoom
            {
                float cx = fx - 0.5f;
                float cy = fy - 0.5f;
                float angle = atan2f(cy, cx);
                float dist  = sqrtf(cx*cx + cy*cy);
                v  = sinf(angle * 6.0f + ft * 2.0f);
                v += sinf(1.0f / (dist + 0.01f) * 0.5f - ft * 3.0f);
            } break;

            case 4: // Twisted grid
                v  = sinf(fx * 12.0f + sinf(fy * 4.0f + ft));
                v += sinf(fy * 12.0f + sinf(fx * 4.0f + ft * 1.2f));
                v += sinf((fx - fy) * 8.0f + ft * 0.8f);
                break;
            }

            // Map [-4..4] → [0..255]
            int idx = (int)((v / 4.0f + 1.0f) * 0.5f * 255.0f) & 0xFF;
            pixels[y * WIDTH + x] = palette[idx];
        }
    }
}

// Window procedure
LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
    case WM_DESTROY:
        running = false;
        break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE: running = false; break;
        case '1': case VK_NUMPAD1: mode = 0; break;
        case '2': case VK_NUMPAD2: mode = 1; break;
        case '3': case VK_NUMPAD3: mode = 2; break;
        case '4': case VK_NUMPAD4: mode = 3; break;
        case '5': case VK_NUMPAD5: mode = 4; break;
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Entry point 
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    build_palette();

    buffer_memory = VirtualAlloc(0, WIDTH * HEIGHT * 4, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth       = WIDTH;
    bmi.bmiHeader.biHeight      = -HEIGHT;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    WNDCLASSA wc    = {};
    wc.style        = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc  = window_callback;
    wc.hInstance    = hInstance;
    wc.lpszClassName = "PlasmaWindow";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA(wc.lpszClassName,
        "Plasma Demo  [1-5 to switch mode, ESC to quit]",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT,
        0, 0, hInstance, 0);
    HDC hdc = GetDC(hwnd);

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    perf_freq = (double)freq.QuadPart;
    QueryPerformanceCounter(&last_tick);

    while (running) {
        // timing
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double dt = (double)(now.QuadPart - last_tick.QuadPart) / perf_freq;
        last_tick = now;
        if (dt > 0.05) dt = 0.05;
        time_elapsed += dt;

        // input
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // simulate + render
        render_plasma(time_elapsed);

        StretchDIBits(hdc,
            0, 0, WIDTH, HEIGHT,
            0, HEIGHT, WIDTH, -HEIGHT,
            buffer_memory, &bmi, DIB_RGB_COLORS, SRCCOPY);
    }

    VirtualFree(buffer_memory, 0, MEM_RELEASE);
    return 0;
}
