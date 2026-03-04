#define GL_SILENCE_DEPRECATION
#import "mac_window.h"
#import "renderer_utils.h"
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

@interface OpenGLView : NSOpenGLView
@end

@implementation OpenGLView
- (BOOL)isOpaque {
    return YES;
}
@end

@interface WindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) bool* shouldClosePtr;
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
    *self.shouldClosePtr = true;
    return YES;
}
@end

namespace Platform {

static NSWindow* g_window = nil;
static NSOpenGLContext* g_glContext = nil;
static bool g_shouldClose = false;

bool init_window(const WindowConfig& config)
{
    @autoreleasepool {
        [NSApplication sharedApplication];

        NSUInteger style = NSWindowStyleMaskTitled |
                           NSWindowStyleMaskClosable |
                           NSWindowStyleMaskResizable;

        NSRect rect = NSMakeRect(0, 0, config.width, config.height);

        g_window = [[NSWindow alloc] initWithContentRect:rect
                                                styleMask:style
                                                  backing:NSBackingStoreBuffered
                                                    defer:NO];
        [g_window setTitle:[NSString stringWithUTF8String:config.title]];

        // Create OpenGL pixel format
        NSOpenGLPixelFormatAttribute attrs[] = {
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFADepthSize, 24,
            0
        };

        NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        
        // Create OpenGL view with the pixel format
        OpenGLView* glView = [[OpenGLView alloc] initWithFrame:rect pixelFormat:pixelFormat];
        [glView setWantsBestResolutionOpenGLSurface:YES];
        
        // Get the context from the view and make it current
        g_glContext = [glView openGLContext];
        [g_glContext makeCurrentContext];

        // Set swap interval (vsync)
        GLint swapInt = 1;
        [g_glContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

        glViewport(0, 0, config.width, config.height);

        // Set the OpenGL view as the window's content view
        [g_window setContentView:glView];

        // Set up window delegate for close events
        WindowDelegate* delegate = [[WindowDelegate alloc] init];
        delegate.shouldClosePtr = &g_shouldClose;
        [g_window setDelegate:delegate];

        [NSApp finishLaunching];
        [g_window makeKeyAndOrderFront:nil];

        return true;
    }
}

void swap_buffers()
{
    @autoreleasepool {
        [g_glContext makeCurrentContext];
        [g_glContext flushBuffer];
    }
}

void clear_screen()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool window_should_close()
{
    @autoreleasepool {
        NSEvent* event;
        do {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:[NSDate distantPast]
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES];
            if (event) {
                [NSApp sendEvent:event];
            }
        } while (event != nil);

        [NSApp updateWindows];
        return g_shouldClose;
    }
}

GLuint quadVAO = 0;
GLuint quadVBO = 0;
GLuint shaderProgram = 0;

void init_renderer()
{
    float vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
        -0.5f,  0.5f,
         0.5f,  0.5f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    const char* vertexSrc = "#version 330 core\nlayout(location=0) in vec2 aPos;\nvoid main() { gl_Position = vec4(aPos,0.0,1.0); }";
    const char* fragSrc   = "#version 330 core\nout vec4 FragColor;\nvoid main() { FragColor = vec4(1.0,0.0,0.0,1.0); }";

    shaderProgram = compile_and_link_shader(vertexSrc, fragSrc);
}

void render_quad()
{
    glUseProgram(shaderProgram);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void shutdown()
{
    @autoreleasepool {
        [g_window close];
        g_window = nil;
        g_glContext = nil;
    }
}

}
